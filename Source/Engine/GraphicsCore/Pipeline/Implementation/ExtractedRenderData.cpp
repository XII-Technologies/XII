/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Threading/TaskSystem.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/View.h>

#include <algorithm>

xiiExtractedRenderData::xiiExtractedRenderData() = default;

xiiExtractedRenderData::xiiExtractedRenderData(const xiiWorld* pWorld) :
  m_WorldDebugContext(pWorld)
{
}

xiiExtractedRenderData::xiiExtractedRenderData(const xiiView& view) :
  m_WorldDebugContext(view.GetWorld()), m_ViewDebugContext(view)
{
}

xiiExtractedRenderData::~xiiExtractedRenderData() = default;

void xiiExtractedRenderData::AddRenderDataInternal(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  if (pRenderData == nullptr)
  {
    return;
  }

  if (caching == xiiRenderData::Caching::IfStatic)
  {
    m_StaticRenderData.PushBack(pRenderData);
  }
  else
  {
    m_DynamicRenderData.PushBack(pRenderData);
  }
}

void xiiExtractedRenderData::AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);

  AddRenderDataInternal(pRenderData, caching);
}

void xiiExtractedRenderData::AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching)
{
  XII_LOCK(m_Mutex);

  for (xiiRenderData* pRenderData : batch.m_Data)
  {
    AddRenderDataInternal(pRenderData, caching);
  }
}

void xiiExtractedRenderData::Clear()
{
  XII_LOCK(m_Mutex);

  m_StaticRenderData.Clear();
  m_DynamicRenderData.Clear();
  m_uiPublishedStaticCount  = 0U;
  m_uiPublishedDynamicCount = 0U;
  m_RenderDataRanges.Clear();
  m_SortedAllRenderData.Clear();
}

xiiArrayPtr<const xiiExtractedRenderData::RenderDataRange> xiiExtractedRenderData::GetRenderDataRanges(const xiiRTTI* pType) const
{
  const auto* pRanges = m_RenderDataRanges.GetValue(pType);
  return pRanges != nullptr ? pRanges->GetArrayPtr() : xiiArrayPtr<const RenderDataRange>();
}

void xiiExtractedRenderData::SortAndBatches()
{
  XII_LOCK(m_Mutex);

  auto sortPartition = [](xiiDynamicArray<xiiRenderData*>* pData) {
    if (pData->GetCount() < 2U)
      return;

    xiiDynamicArray<xiiRenderData*> scratch;
    scratch.SetCountUninitialized(pData->GetCount());
    xiiSorting::RadixSort(*pData, scratch, [](const xiiRenderData* pRenderData) -> xiiUInt64 {
      return pRenderData->m_uiSortingKey;
    });
  };

  if (m_StaticRenderData.GetCount() >= 1024U && m_DynamicRenderData.GetCount() >= 1024U)
  {
    xiiDynamicArray<xiiRenderData*>* partitions[] = {&m_StaticRenderData, &m_DynamicRenderData};
    xiiTaskSystem::ParallelForSingle(xiiMakeArrayPtr(partitions), sortPartition, "Sort extracted render data");
  }
  else
  {
    sortPartition(&m_StaticRenderData);
    sortPartition(&m_DynamicRenderData);
  }

  m_uiPublishedStaticCount  = m_StaticRenderData.GetCount();
  m_uiPublishedDynamicCount = m_DynamicRenderData.GetCount();
  m_RenderDataRanges.Clear();
  if (m_StaticRenderData.IsEmpty())
    m_SortedAllRenderData = m_DynamicRenderData;
  else if (m_DynamicRenderData.IsEmpty())
    m_SortedAllRenderData = m_StaticRenderData;
  else
  {
    m_SortedAllRenderData.SetCountUninitialized(m_uiPublishedStaticCount + m_uiPublishedDynamicCount);
    std::merge(begin(m_StaticRenderData), end(m_StaticRenderData), begin(m_DynamicRenderData), end(m_DynamicRenderData), begin(m_SortedAllRenderData), [](const xiiRenderData* pLeft, const xiiRenderData* pRight) {
      return pLeft->m_uiSortingKey < pRight->m_uiSortingKey;
    });
  }

  const xiiRTTI*        pRenderDataType = xiiGetStaticRTTI<xiiRenderData>();
  const RenderDataRange all             = m_SortedAllRenderData;
  for (xiiUInt32 uiStart = 0U; uiStart < all.GetCount();)
  {
    const xiiRTTI* pType = all[uiStart]->GetDynamicRTTI();
    xiiUInt32      uiEnd = uiStart + 1U;
    while (uiEnd < all.GetCount() && all[uiEnd]->GetDynamicRTTI() == pType)
      ++uiEnd;

    for (const xiiRTTI* pBase = pType; pBase != nullptr; pBase = pBase->GetParentType())
    {
      auto& ranges = m_RenderDataRanges[pBase];
      if (!ranges.IsEmpty() && ranges.PeekBack().GetEndPtr() == all.GetPtr() + uiStart)
      {
        auto& previous = ranges.PeekBack();
        previous       = xiiMakeArrayPtr(previous.GetPtr(), previous.GetCount() + uiEnd - uiStart);
      }
      else
      {
        ranges.PushBack(all.GetSubArray(uiStart, uiEnd - uiStart));
      }
      if (pBase == pRenderDataType)
        break;
    }
    uiStart = uiEnd;
  }
}
