#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/ExtractedRenderData.h>

xiiExtractedRenderData::xiiExtractedRenderData() = default;

xiiExtractedRenderData::~xiiExtractedRenderData() = default;

void xiiExtractedRenderData::AddRenderDataInternal(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching)
{
  if (pRenderData == nullptr)
  {
    return;
  }

  if (caching == xiiRenderData::Caching::IfStatic)
  {
    m_SubmittedStaticRenderData.PushBack(pRenderData);
  }
  else
  {
    m_SubmittedDynamicRenderData.PushBack(pRenderData);
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

  m_SubmittedStaticRenderData.Clear();
  m_SubmittedDynamicRenderData.Clear();

  m_SortedStaticRenderData.Clear();
  m_SortedDynamicRenderData.Clear();
  m_SortedAllRenderData.Clear();
}

void xiiExtractedRenderData::SortAndBatches()
{
  XII_LOCK(m_Mutex);

  xiiDynamicArray<xiiRenderData*> sortScratchBuffer;

  auto SortByKey = [&sortScratchBuffer](xiiDynamicArray<xiiRenderData*>& data) {
    if (data.IsEmpty())
      return;

    sortScratchBuffer.SetCountUninitialized(data.GetCount());

    xiiArrayPtr<xiiRenderData*> pData = data;
    xiiSorting::RadixSort(data, sortScratchBuffer, [](const xiiRenderData* pRenderData) -> xiiUInt64 {
      return pRenderData->m_uiSortingKey;
    });
  };

  m_SortedStaticRenderData  = m_SubmittedStaticRenderData;
  m_SortedDynamicRenderData = m_SubmittedDynamicRenderData;

  SortByKey(m_SortedStaticRenderData);
  SortByKey(m_SortedDynamicRenderData);

  m_SortedAllRenderData = m_SortedStaticRenderData;
  m_SortedAllRenderData.PushBackRange(m_SortedDynamicRenderData);
  SortByKey(m_SortedAllRenderData);
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetAllRenderData() const
{
  return m_SortedAllRenderData;
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetStaticRenderData() const
{
  return m_SortedStaticRenderData;
}

xiiArrayPtr<xiiRenderData* const> xiiExtractedRenderData::GetDynamicRenderData() const
{
  return m_SortedDynamicRenderData;
}
