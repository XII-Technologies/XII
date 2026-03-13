#pragma once

#include <GraphicsCore/Pipeline/Extractor.h>

class XII_GRAPHICSCORE_DLL xiiGridRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGridRenderData, xiiRenderData);

public:
  xiiInt32 m_iAxis;
  xiiInt32 m_iMajorGridDiv;
  float    m_fGridScale;
  float    m_fAxisLineWidth;
  float    m_fMajorLineWidth;
  float    m_fMinorLineWidth;
  float    m_fAxisDashScale;
  xiiColor m_MajorLineColor;
  xiiColor m_MinorLineColor;
  xiiColor m_BaseColor;
  xiiColor m_XAxisColor;
  xiiColor m_XAxisDashColor;
  xiiColor m_YAxisColor;
  xiiColor m_YAxisDashColor;
  xiiColor m_ZAxisColor;
  xiiColor m_ZAxisDashColor;
  xiiColor m_CenterColor;
  bool     m_bOrthographicMode;
  bool     m_bGlobal;
};

class XII_GRAPHICSCORE_DLL xiiGridExtractor : public xiiExtractor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGridExtractor, xiiExtractor);

public:
  xiiGridExtractor(xiiStringView sName = "GridExtractor");

  virtual void Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData) override {}
};
