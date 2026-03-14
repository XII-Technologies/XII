#pragma once

#include <GraphicsCore/Pipeline/Extractor.h>

class XII_GRAPHICSCORE_DLL xiiVisibleObjectsExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisibleObjectsExtractor, xiiExtractor);

public:
  xiiVisibleObjectsExtractor(xiiStringView sName = "VisibleObjectsExtractor");
  ~xiiVisibleObjectsExtractor();

  virtual void Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override {}
};
