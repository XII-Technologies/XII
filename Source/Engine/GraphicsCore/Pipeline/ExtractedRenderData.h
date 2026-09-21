/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Debug/DebugRendererContext.h>
#include <GraphicsCore/Pipeline/RenderData.h>

/// Batch of extracted render data, intended to be populated by component managers sequentially per-batch.
struct XII_GRAPHICSCORE_DLL xiiRenderDataBatch
{
  xiiArrayPtr<xiiRenderData*> m_Data;

  XII_ALWAYS_INLINE xiiUInt32 GetCount() const
  {
    return m_Data.GetCount();
  }

  template <typename T>
  XII_ALWAYS_INLINE const T* GetFirstData() const
  {
    const xiiRTTI* pType = xiiGetStaticRTTI<T>();
    for (xiiRenderData* pRenderData : m_Data)
    {
      if (pRenderData != nullptr && pRenderData->GetDynamicRTTI() != nullptr && pRenderData->GetDynamicRTTI()->IsDerivedFrom(pType))
      {
        return static_cast<const T*>(pRenderData);
      }
    }

    return nullptr;
  }

  template <typename T>
  class Iterator
  {
  public:
    Iterator(xiiArrayPtr<xiiRenderData* const> data, xiiUInt32 uiStartIndex, xiiUInt32 uiCount) :
      m_Data(data)
    {
      const xiiUInt32 uiDataCount = m_Data.GetCount();
      m_uiCurrentIndex            = xiiMath::Min(uiStartIndex, uiDataCount);

      const xiiUInt32 uiRemaining = uiDataCount - m_uiCurrentIndex;
      if (uiCount >= uiRemaining)
      {
        m_uiEndIndex = uiDataCount;
      }
      else
      {
        m_uiEndIndex = m_uiCurrentIndex + uiCount;
      }

      FindNextValid();
    }

    XII_ALWAYS_INLINE bool IsValid() const { return m_uiCurrentIndex < m_uiEndIndex; }

    XII_ALWAYS_INLINE void operator++()
    {
      ++m_uiCurrentIndex;
      FindNextValid();
    }

    XII_ALWAYS_INLINE operator const T*() const
    {
      return static_cast<const T*>(m_Data[m_uiCurrentIndex]);
    }

    XII_ALWAYS_INLINE const T& operator*() const
    {
      return *static_cast<const T*>(m_Data[m_uiCurrentIndex]);
    }

    XII_ALWAYS_INLINE const T* operator->() const
    {
      return static_cast<const T*>(m_Data[m_uiCurrentIndex]);
    }

  private:
    XII_ALWAYS_INLINE void FindNextValid()
    {
      const xiiRTTI* pType = xiiGetStaticRTTI<T>();

      while (m_uiCurrentIndex < m_uiEndIndex)
      {
        xiiRenderData* pRenderData = m_Data[m_uiCurrentIndex];
        if (pRenderData != nullptr && pRenderData->GetDynamicRTTI() != nullptr && pRenderData->GetDynamicRTTI()->IsDerivedFrom(pType))
        {
          return;
        }

        ++m_uiCurrentIndex;
      }
    }

    xiiArrayPtr<xiiRenderData* const> m_Data;
    xiiUInt32                         m_uiCurrentIndex = 0;
    xiiUInt32                         m_uiEndIndex     = 0;
  };

  template <typename T>
  XII_ALWAYS_INLINE Iterator<T> GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiMath::MaxValue<xiiUInt32>()) const
  {
    return Iterator<T>(m_Data, uiStartIndex, uiCount);
  }
};

/// A thread-safe structure for components to push their extracted render data batches into.
class XII_GRAPHICSCORE_DLL xiiExtractedRenderData
{
public:
  /// Returns all extracted render data, sorted by sorting key.
  XII_ALWAYS_INLINE xiiArrayPtr<xiiRenderData* const> GetAllRenderData() const { return m_SortedAllRenderData; }

  /// Returns extracted render data marked static during extraction, sorted by sorting key.
  XII_ALWAYS_INLINE xiiArrayPtr<xiiRenderData* const> GetStaticRenderData() const { return m_SortedStaticRenderData; }

  /// Returns extracted render data marked dynamic during extraction, sorted by sorting key.
  XII_ALWAYS_INLINE xiiArrayPtr<xiiRenderData* const> GetDynamicRenderData() const { return m_SortedDynamicRenderData; }

  /// Returns a debug context that can be used for rendering debug visualization related to the world in which the data was extracted. The geometry rendered in this context is rendered in all views for that scene.
  XII_ALWAYS_INLINE const xiiDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  /// Returns a debug context that can be used for rendering debug visualization related to the view for which the data was extracted. The geometry rendered in this context is only rendered in this view.
  XII_ALWAYS_INLINE const xiiDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

public:
  xiiExtractedRenderData();

  /// Initializes the extracted render data with a world debug context. The geometry rendered in this context is rendered in all views for that scene.
  xiiExtractedRenderData(const xiiWorld* pWorld);

  /// Initializes the extracted render data with a view debug context. The geometry rendered in this context is only rendered in this view.
  xiiExtractedRenderData(const xiiViewHandle& hView);

  /// Initializes the extracted render data with both a world and view debug context. The geometry rendered in the world context is rendered in all views for that scene, while the geometry rendered in the view context is only rendered in this view.
  xiiExtractedRenderData(const xiiWorld* pWorld, const xiiViewHandle& hView);

  ~xiiExtractedRenderData();

  /// Adds a single extracted render data item without assigning a category.
  void AddRenderData(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// Pushes a batch of extracted data safely to the internal list without assigning a category.
  void AddRenderDataBatch(const xiiRenderDataBatch& batch, xiiRenderData::Caching::Enum caching = xiiRenderData::Caching::Never);

  /// Clears the internal arrays entirely. Called at the start of extreme frame extraction.
  void Clear();

  /// Sorts the underlying render data by sorting key for cache-efficient render execution.
  void SortAndBatches();

private:
  friend class xiiRenderWorldModule;

  void AddRenderDataInternal(xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching);

  xiiMutex m_Mutex;

  // Render data submitted concurrently in the current extraction.
  xiiDynamicArray<xiiRenderData*> m_SubmittedStaticRenderData;
  xiiDynamicArray<xiiRenderData*> m_SubmittedDynamicRenderData;

  // Flattened and sorted arrays built during SortAndBatches.
  xiiDynamicArray<xiiRenderData*> m_SortedStaticRenderData;
  xiiDynamicArray<xiiRenderData*> m_SortedDynamicRenderData;
  xiiDynamicArray<xiiRenderData*> m_SortedAllRenderData;

  xiiDebugRendererContext m_WorldDebugContext;
  xiiDebugRendererContext m_ViewDebugContext;
};
