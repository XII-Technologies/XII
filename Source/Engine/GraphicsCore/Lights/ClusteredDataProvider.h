#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/FrameDataProvider.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>

struct XII_GRAPHICSCORE_DLL xiiClusteredDataGPU
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiClusteredDataGPU);

public:
  xiiClusteredDataGPU();
  ~xiiClusteredDataGPU();

  xiiUInt32                   m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint> m_cameraUsageHint      = xiiCameraUsageHint::Default;

  xiiGALBufferHandle m_hLightDataBuffer;
  xiiGALBufferHandle m_hDecalDataBuffer;
  xiiGALBufferHandle m_hReflectionProbeDataBuffer;
  xiiGALBufferHandle m_hClusterDataBuffer;
  xiiGALBufferHandle m_hClusterItemBuffer;

  xiiConstantBufferStorageHandle m_hConstantBuffer;

  xiiGALSamplerStateHandle m_hShadowSampler;

  xiiDecalAtlasResourceHandle m_hDecalAtlas;
  xiiGALSamplerStateHandle    m_hDecalAtlasSampler;

  void BindResources(xiiRenderContext* pRenderContext);
};

class XII_GRAPHICSCORE_DLL xiiClusteredDataProvider : public xiiFrameDataProvider<xiiClusteredDataGPU>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClusteredDataProvider, xiiFrameDataProviderBase);

public:
  xiiClusteredDataProvider();
  ~xiiClusteredDataProvider();

private:
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) override;

  xiiClusteredDataGPU m_Data;
};
