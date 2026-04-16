#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

#include <Foundation/Communication/Message.h>

class xiiView;

/// \brief Sent to components or component managers to request them to extract their render data into the given structure.
struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

  const xiiView*          m_pView                = nullptr;
  xiiExtractedRenderData* m_pExtractedRenderData = nullptr;
  xiiRenderDataCategory   m_OverrideCategory     = xiiInvalidRenderDataCategory;

  XII_ALWAYS_INLINE void AddRenderData(xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never)
  {
    if (m_pExtractedRenderData == nullptr || pRenderData == nullptr)
      return;

    const xiiRenderDataCategory effectiveCategory = m_OverrideCategory.IsValid() ? m_OverrideCategory : category;
    m_pExtractedRenderData->AddRenderData(pRenderData, effectiveCategory, caching);
  }

  XII_ALWAYS_INLINE void AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never)
  {
    if (m_pExtractedRenderData == nullptr)
      return;

    const xiiRenderDataCategory effectiveCategory = m_OverrideCategory.IsValid() ? m_OverrideCategory : category;
    m_pExtractedRenderData->AddRenderDataBatch(effectiveCategory, batch, caching);
  }
};
