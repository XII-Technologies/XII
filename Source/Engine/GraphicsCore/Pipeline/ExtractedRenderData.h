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

  /// \brief Adds a single extracted render data item without assigning a category.
  void AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// \brief Adds a single extracted render data item.
  void AddRenderData(xiiRenderData* pRenderData, xiiRenderDataCategory category, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// \brief Pushes a batch of extracted data safely to the internal list without assigning a category.
  void AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// \brief Pushes a batch of extracted data safely to the internal list.
  void AddRenderDataBatch(xiiRenderDataCategory category, const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// \brief Clears the internal arrays entirely. Called at the start of extreme frame extraction.
  void Clear();

  /// \brief Sorts the underlying render data by sorting key for cache-efficient render execution.
  void SortAndBatches();

  /// \brief Returns all extracted render data, sorted by sorting key.
  xiiArrayPtr<xiiRenderData* const> GetAllRenderData() const;

  /// \brief Returns extracted render data marked static during extraction, sorted by sorting key.
  xiiArrayPtr<xiiRenderData* const> GetStaticRenderData() const;

  /// \brief Returns extracted render data marked dynamic during extraction, sorted by sorting key.
  xiiArrayPtr<xiiRenderData* const> GetDynamicRenderData() const;

private:
  void AddRenderDataInternal(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching);

  xiiMutex m_Mutex;

  // Render data submitted concurrently in the current extraction.
  xiiDynamicArray<xiiRenderData*> m_SubmittedStaticRenderData;
  xiiDynamicArray<xiiRenderData*> m_SubmittedDynamicRenderData;

  // Flattened and sorted arrays built during SortAndBatches.
  xiiDynamicArray<xiiRenderData*> m_SortedStaticRenderData;
  xiiDynamicArray<xiiRenderData*> m_SortedDynamicRenderData;
  xiiDynamicArray<xiiRenderData*> m_SortedAllRenderData;
};
