#pragma once

#include <Core/Graphics/Camera.h>
#include <GraphicsCore/Debug/DebugRendererContext.h>
#include <GraphicsCore/Pipeline/RenderData.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/ViewData.h>

class XII_GRAPHICSCORE_DLL xiiExtractedRenderData
{
public:
  xiiExtractedRenderData();

  XII_ALWAYS_INLINE void  SetCamera(const xiiCamera& camera) { m_Camera = camera; }
  XII_ALWAYS_INLINE const xiiCamera& GetCamera() const { return m_Camera; }

  XII_ALWAYS_INLINE void  SetLodCamera(const xiiCamera& camera) { m_LodCamera = camera; }
  XII_ALWAYS_INLINE const xiiCamera& GetLodCamera() const { return m_LodCamera; }

  XII_ALWAYS_INLINE void  SetViewData(const xiiViewData& viewData) { m_ViewData = viewData; }
  XII_ALWAYS_INLINE const xiiViewData& GetViewData() const { return m_ViewData; }

  XII_ALWAYS_INLINE void    SetWorldTime(xiiTime time) { m_WorldTime = time; }
  XII_ALWAYS_INLINE xiiTime GetWorldTime() const { return m_WorldTime; }

  XII_ALWAYS_INLINE void  SetWorldDebugContext(const xiiDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  XII_ALWAYS_INLINE const xiiDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  XII_ALWAYS_INLINE void  SetViewDebugContext(const xiiDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  XII_ALWAYS_INLINE const xiiDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  void AddRenderData(const xiiRenderData* pRenderData, xiiRenderData::Category category);
  void AddFrameData(const xiiRenderData* pFrameData);

  void SortAndBatch();

  void Clear();

  xiiRenderDataBatchList GetRenderDataBatchesWithCategory(
    xiiRenderData::Category    category,
    xiiRenderDataBatch::Filter filter = xiiRenderDataBatch::Filter()) const;

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
  };

  xiiCamera   m_Camera;
  xiiCamera   m_LodCamera; // Temporary until we have a real LOD system
  xiiViewData m_ViewData;
  xiiTime     m_WorldTime;

  xiiDebugRendererContext m_WorldDebugContext;
  xiiDebugRendererContext m_ViewDebugContext;

  xiiHybridArray<DataPerCategory, 16>      m_DataPerCategory;
  xiiHybridArray<const xiiRenderData*, 16> m_FrameData;
};
