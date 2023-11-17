#pragma once

#include <GraphicsCore/Pipeline/Extractor.h>

class xiiSimplifiedDataCPU : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimplifiedDataCPU, xiiRenderData);

public:
  xiiSimplifiedDataCPU();
  ~xiiSimplifiedDataCPU();

  xiiUInt32                   m_uiSkyIrradianceIndex = 0;
  xiiEnum<xiiCameraUsageHint> m_cameraUsageHint      = xiiCameraUsageHint::Default;
};

class XII_GRAPHICSCORE_DLL xiiSimplifiedDataExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSimplifiedDataExtractor, xiiExtractor);

public:
  xiiSimplifiedDataExtractor(const char* szName = "SimplifiedDataExtractor");
  ~xiiSimplifiedDataExtractor();

  virtual void PostSortAndBatch(
    const xiiView&                               view,
    const xiiDynamicArray<const xiiGameObject*>& visibleObjects,
    xiiExtractedRenderData&                      ref_extractedRenderData) override;
  virtual xiiResult Serialize(xiiStreamWriter& inout_stream) const override;
  virtual xiiResult Deserialize(xiiStreamReader& inout_stream) override;
};
