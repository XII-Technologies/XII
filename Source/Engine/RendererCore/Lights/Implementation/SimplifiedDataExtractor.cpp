#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimplifiedDataCPU, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSimplifiedDataCPU::xiiSimplifiedDataCPU()  = default;
xiiSimplifiedDataCPU::~xiiSimplifiedDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSimplifiedDataExtractor, 1, xiiRTTIDefaultAllocator<xiiSimplifiedDataExtractor>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSimplifiedDataExtractor::xiiSimplifiedDataExtractor(const char* szName) :
  xiiExtractor(szName)
{
  m_DependsOn.PushBack(xiiMakeHashedString("xiiVisibleObjectsExtractor"));
}

xiiSimplifiedDataExtractor::~xiiSimplifiedDataExtractor() {}

void xiiSimplifiedDataExtractor::PostSortAndBatch(
  const xiiView&                               view,
  const xiiDynamicArray<const xiiGameObject*>& visibleObjects,
  xiiExtractedRenderData&                      extractedRenderData)
{
  const xiiCamera* pCamera      = view.GetCullingCamera();
  const float      fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  xiiSimplifiedDataCPU* pData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), xiiSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint      = view.GetCameraUsageHint();

  extractedRenderData.AddFrameData(pData);
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);
