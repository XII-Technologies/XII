#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Decals/DecalAtlasResource.h>
#include <GraphicsCore/Lights/ClusteredDataExtractor.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Lights/Implementation/ClusteredDataUtils.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Lights/Implementation/ShadowPool.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

xiiClusteredDataGPU::xiiClusteredDataGPU()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  {
    xiiGALBufferCreationDescription desc;
    desc.m_Mode           = xiiGALBufferMode::Structured;
    desc.m_BindFlags      = xiiGALBindFlags::ShaderResource;
    desc.m_Usage          = xiiGALResourceUsage::Dynamic;
    desc.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;

    {
      desc.m_uiElementByteStride = sizeof(xiiPerLightData);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_LIGHT_DATA;

      m_pLightDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiPerDecalData);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_DECAL_DATA;

      m_pDecalDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiPerReflectionProbeData);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA;

      m_pReflectionProbeDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiPerClusterData);
      desc.m_uiSize              = desc.m_uiElementByteStride * NUM_CLUSTERS;

      m_pClusterDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiElementByteStride = sizeof(xiiUInt32);
      desc.m_uiSize              = desc.m_uiElementByteStride * xiiClusteredDataCPU::MAX_ITEMS_PER_CLUSTER * NUM_CLUSTERS;

      m_pClusterItemBuffer = pDevice->CreateBuffer(desc);
    }
  }

  m_hConstantBuffer = xiiRenderContext::CreateConstantBufferStorage<xiiClusteredDataConstants>();

  {
    xiiGALSamplerCreationDescription desc;
    desc.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_MinFilter          = xiiGALFilterType::ComparisonLinear;
    desc.m_MagFilter          = xiiGALFilterType::ComparisonLinear;
    desc.m_MipFilter          = xiiGALFilterType::ComparisonLinear;
    desc.m_ComparisonFunction = xiiGALComparisonFunction::Less;
    desc.m_BorderColor        = xiiColor::Black;
    desc.m_fMipLODBias        = 0.0f;
    desc.m_fMinLOD            = -1.0f;
    desc.m_fMaxLOD            = 42000.0f;
    desc.m_uiMaxAnisotropy    = 4U;

    m_pShadowSampler = pDevice->CreateSampler(desc);
  }

  m_hDecalAtlas = xiiDecalAtlasResource::GetDecalAtlasResource();

  {
    xiiGALSamplerCreationDescription desc;
    desc.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    desc.m_MinFilter          = xiiGALFilterType::Linear;
    desc.m_MagFilter          = xiiGALFilterType::Linear;
    desc.m_MipFilter          = xiiGALFilterType::Linear;
    desc.m_ComparisonFunction = xiiGALComparisonFunction::Never;
    desc.m_BorderColor        = xiiColor::Black;
    desc.m_fMipLODBias        = 0.0f;
    desc.m_fMinLOD            = -1.0f;
    desc.m_fMaxLOD            = 42000.0f;
    desc.m_uiMaxAnisotropy    = 4U;

    xiiTextureUtils::ConfigureSampler(xiiTextureFilterSetting::DefaultQuality, desc);
    desc.m_uiMaxAnisotropy = xiiMath::Min(desc.m_uiMaxAnisotropy, 4u);

    m_pDecalAtlasSampler = pDevice->CreateSampler(desc);
  }
}

xiiClusteredDataGPU::~xiiClusteredDataGPU()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  m_pLightDataBuffer.Clear();
  m_pDecalDataBuffer.Clear();
  m_pReflectionProbeDataBuffer.Clear();
  m_pClusterDataBuffer.Clear();
  m_pClusterItemBuffer.Clear();
  m_pShadowSampler.Clear();
  m_pDecalAtlasSampler.Clear();

  xiiRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void xiiClusteredDataGPU::BindResources(xiiRenderContext* pRenderContext)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  xiiSharedPtr<xiiGALBufferView> pShadowDataBufferView;
  if (xiiSharedPtr<xiiGALBuffer> pBuffer = xiiShadowPool::GetShadowDataBuffer())
  {
    pShadowDataBufferView = pBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource);
  }

  xiiSharedPtr<xiiGALTextureView> pShadowAtlasTextureView;
  if (xiiSharedPtr<xiiGALTexture> pTexture = xiiShadowPool::GetShadowAtlasTexture())
  {
    pShadowAtlasTextureView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
  }

  xiiSharedPtr<xiiGALTextureView> pReflectionSpecularTextureView = xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint)->GetDefaultView(xiiGALTextureViewType::ShaderResource);
  xiiSharedPtr<xiiGALTextureView> pSkyIrradianceTextureView      = xiiReflectionPool::GetSkyIrradianceTexture()->GetDefaultView(xiiGALTextureViewType::ShaderResource);

  pRenderContext->BindBuffer("perLightDataBuffer", m_pLightDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("perDecalDataBuffer", m_pDecalDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("perPerReflectionProbeDataBuffer", m_pReflectionProbeDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("perClusterDataBuffer", m_pClusterDataBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  pRenderContext->BindBuffer("clusterItemBuffer", m_pClusterItemBuffer->GetDefaultView(xiiGALBufferViewType::ShaderResource));

  pRenderContext->BindBuffer("shadowDataBuffer", pShadowDataBufferView);
  pRenderContext->BindTexture2D("ShadowAtlasTexture", pShadowAtlasTextureView);
  pRenderContext->BindSampler("ShadowSampler", m_pShadowSampler);

  xiiResourceLock<xiiDecalAtlasResource> pDecalAtlas(m_hDecalAtlas, xiiResourceAcquireMode::AllowLoadingFallback);
  pRenderContext->BindTexture2D("DecalAtlasBaseColorTexture", pDecalAtlas->GetBaseColorTexture());
  pRenderContext->BindTexture2D("DecalAtlasNormalTexture", pDecalAtlas->GetNormalTexture());
  pRenderContext->BindTexture2D("DecalAtlasORMTexture", pDecalAtlas->GetORMTexture());
  pRenderContext->BindSampler("DecalAtlasSampler", m_pDecalAtlasSampler);

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", pReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", pSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer("xiiClusteredDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClusteredDataProvider, 1, xiiRTTIDefaultAllocator<xiiClusteredDataProvider>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiClusteredDataProvider::xiiClusteredDataProvider() = default;

xiiClusteredDataProvider::~xiiClusteredDataProvider() = default;

void* xiiClusteredDataProvider::UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData)
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  if (auto pData = extractedData.GetFrameData<xiiClusteredDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint      = pData->m_cameraUsageHint;

    auto pCommandList = renderViewContext.m_pRenderContext->GetCommandList();

    pCommandList->BeginDebugGroup("xiiClusteredDataProvider Update");
    {
      // Update buffer
      if (!pData->m_ClusterItemList.IsEmpty())
      {
        if (!pData->m_LightData.IsEmpty())
        {
          xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_Data.m_pLightDataBuffer, 0, pData->m_LightData.ToByteArray()).AssertSuccess();
        }

        if (!pData->m_DecalData.IsEmpty())
        {
          xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_Data.m_pDecalDataBuffer, 0, pData->m_DecalData.ToByteArray()).AssertSuccess();
        }

        if (!pData->m_ReflectionProbeData.IsEmpty())
        {
          xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_Data.m_pReflectionProbeDataBuffer, 0, pData->m_ReflectionProbeData.ToByteArray()).AssertSuccess();
        }

        xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_Data.m_pClusterItemBuffer, 0, pData->m_ClusterItemList.ToByteArray()).AssertSuccess();
      }

      xiiGALDeviceUtilities::MapAndUpdateBuffer(pCommandList, m_Data.m_pClusterDataBuffer, 0, pData->m_ClusterData.ToByteArray()).AssertSuccess();
    }
    pCommandList->EndDebugGroup();

    // Update Constants
    const xiiRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    xiiClusteredDataConstants* pConstants = xiiRenderContext::GetConstantBufferData<xiiClusteredDataConstants>(m_Data.m_hConstantBuffer);
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
