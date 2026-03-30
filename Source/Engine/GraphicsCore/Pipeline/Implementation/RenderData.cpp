#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Pipeline/MsgExtractRenderData.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Lock.h>

// RenderData

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;


// ExtractedRenderData

xiiExtractedRenderData::xiiExtractedRenderData()
{
  m_BatchesPerCategory.SetCount((xiiUInt32)xiiRenderData::Category::ENUM_COUNT);
}

xiiExtractedRenderData::~xiiExtractedRenderData() = default;

void xiiExtractedRenderData::AddRenderDataBatch(xiiRenderData::Category category, const xiiRenderDataBatch& batch)
{
  if (batch.m_Data.IsEmpty())
    return;

  xiiLock<xiiMutex> lock(m_Mutex);
  m_BatchesPerCategory[(xiiUInt32)category].PushBack(batch);
}

void xiiExtractedRenderData::Clear()
{
  for (auto& batches : m_BatchesPerCategory)
  {
    batches.Clear();
  }
}

xiiArrayPtr<const xiiRenderDataBatch> xiiExtractedRenderData::GetBatches(xiiRenderData::Category category) const
{
  return m_BatchesPerCategory[(xiiUInt32)category].GetArrayPtr();
}

void xiiExtractedRenderData::SortAndBatches()
{
  // For sorting, we either sort in-place per batch or we flatten, sort, and re-batch here.
  // In a robust implementation, usually a linear array of pointers is gathered, sorted, and that is what's iterated.
  // For simplicity right now, we assume batch sorting logic happens either by passes or here via simple sorting.
  // We'll leave the implementation mostly as a placeholder or perform a simple sort if we flatten.
}


// MsgExtractRenderData

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgExtractRenderData);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgExtractRenderData, 1, xiiRTTIDefaultAllocator<xiiMsgExtractRenderData>)
{
  XII_BEGIN_PROPERTIES
  {
    // Normally messages don't deeply reflect their payload for serialization unless needed, this is runtime.
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
