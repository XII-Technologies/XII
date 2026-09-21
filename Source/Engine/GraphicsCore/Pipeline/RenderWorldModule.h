/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/WorldModule.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

class xiiRenderGraph;
class xiiRenderGraphBlackboard;
class xiiExtractedRenderData;
struct xiiMsgExtractRenderData;

class xiiView;
class xiiGameObject;

struct XII_GRAPHICSCORE_DLL xiiViewEventType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Created = 0U, ///< A view was created.
    Deleted,      ///< A view was deleted.

    ENUM_COUNT,

    Default = Created
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiViewEventType);

struct XII_GRAPHICSCORE_DLL xiiViewEvent : public xiiHashableStruct<xiiViewEvent>
{
  XII_DECLARE_POD_TYPE();

  xiiEnum<xiiViewEventType> m_Type  = xiiViewEventType::Created;
  xiiView*                  m_pView = nullptr;
};

struct XII_GRAPHICSCORE_DLL xiiRenderWorldModuleExtractionEvent
{
  enum class Type
  {
    BeforeViewExtraction = 0U, ///< Fired before extracting data for a specific view.
    AfterViewExtraction,       ///< Fired after extracting data for a specific view.
  };

  Type      m_Type;
  xiiView*  m_pView          = nullptr;
  xiiUInt64 m_uiFrameCounter = 0;
};

/// Central world module that owns all render views and drives the per-frame render graph compilation and execution.
///
/// ## Render graph construction
/// Each frame, for views that do not have a custom RenderGraphBuilder set, the module delegates to
/// xiiView::BuildDefaultRenderGraph() which populates the graph from the view's own pipeline resources.
/// External code may override this by calling xiiView::SetRenderGraphBuilder().
///
/// ## Render data
/// During the Async world-update phase, xiiRenderWorldModule walks all world objects and sends
/// xiiMsgExtractRenderData. Components handling this message submit data as usual, but the extracted
/// data cache is owned by xiiRenderWorldModule (not by xiiView) and keeps static/dynamic streams.
/// Static-only objects are persisted per-view and reused across frames until invalidated through the
/// cache invalidation API. Before execution, the module finalizes and sorts those streams for graph consumers.
///
/// ## Per-view blackboard and resource cache
/// Every xiiView owns its own xiiRenderGraphBlackboard and xiiRenderGraphResourceCache.
/// The blackboard is cleared at the start of each frame and repopulated by the passes in order.
/// Cross-frame data (TAA history, exposure, particle state, etc.) lives in persistent GPU buffers
/// inside the view's ViewPassResources struct.
class XII_GRAPHICSCORE_DLL xiiRenderWorldModule : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();

  XII_ADD_DYNAMIC_REFLECTION(xiiRenderWorldModule, xiiWorldModule);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderWorldModule);

public:
  xiiRenderWorldModule(xiiWorld* pWorld);

  virtual ~xiiRenderWorldModule();

  virtual void Initialize() override;

  virtual void Deinitialize() override;

  virtual void OnSimulationStarted() override;

  /// Creates a new view and assumes ownership.
  ///
  /// The view is registered for render-data extraction and render-graph execution from the next frame onward.
  xiiViewHandle CreateView(xiiStringView sName, xiiView*& out_pView);

  /// Destroys a view. The view must have been created by this module.
  void DestroyView(const xiiViewHandle& hView);

  /// Retrieves a view by its handle. Returns false if the handle is invalid.
  bool TryGetView(const xiiViewHandle& hView, xiiView*& out_pView) const;

  /// Retrieves a view by its usage hint. If multiple views share the same hint, the first one found is returned.
  xiiView* GetViewByUsageHint(xiiEnum<xiiCameraUsageHint> usageHint, xiiEnum<xiiCameraUsageHint> alternativeUsageHint = {}) const;

  /// Creates render data that is only valid for this frame. The data is automatically deleted after the frame has been rendered.
  template <typename T>
  T* CreateRenderDataForThisFrame(const xiiComponent* pComponent) const;

  /// Invalidates cached static render data for one object.
  ///
  /// The component handle is accepted for compatibility with existing call sites.
  /// Static cache invalidation is keyed by object identity.
  void DeleteCachedRenderData(xiiGameObjectHandle hOwnerObject, xiiComponentHandle hComponent);

  /// Invalidates cached static render data for an object and all children.
  void DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* pObject);

  /// Clears all cached static render data for every view.
  void DeleteAllCachedRenderData();

public:
  /// Events that external code can subscribe to. The events are triggered when a view is created or deleted.
  XII_ALWAYS_INLINE xiiEvent<xiiViewEvent, xiiMutex>& GetViewEvents();

  /// Events that external code can subscribe to. The events are triggered when a view is processed for render data extraction, before and after the extraction process.
  XII_ALWAYS_INLINE static const xiiEvent<const xiiRenderWorldModuleExtractionEvent&, xiiMutex>& GetRenderEvents();

private:
  struct CachedStaticObjectData
  {
    xiiDynamicArray<xiiUniquePtr<xiiRenderData>> m_StaticRenderData;
  };

  struct CachedStaticComponentData
  {
    xiiDynamicArray<xiiUniquePtr<xiiRenderData>> m_StaticRenderData;
  };

  struct ExtractedComponentFrameData
  {
    xiiDynamicArray<xiiRenderData*> m_StaticRenderData;
    bool                            m_bHasDynamicRenderData = false;
  };

  struct ExtractedObjectFrameData
  {
    xiiHashTable<xiiComponentHandle, ExtractedComponentFrameData> m_ComponentFrameData;
    xiiDynamicArray<xiiRenderData*>                               m_ObjectLevelStaticRenderData;
    bool                                                          m_bHasDynamicRenderData = false;
  };

  struct ViewExtractionCache
  {
    xiiHashTable<xiiGameObjectHandle, CachedStaticObjectData>              m_StaticObjectCache;
    xiiHashTable<xiiComponentHandle, CachedStaticComponentData>            m_StaticComponentCache;
    xiiHashTable<xiiGameObjectHandle, xiiDynamicArray<xiiComponentHandle>> m_ObjectToCachedComponents;
    xiiHashTable<xiiGameObjectHandle, ExtractedObjectFrameData>            m_FrameObjectData;
  };

  static void SubmitRenderData(void* pContext, const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching);

  void OnRenderDataSubmitted(const xiiMsgExtractRenderData& msg, xiiRenderData* pRenderData, xiiRenderData::Caching::Enum caching);
  bool ReuseCachedStaticRenderData(const ViewExtractionCache& cache, xiiGameObjectHandle hObject, xiiExtractedRenderData& out_extractedRenderData) const;
  bool ReuseCachedStaticRenderData(const ViewExtractionCache& cache, xiiComponentHandle hComponent, xiiExtractedRenderData& out_extractedRenderData) const;
  void FinalizeViewExtractionCache(ViewExtractionCache& cache);
  void RemoveCachedRenderDataForObject(ViewExtractionCache& cache, xiiGameObjectHandle hObject);
  void RemoveCachedRenderDataForComponent(ViewExtractionCache& cache, xiiGameObjectHandle hOwnerObject, xiiComponentHandle hComponent);
  void RemoveCachedRenderDataForObjectRecursive(ViewExtractionCache& cache, const xiiGameObject* pObject);

  void ExtractRenderData(const xiiWorldModule::UpdateContext& context);
  void ExecuteRenderGraphs(const xiiWorldModule::UpdateContext& context);

private:
  struct ViewDetail
  {
    xiiUniquePtr<xiiView>                m_pView;
    xiiUniquePtr<xiiExtractedRenderData> m_pExtractedData;
    ViewExtractionCache                  m_ExtractionCache;
  };

  xiiUInt64 m_uiRenderFrameIndex = 0;

  xiiIdTable<xiiViewId, ViewDetail> m_ViewIdTable;
  xiiEvent<xiiViewEvent, xiiMutex>  m_ViewEvents;

  static xiiEvent<const xiiRenderWorldModuleExtractionEvent&, xiiMutex> s_RenderEvent;
};

#include <GraphicsCore/Pipeline/Implementation/RenderWorldModule_inl.h>
