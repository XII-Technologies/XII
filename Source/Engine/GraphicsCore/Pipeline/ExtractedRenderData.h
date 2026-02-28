#pragma once

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Debug/DebugRendererContext.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/ViewData.h>

/// \brief Contains all render data extracted from a view for one frame.
///
/// During the extraction phase, render components add their render data to this container, organized by category (opaque, transparent, etc.).
/// The data is then sorted and batched for efficient rendering. Also stores camera data, view data, world time, and debug contexts.
class XII_GRAPHICSCORE_DLL xiiExtractedRenderData
{
public:
  xiiExtractedRenderData();
  ~xiiExtractedRenderData();

  XII_ALWAYS_INLINE void             SetCamera(const xiiCamera& camera) { m_Camera = camera; }
  XII_ALWAYS_INLINE const xiiCamera& GetCamera() const { return m_Camera; }

  XII_ALWAYS_INLINE void               SetViewData(const xiiViewData& viewData) { m_ViewData = viewData; }
  XII_ALWAYS_INLINE const xiiViewData& GetViewData() const { return m_ViewData; }

  XII_ALWAYS_INLINE void    SetWorldTime(xiiTime time) { m_WorldTime = time; }
  XII_ALWAYS_INLINE xiiTime GetWorldTime() const { return m_WorldTime; }

  XII_ALWAYS_INLINE void                           SetWorldDebugContext(const xiiDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  XII_ALWAYS_INLINE const xiiDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  XII_ALWAYS_INLINE void                           SetViewDebugContext(const xiiDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  XII_ALWAYS_INLINE const xiiDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  /// Adds render data for a specific rendering category.
  void AddRenderData(const xiiRenderData* pRenderData, xiiRenderData::Category category);

  /// Adds frame-level data that is not tied to a specific render category.
  void AddFrameData(const xiiRenderData* pFrameData);

  /// Sorts and batches all render data by category and sorting key for efficient rendering.
  void SortAndBatch();

  void Clear();

  /// Returns all render data batches for a specific category.
  xiiRenderDataBatchList GetRenderDataBatchesWithCategory(xiiRenderData::Category category) const;

  /// Returns raw unsorted render data for a specific category.
  xiiArrayPtr<const xiiRenderDataBatch::SortableRenderData> GetRawRenderDataWithCategory(xiiRenderData::Category category) const;

  template <typename T>
  XII_ALWAYS_INLINE const T* GetFrameData() const
  {
    return static_cast<const T*>(GetFrameData(xiiGetStaticRTTI<T>()));
  }

private:
  const xiiRenderData* GetFrameData(const xiiRTTI* pRtti) const;

  struct DataPerCategory
  {
    xiiDynamicArray<xiiRenderDataBatch>                     m_Batches;
    xiiDynamicArray<xiiRenderDataBatch::SortableRenderData> m_SortableRenderData;

    xiiDynamicArray<xiiInstanceableRenderData::DataOffsets> m_DataOffsets;
    xiiSharedPtr<xiiGALBuffer>                              m_pDataOffsetsBuffer;
  };

  void SortAndBatchCategory(DataPerCategory& dataPerCategory, xiiRenderData::Category category);

  xiiCamera   m_Camera;
  xiiViewData m_ViewData;
  xiiTime     m_WorldTime;

  xiiDebugRendererContext m_WorldDebugContext;
  xiiDebugRendererContext m_ViewDebugContext;

  xiiHybridArray<DataPerCategory, 32U>      m_DataPerCategory;
  xiiHybridArray<const xiiRenderData*, 16U> m_FrameData;
};
