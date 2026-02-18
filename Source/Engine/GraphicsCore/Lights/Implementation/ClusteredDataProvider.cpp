#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Decals/DecalAtlasResource.h>
#include <GraphicsCore/Lights/ClusteredDataExtractor.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Lights/Implementation/ClusteredDataUtils.h>
#include <GraphicsCore/Lights/Implementation/ReflectionPool.h>
#include <GraphicsCore/Managers/ShadowPool.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

xiiClusteredDataGPU::xiiClusteredDataGPU()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  {
    xiiGALBufferCreationDescription bufferDescription;
    bufferDescription.m_Mode           = xiiGALBufferMode::Structured;
    bufferDescription.m_BindFlags      = xiiGALBindFlags::ShaderResource;
    bufferDescription.m_Usage          = xiiGALResourceUsage::Dynamic;
    bufferDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;

    {
      bufferDescription.m_uiElementByteStride = sizeof(xiiPerLightData);
      bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * xiiClusteredDataCPU::MAX_LIGHT_DATA;

      m_pLightDataBuffer = pDevice->CreateBuffer(bufferDescription);

      if (m_pLightDataBuffer)
      {
        m_pLightDataBuffer->SetDebugName("ClusteredDataProvider::LightDataBuffer");
      }
    }

    {
      bufferDescription.m_uiElementByteStride = sizeof(xiiPerDecalData);
      bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * xiiClusteredDataCPU::MAX_DECAL_DATA;

      m_pDecalDataBuffer = pDevice->CreateBuffer(bufferDescription);

      if (m_pDecalDataBuffer)
      {
        m_pDecalDataBuffer->SetDebugName("ClusteredDataProvider::DecalDataBuffer");
      }
    }

    {
      bufferDescription.m_uiElementByteStride = sizeof(xiiPerReflectionProbeData);
      bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * xiiClusteredDataCPU::MAX_REFLECTION_PROBE_DATA;

      m_pReflectionProbeDataBuffer = pDevice->CreateBuffer(bufferDescription);

      if (m_pReflectionProbeDataBuffer)
      {
        m_pReflectionProbeDataBuffer->SetDebugName("ClusteredDataProvider::ReflectionProbeDataBuffer");
      }
    }

    {
      bufferDescription.m_uiElementByteStride = sizeof(xiiPerClusterData);
      bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * NUM_CLUSTERS;

      m_pClusterDataBuffer = pDevice->CreateBuffer(bufferDescription);

      if (m_pClusterDataBuffer)
      {
        m_pClusterDataBuffer->SetDebugName("ClusteredDataProvider::ClusterDataBuffer");
      }
    }

    {
      bufferDescription.m_uiElementByteStride = sizeof(xiiUInt32);
      bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * xiiClusteredDataCPU::MAX_ITEMS_PER_CLUSTER * NUM_CLUSTERS;

      m_pClusterItemBuffer = pDevice->CreateBuffer(bufferDescription);

      if (m_pClusterItemBuffer)
      {
        m_pClusterItemBuffer->SetDebugName("ClusteredDataProvider::ClusterItemBuffer");
      }
    }
  }

  m_pClusterDataConstantBuffer = xiiGALDeviceUtilities::CreateConstantBuffer(pDevice, sizeof(xiiClusteredDataConstants), "xiiClusteredDataConstants");

  {
    xiiGALSamplerCreationDescription samplerDescription;
    samplerDescription.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_MinFilter          = xiiGALFilterType::ComparisonLinear;
    samplerDescription.m_MagFilter          = xiiGALFilterType::ComparisonLinear;
    samplerDescription.m_MipFilter          = xiiGALFilterType::ComparisonLinear;
    samplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Less;
    samplerDescription.m_BorderColor        = xiiColor::Black;
    samplerDescription.m_fMipLODBias        = 0.0f;
    samplerDescription.m_fMinLOD            = -1.0f;
    samplerDescription.m_fMaxLOD            = 42000.0f;
    samplerDescription.m_uiMaxAnisotropy    = 4U;

    m_pShadowSampler = pDevice->CreateSampler(samplerDescription);

    if (m_pShadowSampler)
    {
      m_pShadowSampler->SetDebugName("ClusteredDataProvider::ShadowSampler");
    }
  }

  m_hDecalAtlas = xiiDecalAtlasResource::GetDecalAtlasResource();

  {
    xiiGALSamplerCreationDescription samplerDescription;
    samplerDescription.m_AddressU           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_AddressV           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_AddressW           = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
    samplerDescription.m_MinFilter          = xiiGALFilterType::Linear;
    samplerDescription.m_MagFilter          = xiiGALFilterType::Linear;
    samplerDescription.m_MipFilter          = xiiGALFilterType::Linear;
    samplerDescription.m_ComparisonFunction = xiiGALComparisonFunction::Never;
    samplerDescription.m_BorderColor        = xiiColor::Black;
    samplerDescription.m_fMipLODBias        = 0.0f;
    samplerDescription.m_fMinLOD            = -1.0f;
    samplerDescription.m_fMaxLOD            = 42000.0f;
    samplerDescription.m_uiMaxAnisotropy    = 4U;

    xiiTextureUtils::ConfigureSampler(xiiTextureFilterSetting::DefaultQuality, samplerDescription);
    samplerDescription.m_uiMaxAnisotropy = xiiMath::Min(samplerDescription.m_uiMaxAnisotropy, 4u);

    m_pDecalAtlasSampler = pDevice->CreateSampler(samplerDescription);

    if (m_pDecalAtlasSampler)
    {
      m_pDecalAtlasSampler->SetDebugName("ClusteredDataProvider::DecalAtlasSampler");
    }
  }
}

xiiClusteredDataGPU::~xiiClusteredDataGPU()
{
  m_pLightDataBuffer.Clear();
  m_pDecalDataBuffer.Clear();
  m_pReflectionProbeDataBuffer.Clear();
  m_pClusterDataBuffer.Clear();
  m_pClusterItemBuffer.Clear();
  m_pShadowSampler.Clear();
  m_pDecalAtlasSampler.Clear();
  m_pClusterDataConstantBuffer.Borrow();
}

void xiiClusteredDataGPU::BindResources()
{
  // pRenderContext->BindBuffer("perLightDataBuffer", m_pLightDataBuffer);
  // pRenderContext->BindBuffer("perDecalDataBuffer", m_pDecalDataBuffer);
  // pRenderContext->BindBuffer("perPerReflectionProbeDataBuffer", m_pReflectionProbeDataBuffer);
  // pRenderContext->BindBuffer("perClusterDataBuffer", m_pClusterDataBuffer);
  // pRenderContext->BindBuffer("clusterItemBuffer", m_pClusterItemBuffer);

  // pRenderContext->BindBuffer("shadowDataBuffer", xiiShadowPool::GetShadowDataBuffer());
  // pRenderContext->BindTexture("ShadowAtlasTexture", xiiShadowPool::GetShadowAtlasTexture());
  // pRenderContext->BindSampler("ShadowSampler", m_pShadowSampler);

  // xiiResourceLock<xiiDecalAtlasResource> pDecalAtlas(m_hDecalAtlas, xiiResourceAcquireMode::AllowLoadingFallback);
  // pRenderContext->BindTexture2D("DecalAtlasBaseColorTexture", pDecalAtlas->GetBaseColorTexture());
  // pRenderContext->BindTexture2D("DecalAtlasNormalTexture", pDecalAtlas->GetNormalTexture());
  // pRenderContext->BindTexture2D("DecalAtlasORMTexture", pDecalAtlas->GetORMTexture());
  // pRenderContext->BindSampler("DecalAtlasSampler", m_pDecalAtlasSampler);

  // pRenderContext->BindTexture("ReflectionSpecularTexture", xiiReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_CameraUsageHint));
  // pRenderContext->BindTexture("SkyIrradianceTexture", xiiReflectionPool::GetSkyIrradianceTexture());

  // pRenderContext->BindConstantBuffer("xiiClusteredDataConstants", m_pClusterDataConstantBuffer);
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
    m_Data.m_CameraUsageHint      = pData->m_cameraUsageHint;

    xiiSharedPtr<xiiGALDevice>      pDevice      = xiiGALDevice::GetDefaultDevice();
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(xiiGALCommandListCreationDescription{.m_QueueFlags = xiiGALCommandQueueFlags::Graphics});
    XII_ASSERT_DEV(pCommandList != nullptr, "Failed to create command list!");

    pCommandList->Begin();
    {
      {
        xiiGALScopedDebugGroup scope(pCommandList, "xiiClusteredDataProvider::UpdateData");

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
      {
        xiiGALScopedDebugGroup scope(pCommandList, "xiiClusteredDataProvider::UpdateConstants");

        const xiiRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

        xiiGALMapHelper<xiiClusteredDataConstants> pConstants(pCommandList, m_Data.m_pClusterDataConstantBuffer, xiiGALMapType::Write, xiiGALMapFlags::Discard);

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
    }
    pCommandList->End();

    pDevice->GetCommandQueue()->Submit(std::move(pCommandList));
  }

  return &m_Data;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_ClusteredDataProvider);
