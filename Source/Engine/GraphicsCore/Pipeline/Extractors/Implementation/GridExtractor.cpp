#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Extractors/GridExtractor.h>
#include <GraphicsCore/Pipeline/View.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridRenderData, 1, xiiRTTIDefaultAllocator<xiiGridRenderData>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridExtractor, 1, xiiRTTIDefaultAllocator<xiiGridExtractor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGridExtractor::xiiGridExtractor(xiiStringView sName) :
  xiiExtractor(sName)
{
}

void xiiGridExtractor::Extract(const xiiView& view, const xiiDynamicArray<const xiiGameObject*>& visibleObjects, xiiExtractedRenderData& ref_extractedRenderData)
{
  XII_IGNORE_UNUSED(visibleObjects);

  const xiiCamera*   pCamera       = view.GetCamera();
  xiiGridRenderData* pRenderData   = xiiCreateRenderDataForThisFrame<xiiGridRenderData>(nullptr);
  pRenderData->m_GlobalBounds      = xiiBoundingBoxSphere::MakeInvalid();
  pRenderData->m_bOrthographicMode = pCamera->IsOrthographic();

  if (pCamera->IsOrthographic())
  {
    pRenderData->m_fGridScale    = 1.0f;
    pRenderData->m_iMajorGridDiv = 10;
  }
  else
  {
    pRenderData->m_fGridScale    = 10.0f;
    pRenderData->m_iMajorGridDiv = 10;
  }

  ref_extractedRenderData.AddRenderData(pRenderData, xiiDefaultRenderDataCategories::SimpleTransparent);
}
