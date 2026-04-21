#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractRenderData);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractRenderData, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiMsgExtractRenderData::AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching /*= xiiRenderData::Caching::Never*/)
{
  if (m_pExtractedRenderData == nullptr || pRenderData == nullptr)
    return;

  if (!m_hCurrentObject.IsInvalidated())
  {
    pRenderData->m_hOwnerObject = m_hCurrentObject;
  }

  if (!m_hCurrentComponent.IsInvalidated())
  {
    pRenderData->m_hOwnerComponent = m_hCurrentComponent;
  }

  if (m_SubmitRenderDataFunction != nullptr)
  {
    m_SubmitRenderDataFunction(m_pSubmitRenderDataContext, *this, pRenderData, caching);
    return;
  }

  m_pExtractedRenderData->AddRenderData(pRenderData, caching);
}

void xiiMsgExtractRenderData::AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching /*= xiiRenderData::Caching::Never*/)
{
  if (m_pExtractedRenderData == nullptr)
    return;

  for (xiiRenderData* pRenderData : batch.m_Data)
  {
    AddRenderData(pRenderData, caching);
  }
}
