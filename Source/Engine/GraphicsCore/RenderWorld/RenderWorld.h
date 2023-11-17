#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/Pipeline/Declarations.h>

using xiiRenderPipelineResourceHandle = xiiTypedResourceHandle<class xiiRenderPipelineResource>;

struct xiiRenderWorldExtractionEvent
{
  enum class Type
  {
    BeginExtraction,
    BeforeViewExtraction,
    AfterViewExtraction,
    EndExtraction
  };

  Type      m_Type;
  xiiView*  m_pView          = nullptr;
  xiiUInt64 m_uiFrameCounter = 0;
};

struct xiiRenderWorldRenderEvent
{
  enum class Type
  {
    BeginRender,
    BeforePipelineExecution,
    AfterPipelineExecution,
    EndRender,
  };

  Type                        m_Type;
  xiiRenderPipeline*          m_pPipeline          = nullptr;
  const xiiRenderViewContext* m_pRenderViewContext = nullptr;
  xiiUInt64                   m_uiFrameCounter     = 0;
};

class XII_GRAPHICSCORE_DLL xiiRenderWorld
{
public:
  static xiiViewHandle CreateView(const char* szName, xiiView*& out_pView);
  static void          DeleteView(const xiiViewHandle& hView);

  static bool     TryGetView(const xiiViewHandle& hView, xiiView*& out_pView);
  static xiiView* GetViewByUsageHint(xiiCameraUsageHint::Enum usageHint, xiiCameraUsageHint::Enum alternativeUsageHint = xiiCameraUsageHint::None, const xiiWorld* pWorld = nullptr);

  static void                       AddMainView(const xiiViewHandle& hView);
  static void                       RemoveMainView(const xiiViewHandle& hView);
  static void                       ClearMainViews();
  static xiiArrayPtr<xiiViewHandle> GetMainViews();

  static void CacheRenderData(const xiiView& view, const xiiGameObjectHandle& hOwnerObject, const xiiComponentHandle& hOwnerComponent, xiiUInt16 uiComponentVersion, xiiArrayPtr<xiiInternal::RenderDataCacheEntry> cacheEntries);

  static void                                                 DeleteAllCachedRenderData();
  static void                                                 DeleteCachedRenderData(const xiiGameObjectHandle& hOwnerObject, const xiiComponentHandle& hOwnerComponent);
  static void                                                 DeleteCachedRenderDataForObject(const xiiGameObject* pOwnerObject);
  static void                                                 DeleteCachedRenderDataForObjectRecursive(const xiiGameObject* pOwnerObject);
  static void                                                 ResetRenderDataCache(xiiView& ref_view);
  static xiiArrayPtr<const xiiInternal::RenderDataCacheEntry> GetCachedRenderData(const xiiView& view, const xiiGameObjectHandle& hOwner, xiiUInt16 uiComponentVersion);

  static void AddViewToRender(const xiiViewHandle& hView);

  static void ExtractMainViews();

  static void Render(xiiRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static xiiEvent<xiiView*, xiiMutex> s_ViewCreatedEvent;
  static xiiEvent<xiiView*, xiiMutex> s_ViewDeletedEvent;

  static const xiiEvent<const xiiRenderWorldExtractionEvent&, xiiMutex>& GetExtractionEvent() { return s_ExtractionEvent; }
  static const xiiEvent<const xiiRenderWorldRenderEvent&, xiiMutex>&     GetRenderEvent() { return s_RenderEvent; }

  static bool GetUseMultithreadedRendering();

  /// \brief Resets the frame counter to zero. Only for test purposes !
  XII_ALWAYS_INLINE static void ResetFrameCounter() { s_uiFrameCounter = 0; }

  XII_ALWAYS_INLINE static xiiUInt64 GetFrameCounter() { return s_uiFrameCounter; }

  XII_FORCE_INLINE static xiiUInt32 GetDataIndexForExtraction() { return GetUseMultithreadedRendering() ? (s_uiFrameCounter & 1) : 0; }

  XII_FORCE_INLINE static xiiUInt32 GetDataIndexForRendering() { return GetUseMultithreadedRendering() ? ((s_uiFrameCounter + 1) & 1) : 0; }

  static bool IsRenderingThread();

  /// \name Render To Texture
  /// @{
public:
  struct CameraConfig
  {
    xiiRenderPipelineResourceHandle m_hRenderPipeline;
  };

  static void                BeginModifyCameraConfigs();
  static void                EndModifyCameraConfigs();
  static void                ClearCameraConfigs();
  static void                SetCameraConfig(const char* szName, const CameraConfig& config);
  static const CameraConfig* FindCameraConfig(const char* szName);

  static xiiEvent<void*> s_CameraConfigsModifiedEvent;

private:
  static bool                            s_bModifyingCameraConfigs;
  static xiiMap<xiiString, CameraConfig> s_CameraConfigs;

  /// @}

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderWorld);
  friend class xiiView;
  friend class xiiRenderPipeline;

  static void DeleteCachedRenderDataInternal(const xiiGameObjectHandle& hOwnerObject);
  static void ClearRenderDataCache();
  static void UpdateRenderDataCache();

  static void AddRenderPipelineToRebuild(xiiRenderPipeline* pRenderPipeline, const xiiViewHandle& hView);
  static void RebuildPipelines();

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static xiiEvent<const xiiRenderWorldExtractionEvent&, xiiMutex> s_ExtractionEvent;
  static xiiEvent<const xiiRenderWorldRenderEvent&, xiiMutex>     s_RenderEvent;
  static xiiUInt64                                                s_uiFrameCounter;
};
