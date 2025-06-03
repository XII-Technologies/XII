#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/FrameDataProvider.h>

struct XII_GRAPHICSCORE_DLL xiiSimplifiedDataGPU
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSimplifiedDataGPU);

public:
  xiiSimplifiedDataGPU();
  ~xiiSimplifiedDataGPU();

  xiiUInt32                      m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint>    m_cameraUsageHint      = xiiCameraUsageHint::Default;

  xiiSharedPtr<xiiGALBuffer>     m_pSimplifiedDataConstantBuffer;

  void BindResources(xiiSharedPtr<xiiGALCommandList> pCommandList);
};

class XII_GRAPHICSCORE_DLL xiiSimplifiedDataProvider : public xiiFrameDataProvider<xiiSimplifiedDataGPU>
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimplifiedDataProvider, xiiFrameDataProviderBase);

public:
  xiiSimplifiedDataProvider();
  ~xiiSimplifiedDataProvider();

private:
  virtual void* UpdateData(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiExtractedRenderData& extractedData) override;

  xiiSimplifiedDataGPU m_Data;
};
