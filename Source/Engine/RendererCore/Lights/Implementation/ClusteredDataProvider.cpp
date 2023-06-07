#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Profiling/Profiling.h>

xiiClusteredDataGPU::xiiClusteredDataGPU()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  {
    xiiGALBufferCreationDescription desc;

    {
      desc.m_uiStructSize                = sizeof(xiiPerLightData);
      desc.m_uiTotalSize                 = desc.m_uiStructSize * xiiClusteredDataCPU::MAX_LIGHT_DATA;
      desc.m_BufferType                  = xiiGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer      = true;
      desc.m_bAllowShaderResourceView    = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hLightDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize                = sizeof(xiiPerDecalData);
      desc.m_uiTotalSize                 = desc.m_uiStructSize * xiiClusteredDataCPU::MAX_DECAL_DATA;
      desc.m_BufferType                  = xiiGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer      = true;
      desc.m_bAllowShaderResourceView    = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hDecalDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize                = sizeof(xiiPerReflectionProbeData);
      desc.m_uiTotalSize                 = desc.m_uiStructSize * xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA;
      desc.m_BufferType                  = xiiGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer      = true;
      desc.m_bAllowShaderResourceView    = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hReflectionProbeDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(xiiPerClusterData);
      desc.m_uiTotalSize  = desc.m_uiStructSize * NUM_CLUSTERS;

      m_hClusterDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(xiiUInt32);
      desc.m_uiTotalSize  = desc.m_uiStructSize * xiiClusteredDataCPU::MAX_ITEMS_PER_CLUSTER * NUM_CLUSTERS;

      m_hClusterItemBuffer = pDevice->CreateBuffer(desc);
    }
  }

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiClusteredDataConstants>(XII_STRINGIZE(xiiClusteredDataConstants));

  {
    xiiGALSamplerStateCreationDescription desc;
    desc.m_AddressU          = xiiImageAddressMode::Clamp;
    desc.m_AddressV          = xiiImageAddressMode::Clamp;
    desc.m_AddressW          = xiiImageAddressMode::Clamp;
    desc.m_SampleCompareFunc = xiiGALCompareFunc::Less;

    m_hShadowSampler = pDevice->CreateSamplerState(desc);
  }

  m_hDecalAtlas = xiiDecalAtlasResource::GetDecalAtlasResource();

  {
    xiiGALSamplerStateCreationDescription desc;
    desc.m_AddressU = xiiImageAddressMode::Clamp;
    desc.m_AddressV = xiiImageAddressMode::Clamp;
    desc.m_AddressW = xiiImageAddressMode::Clamp;

    xiiTextureUtils::ConfigureSampler(xiiTextureFilterSetting::DefaultQuality, desc);
    desc.m_uiMaxAnisotropy = xiiMath::Min(desc.m_uiMaxAnisotropy, 4u);

    m_hDecalAtlasSampler = pDevice->CreateSamplerState(desc);
  }
}

xiiClusteredDataGPU::~xiiClusteredDataGPU()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hLightDataBuffer);
  pDevice->DestroyBuffer(m_hDecalDataBuffer);
  pDevice->DestroyBuffer(m_hReflectionProbeDataBuffer);
  pDevice->DestroyBuffer(m_hClusterDataBuffer);
  pDevice->DestroyBuffer(m_hClusterItemBuffer);
  pDevice->DestroySamplerState(m_hShadowSampler);
  pDevice->DestroySamplerState(m_hDecalAtlasSampler);

  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiClusteredDataGPU::BindResources(xiiRenderContext* pRenderContext)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto hShadowDataBufferView   = pDevice->GetDefaultResourceView(xiiShadowPool::GetShadowDataBuffer());
  auto hShadowAtlasTextureView = pDevice->GetDefaultResourceView(xiiShadowPool::GetShadowAtlasTexture());

  auto hReflectionSpecularTextureView = pDevice->GetDefaultResourceView(xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint));
  auto hSkyIrradianceTextureView      = pDevice->GetDefaultResourceView(xiiReflectionPool::GetSkyIrradianceTexture());

  pRenderContext->BindBuffer("perLightDataBuffer", pDevice->GetDefaultResourceView(m_hLightDataBuffer));
  pRenderContext->BindBuffer("perDecalDataBuffer", pDevice->GetDefaultResourceView(m_hDecalDataBuffer));
  pRenderContext->BindBuffer("perPerReflectionProbeDataBuffer", pDevice->GetDefaultResourceView(m_hReflectionProbeDataBuffer));
  pRenderContext->BindBuffer("perClusterDataBuffer", pDevice->GetDefaultResourceView(m_hClusterDataBuffer));
  pRenderContext->BindBuffer("clusterItemBuffer", pDevice->GetDefaultResourceView(m_hClusterItemBuffer));

  pRenderContext->BindBuffer("shadowDataBuffer", hShadowDataBufferView);
  pRenderContext->BindTexture2D("ShadowAtlasTexture", hShadowAtlasTextureView);
  pRenderContext->BindSamplerState("ShadowSampler", m_hShadowSampler);

  xiiResourceLock<xiiDecalAtlasResource> pDecalAtlas(m_hDecalAtlas, xiiResourceAcquireMode::AllowLoadingFallback);
  pRenderContext->BindTexture2D("DecalAtlasBaseColorTexture", pDecalAtlas->GetBaseColorTexture());
  pRenderContext->BindTexture2D("DecalAtlasNormalTexture", pDecalAtlas->GetNormalTexture());
  pRenderContext->BindTexture2D("DecalAtlasORMTexture", pDecalAtlas->GetORMTexture());
  pRenderContext->BindSamplerState("DecalAtlasSampler", m_hDecalAtlasSampler);

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer("xiiClusteredDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusteredDataProvider, 1, xiiRTTIDefaultAllocator<xiiClusteredDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiClusteredDataProvider::xiiClusteredDataProvider() = default;

xiiClusteredDataProvider::~xiiClusteredDataProvider() = default;

void* xiiClusteredDataProvider::UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData)
{
  xiiGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetRenderCommandEncoder();

  XII_PROFILE_AND_MARKER(pGALCommandEncoder, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<xiiClusteredDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint      = pData->m_cameraUsageHint;

    // Update buffer
    if (!pData->m_ClusterItemList.IsEmpty())
    {
      if (!pData->m_LightData.IsEmpty())
      {
        pGALCommandEncoder->UpdateBuffer(m_Data.m_hLightDataBuffer, 0, pData->m_LightData.ToByteArray());
      }

      if (!pData->m_DecalData.IsEmpty())
      {
        pGALCommandEncoder->UpdateBuffer(m_Data.m_hDecalDataBuffer, 0, pData->m_DecalData.ToByteArray());
      }

      if (!pData->m_ReflectionProbeData.IsEmpty())
      {
        pGALCommandEncoder->UpdateBuffer(m_Data.m_hReflectionProbeDataBuffer, 0, pData->m_ReflectionProbeData.ToByteArray());
      }

      pGALCommandEncoder->UpdateBuffer(m_Data.m_hClusterItemBuffer, 0, pData->m_ClusterItemList.ToByteArray());
    }

    pGALCommandEncoder->UpdateBuffer(m_Data.m_hClusterDataBuffer, 0, pData->m_ClusterData.ToByteArray());

    // Update Constants
    const xiiRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    xiiClusteredDataConstants* pConstants =
      renderViewContext.m_pRenderContext->GetConstantBufferData<xiiClusteredDataConstants>(m_Data.m_hConstantBuffer);
    pConstants->DepthSliceScale = s_fDepthSliceScale;
    pConstants->DepthSliceBias  = s_fDepthSliceBias;
    pConstants->InvTileSize     = xiiVec2(NUM_CLUSTERS_X / viewport.width, NUM_CLUSTERS_Y / viewport.height);
    pConstants->NumLights       = pData->m_LightData.GetCount();
    pConstants->NumDecals       = pData->m_DecalData.GetCount();

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;

    pConstants->FogHeight             = pData->m_fFogHeight;
    pConstants->FogHeightFalloff      = pData->m_fFogHeightFalloff;
    pConstants->FogDensityAtCameraPos = pData->m_fFogDensityAtCameraPos;
    pConstants->FogDensity            = pData->m_fFogDensity;
    pConstants->FogColor              = pData->m_FogColor;
    pConstants->FogInvSkyDistance     = pData->m_fFogInvSkyDistance;
  }

  return &m_Data;
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataProvider);
