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
  void AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch);

  /// \brief Clears the internal arrays entirely. Called at the start of extreme frame extraction.
  void Clear();

  /// \brief Sorts the underlying render data by sorting key for cache-efficient render execution.
  void SortAndBatches();

  /// \brief Returns the flattened and sorted render data for the given category.
  xiiArrayPtr<xiiRenderData* const> GetRenderData(xiiRenderDataCategory category) const;

private:
  xiiMutex m_Mutex;

  // Batches submitted concurrently
  xiiDynamicArray<xiiDynamicArray<xiiRenderDataBatch>> m_BatchesPerCategory;

  // Flattened and sorted array per category, built during SortAndBatches
  xiiDynamicArray<xiiDynamicArray<xiiRenderData*>> m_SortedRenderData;
};
