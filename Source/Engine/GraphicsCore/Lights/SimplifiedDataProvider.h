#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/FrameDataProvider.h>
#include <GraphicsCore/Shader/ConstantBufferStorage.h>

struct XII_RENDERERCORE_DLL xiiSimplifiedDataGPU
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSimplifiedDataGPU);

public:
  xiiSimplifiedDataGPU();
  ~xiiSimplifiedDataGPU();

  xiiUInt32                      m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint>    m_cameraUsageHint      = xiiCameraUsageHint::Default;
  xiiConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(xiiRenderContext* pRenderContext);
};

class XII_RENDERERCORE_DLL xiiSimplifiedDataProvider : public xiiFrameDataProvider<xiiSimplifiedDataGPU>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimplifiedDataProvider, xiiFrameDataProviderBase);

public:
  xiiSimplifiedDataProvider();
  ~xiiSimplifiedDataProvider();

private:
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, const xiiExtractedRenderData& extractedData) override;

  xiiSimplifiedDataGPU m_Data;
};
