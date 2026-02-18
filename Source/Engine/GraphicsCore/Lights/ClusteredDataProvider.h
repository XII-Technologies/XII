#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/FrameDataProvider.h>

struct XII_GRAPHICSCORE_DLL xiiClusteredDataGPU
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiClusteredDataGPU);

public:
  xiiClusteredDataGPU();
  ~xiiClusteredDataGPU();

  xiiUInt32                   m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint> m_CameraUsageHint      = xiiCameraUsageHint::Default;

  xiiSharedPtr<xiiGALBuffer> m_pLightDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDecalDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pReflectionProbeDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pClusterDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pClusterItemBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pClusterDataConstantBuffer;

  xiiSharedPtr<xiiGALSampler> m_pShadowSampler;

  xiiDecalAtlasResourceHandle m_hDecalAtlas;
  xiiSharedPtr<xiiGALSampler> m_pDecalAtlasSampler;

  void BindResources();
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
