#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsFoundation/Profiling/Profiling.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Sampler.h>

#include <GraphicsCore/Decals/DecalAtlasResource.h>
#include <GraphicsCore/Lights/ClusteredDataExtractor.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Lights/Implementation/ClusteredDataUtils.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Lights/Implementation/ShadowPool.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/TextureUtils.h>

xiiClusteredDataGPU::xiiClusteredDataGPU()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  {
    xiiGALBufferCreationDescription desc;
    desc.m_Mode = xiiGALBufferMode::Structured;
    desc.m_BindFlags.Add(xiiGALBindFlags::ShaderResource);

    {
      desc.m_uiElementByteStride = sizeof(xiiPerLightData);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_LIGHT_DATA;

      m_hLightDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiPerDecalData);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_DECAL_DATA;

      m_hDecalDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiPerReflectionProbeData);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA;

      m_hReflectionProbeDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiPerClusterData);
      desc.m_uiSize              = desc.m_uiElementByteStride * NUM_CLUSTERS;

      m_hClusterDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiUInt32);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_ITEMS_PER_CLUSTER * NUM_CLUSTERS;

      m_hClusterItemBuffer = pDevice->CreateBuffer(desc);
    }
  }

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiClusteredDataConstants>();

  {
    xiiGALSamplerCreationDescription desc;
    desc.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_ComparisonFunction = xiiGALComparisonFunction::Less;

    m_hShadowSampler = pDevice->CreateSampler(desc);
  }

  m_hDecalAtlas = xiiDecalAtlasResource::GetDecalAtlasResource();

  {
    xiiGALSamplerCreationDescription desc;
    desc.m_AddressU = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressV = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressW = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);

    xiiTextureUtils::ConfigureSampler(xiiTextureFilterSetting::DefaultQuality, desc);
    desc.m_uiMaxAnisotropy = xiiMath::Min(desc.m_uiMaxAnisotropy, 4u);

    m_hDecalAtlasSampler = pDevice->CreateSampler(desc);
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
  pDevice->DestroySampler(m_hShadowSampler);
  pDevice->DestroySampler(m_hDecalAtlasSampler);

  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiClusteredDataGPU::BindResources(xiiRenderContext* pRenderContext)
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  auto hShadowDataBufferView = xiiGALBufferViewHandle();
  if (xiiGALBuffer* pBuffer = pDevice->GetBuffer(xiiShadowPool::GetShadowDataBuffer()))
  {
    hShadowDataBufferView = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
  }

  auto hShadowAtlasTextureView = xiiGALTextureViewHandle();
  if (xiiGALTexture* pTexture = pDevice->GetTexture(xiiShadowPool::GetShadowAtlasTexture()))
  {
    hShadowAtlasTextureView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
  }

#if XII_RENDERER_ENABLE
  auto hReflectionSpecularTextureView = pDevice->GetTexture(xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint))->GetDefaultView(xiiGALTextureViewType::ShaderResource);
  auto hSkyIrradianceTextureView      = pDevice->GetTexture(xiiReflectionPool::GetSkyIrradianceTexture())->GetDefaultView(xiiGALTextureViewType::ShaderResource);
#endif

  pRenderContext->BindBuffer("perLightDataBuffer", pDevice->GetBuffer(m_hLightDataBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("perDecalDataBuffer", pDevice->GetBuffer(m_hDecalDataBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("perPerReflectionProbeDataBuffer", pDevice->GetBuffer(m_hReflectionProbeDataBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("perClusterDataBuffer", pDevice->GetBuffer(m_hClusterDataBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("clusterItemBuffer", pDevice->GetBuffer(m_hClusterItemBuffer)->GetDefaultView(xiiGALBufferViewType::ShaderResource));

  pRenderContext->BindBuffer("shadowDataBuffer", hShadowDataBufferView);
  pRenderContext->BindTexture2D("ShadowAtlasTexture", hShadowAtlasTextureView);
  pRenderContext->BindSampler("ShadowSampler", m_hShadowSampler);

  xiiResourceLock<xiiDecalAtlasResource> pDecalAtlas(m_hDecalAtlas, xiiResourceAcquireMode::AllowLoadingFallback);
  pRenderContext->BindTexture2D("DecalAtlasBaseColorTexture", pDecalAtlas->GetBaseColorTexture());
  pRenderContext->BindTexture2D("DecalAtlasNormalTexture", pDecalAtlas->GetNormalTexture());
  pRenderContext->BindTexture2D("DecalAtlasORMTexture", pDecalAtlas->GetORMTexture());
  pRenderContext->BindSampler("DecalAtlasSampler", m_hDecalAtlasSampler);

#if XII_RENDERER_ENABLE
  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);
#endif

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
  xiiGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetGraphicsCommandEncoder();

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

    xiiClusteredDataConstants* pConstants = renderViewContext.m_pRenderContext->GetConstantBufferData<xiiClusteredDataConstants>(m_Data.m_hConstantBuffer);
    pConstants->DepthSliceScale           = s_fDepthSliceScale;
    pConstants->DepthSliceBias            = s_fDepthSliceBias;
    pConstants->InvTileSize               = xiiVec2(NUM_CLUSTERS_X / viewport.width, NUM_CLUSTERS_Y / viewport.height);
    pConstants->NumLights                 = pData->m_LightData.GetCount();
    pConstants->NumDecals                 = pData->m_DecalData.GetCount();

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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_ClusteredDataProvider);
