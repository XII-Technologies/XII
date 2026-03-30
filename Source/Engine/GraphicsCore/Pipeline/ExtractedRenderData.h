#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderData.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>

/// \brief Batch of extracted render data, intended to be populated by component managers sequentially per-batch.
struct XII_GRAPHICSCORE_DLL xiiRenderDataBatch
{
  xiiArrayPtr<xiiRenderData*> m_Data;
};

/// \brief A thread-safe structure for components to push their extracted render data batches into.
class XII_GRAPHICSCORE_DLL xiiExtractedRenderData
{
public:
  xiiExtractedRenderData();
  ~xiiExtractedRenderData();

  /// \brief Pushes a batch of extracted data safely to the internal list.
  void AddRenderDataBatch(xiiRenderData::Category category, const xiiRenderDataBatch& batch);

  /// \brief Clears the internal arrays entirely. Called at the start of extreme frame extraction.
  void Clear();

  /// \brief Returns all batches aggregated under the given category.
  xiiArrayPtr<const xiiRenderDataBatch> GetBatches(xiiRenderData::Category category) const;

  /// \brief Sorts the underlying render data by sorting key for cache-efficient render execution.
  void SortAndBatches();

private:
  xiiMutex                                             m_Mutex;
  xiiDynamicArray<xiiDynamicArray<xiiRenderDataBatch>> m_BatchesPerCategory;
};
