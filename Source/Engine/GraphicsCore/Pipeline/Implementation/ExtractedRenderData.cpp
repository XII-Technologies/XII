#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

xiiExtractedRenderData::xiiExtractedRenderData() = default;

void xiiExtractedRenderData::AddRenderData(const xiiRenderData* pRenderData, xiiRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData          = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData  = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetFinalSortingKey(category, m_Camera);
}

void xiiExtractedRenderData::AddFrameData(const xiiRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void xiiExtractedRenderData::SortAndBatch()
{
  XII_PROFILE_SCOPE("SortAndBatch");

  struct RenderDataComparer
  {
    XII_FORCE_INLINE bool Less(const xiiRenderDataBatch::SortableRenderData& a, const xiiRenderDataBatch::SortableRenderData& b) const
    {
      if (a.m_uiSortingKey != b.m_uiSortingKey)
      {
        return a.m_uiSortingKey < b.m_uiSortingKey;
      }
      return a.m_pRenderData->m_hOwner < b.m_pRenderData->m_hOwner;
    }
  };

  for (auto& dataPerCategory : m_DataPerCategory)
  {
    if (dataPerCategory.m_SortableRenderData.IsEmpty())
      continue;

    auto& data = dataPerCategory.m_SortableRenderData;

    // Sort
    data.Sort(RenderDataComparer());

    // Find batches
    const xiiRenderData* pCurrentBatchRenderData  = data[0].m_pRenderData;
    const xiiRTTI*       pCurrentBatchType        = pCurrentBatchRenderData->GetDynamicRTTI();
    xiiUInt32            uiCurrentBatchStartIndex = 0;

    for (xiiUInt32 i = 1; i < data.GetCount(); ++i)
    {
      auto pRenderData = data[i].m_pRenderData;

      if (pRenderData->GetDynamicRTTI() != pCurrentBatchType || pRenderData->CanBatch(*pCurrentBatchRenderData) == false)
      {
        dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = xiiMakeArrayPtr(&data[uiCurrentBatchStartIndex], i - uiCurrentBatchStartIndex);

        pCurrentBatchRenderData  = pRenderData;
        pCurrentBatchType        = pRenderData->GetDynamicRTTI();
        uiCurrentBatchStartIndex = i;
      }
    }

    dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = xiiMakeArrayPtr(&data[uiCurrentBatchStartIndex], data.GetCount() - uiCurrentBatchStartIndex);
  }
}

void xiiExtractedRenderData::Clear()
{
  for (auto& dataPerCategory : m_DataPerCategory)
  {
    dataPerCategory.m_Batches.Clear();
    dataPerCategory.m_SortableRenderData.Clear();
  }

  m_FrameData.Clear();
}

xiiRenderDataBatchList xiiExtractedRenderData::GetRenderDataBatchesWithCategory(xiiRenderData::Category category, xiiRenderDataBatch::Filter filter) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    xiiRenderDataBatchList list;
    list.m_Batches = m_DataPerCategory[category.m_uiValue].m_Batches;
    list.m_Filter  = filter;

    return list;
  }

  return xiiRenderDataBatchList();
}

const xiiRenderData* xiiExtractedRenderData::GetFrameData(const xiiRTTI* pRtti) const
{
  for (auto pData : m_FrameData)
  {
    if (pData->IsInstanceOf(pRtti))
    {
      return pData;
    }
  }
  return nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_ExtractedRenderData);
