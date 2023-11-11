#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GraphicsCore/Lights/Implementation/ClusteredDataUtils.h>
#include <GraphicsCore/Lights/SimplifiedDataExtractor.h>
#include <GraphicsCore/Pipeline/View.h>

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

xiiSimplifiedDataExtractor::~xiiSimplifiedDataExtractor() = default;

void xiiSimplifiedDataExtractor::PostSortAndBatch(
  const xiiView&                               view,
  const xiiDynamicArray<const xiiGameObject*>& visibleObjects,
  xiiExtractedRenderData&                      ref_extractedRenderData)
{
  xiiSimplifiedDataCPU* pData = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), xiiSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint      = view.GetCameraUsageHint();

  ref_extractedRenderData.AddFrameData(pData);
}

xiiResult xiiSimplifiedDataExtractor::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return XII_SUCCESS;
}

xiiResult xiiSimplifiedDataExtractor::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Lights_Implementation_SimplifiedDataExtractor);
