#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

#include <Foundation/Communication/Message.h>

class xiiView;

/// \brief Sent to components or component managers to request them to extract their render data into the given structure.
struct XII_GRAPHICSCORE_DLL xiiMsgExtractRenderData : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgExtractRenderData, xiiMessage);

  using SubmitRenderDataFunction = void (*)(void* pContext, const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching);

  const xiiView*          m_pView                = nullptr;
  xiiExtractedRenderData* m_pExtractedRenderData = nullptr;
  xiiRenderDataCategory   m_OverrideCategory     = xiiInvalidRenderDataCategory;

  SubmitRenderDataFunction m_SubmitRenderDataFunction = nullptr;
  void*                    m_pSubmitRenderDataContext = nullptr;

  xiiGameObjectHandle m_hCurrentObject;
  xiiComponentHandle  m_hCurrentComponent;
  xiiUInt32           m_uiViewIndex = xiiInvalidIndex;

  XII_ALWAYS_INLINE void AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never)
  {
    if (m_pExtractedRenderData == nullptr || pRenderData == nullptr)
      return;

    if (m_OverrideCategory.IsValid())
    {
      pRenderData->m_RoutingFlags = xiiRenderData::RoutingFlagsFromLegacyCategory(m_OverrideCategory);
    }

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
      m_SubmitRenderDataFunction(m_pSubmitRenderDataContext, *this, pRenderData, xiiInvalidRenderDataCategory, caching);
      return;
    }

    m_pExtractedRenderData->AddRenderData(pRenderData, caching);
  }

  XII_ALWAYS_INLINE void AddRenderData(xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never)
  {
    if (m_pExtractedRenderData == nullptr || pRenderData == nullptr)
      return;

    const xiiRenderDataCategory effectiveCategory = m_OverrideCategory.IsValid() ? m_OverrideCategory : category;

    pRenderData->m_RoutingFlags = xiiRenderData::RoutingFlagsFromLegacyCategory(effectiveCategory);

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
      m_SubmitRenderDataFunction(m_pSubmitRenderDataContext, *this, pRenderData, xiiInvalidRenderDataCategory, caching);
      return;
    }

    m_pExtractedRenderData->AddRenderData(pRenderData, caching);
  }

  XII_ALWAYS_INLINE void AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never)
  {
    if (m_pExtractedRenderData == nullptr)
      return;

    for (xiiRenderData* pRenderData : batch.m_Data)
    {
      AddRenderData(pRenderData, caching);
    }
  }

  XII_ALWAYS_INLINE void AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never)
  {
    if (m_pExtractedRenderData == nullptr)
      return;

    for (xiiRenderData* pRenderData : batch.m_Data)
    {
      AddRenderData(pRenderData, category, caching);
    }
  }
};
