/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

struct XII_GRAPHICSCORE_DLL xiiRenderGraphCategory
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    ScenePreparation = 0U,
    SceneRendering,
    ScenePostProcess,
    Output,
    Background,
    AsyncCompute,

    ENUM_COUNT,

    Default = SceneRendering
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphCategory);

struct XII_GRAPHICSCORE_DLL xiiRenderGraphFrequency
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    EveryFrame = 0U,
    EveryNFrames,
    OnDemand,

    ENUM_COUNT,

    Default = EveryFrame
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphFrequency);

/// Serializable registration metadata. Category, priority and registration order form the stable
/// execution key; frequency only decides whether an entry participates in a frame.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphRegistrationDescription
{
  xiiString                        m_sName;
  xiiEnum<xiiRenderGraphCategory>  m_Category       = xiiRenderGraphCategory::SceneRendering;
  xiiEnum<xiiRenderGraphFrequency> m_Frequency      = xiiRenderGraphFrequency::EveryFrame;
  xiiInt32                         m_iPriority      = 0;
  xiiUInt32                        m_uiEveryNFrames = 1U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphRegistrationDescription);

class xiiRenderGraphManagerState;
class xiiRenderGraphResourceCache;
class xiiRenderGraphTimestampProfiler;

using xiiRenderGraphContextId = xiiGenericId<24, 8>;

/// Generation-checked reference to an independently owned render-graph runtime context.
class XII_GRAPHICSCORE_DLL xiiRenderGraphContextHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRenderGraphContextHandle, xiiRenderGraphContextId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphContextHandle);

/// Serializes device-wide frame preparation and submission across worlds and registered graphs.
/// Begin acquires the rendering lock, waits for reusable frame slots, and advances the shared
/// material/visibility/shadow/bindless timeline. End snapshots all queues, including failed or
/// empty frames, then releases the lock. Destruction calls End. Use on one thread; nesting fails.
class XII_GRAPHICSCORE_DLL xiiRenderGraphFrame
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphFrame);

public:
  xiiRenderGraphFrame() = default;
  ~xiiRenderGraphFrame();
  /// The requested submission window is bounded by the smallest live material, geometry,
  /// visibility, shadow or ray-tracing buffer ring. Zero, an already active scope, or nested frames return failure.
  [[nodiscard]] xiiResult Begin(xiiUInt32 uiFramesInFlight = 3U);
  void                    End();
  [[nodiscard]] bool      IsActive() const { return m_uiFrameIndex != 0U; }
  [[nodiscard]] xiiUInt64 GetFrameIndex() const { return m_uiFrameIndex; }
  [[nodiscard]] xiiUInt64 GetCompletedFrame() const { return m_uiCompletedFrame; }

private:
  friend class xiiRenderGraphManager;
  xiiUInt64 m_uiFrameIndex     = 0U;
  xiiUInt64 m_uiCompletedFrame = 0U;
};

/// Process-wide deterministic scheduler for multiple render graphs in a frame.
///
/// The manager's state is created by the GraphicsCore startup system after Foundation allocators
/// are available. Keeping the public facade stateless prevents global constructors from touching
/// allocators and guarantees that graphs and GPU resources are released during engine shutdown.
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderGraphManager);

public:
  using BuildDelegate = xiiDelegate<void(xiiRenderGraph&, xiiRenderGraphBlackboard&)>;

  xiiRenderGraphManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  /// Creates an isolated graph, blackboard, transient cache, and timestamp profiler.
  [[nodiscard]] static xiiRenderGraphContextHandle CreateContext(xiiStringView sName = {});
  static void                                      DestroyContext(xiiRenderGraphContextHandle handle);
  [[nodiscard]] static bool                        IsValid(xiiRenderGraphContextHandle handle);

  [[nodiscard]] static xiiRenderGraph*                  GetGraph(xiiRenderGraphContextHandle handle);
  [[nodiscard]] static xiiRenderGraphBlackboard*        GetBlackboard(xiiRenderGraphContextHandle handle);
  [[nodiscard]] static xiiRenderGraphResourceCache*     GetResourceCache(xiiRenderGraphContextHandle handle);
  [[nodiscard]] static xiiRenderGraphTimestampProfiler* GetProfiler(xiiRenderGraphContextHandle handle);

  [[nodiscard]] static xiiRenderGraphGraphId RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate);
  static bool                                UnregisterGraph(xiiRenderGraphGraphId id);
  static bool                                RequestExecution(xiiRenderGraphGraphId id);

  [[nodiscard]] static xiiRenderGraph*           GetGraph(xiiRenderGraphGraphId id);
  [[nodiscard]] static xiiRenderGraphBlackboard* GetBlackboard(xiiRenderGraphGraphId id);

  [[nodiscard]] static xiiRenderGraphResourceCache*     GetResourceCache();
  [[nodiscard]] static xiiRenderGraphTimestampProfiler* GetProfiler();

  /// Conservative queue completion for the currently active shared GPU frame. Allows
  /// pass-owned histories to retire images after all consumers have finished. Requires
  /// the caller's graph to belong to this frame; does not wait or advance the timeline.
  [[nodiscard]] static xiiResult GetFrameCompletion(xiiUInt64 uiFrameIndex, xiiUInt64& out_uiCompletedFrame);

  /// Executes eligible graphs in a newly allocated shared GPU frame. Frequency decisions use
  /// the device-wide frame index, including world submissions. Failed frames are captured too.
  [[nodiscard]] static xiiResult ExecuteFrame(const xiiView* pView, xiiUInt32 uiFramesInFlight = 3U, const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

  /// Executes registered graphs within an active frame supplied by the caller, which may also
  /// submit other graphs before End. The frame owns preparation, completion capture and locking.
  [[nodiscard]] static xiiResult ExecuteFrame(const xiiRenderGraphFrame& frame, const xiiView* pView, const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

private:
  friend class xiiRenderGraphFrame;
  [[nodiscard]] static xiiResult BeginFrame(xiiRenderGraphFrame& frame, xiiUInt32 uiFramesInFlight);
  static void                    EndFrame(xiiRenderGraphFrame& frame);
  [[nodiscard]] static xiiResult ExecuteFrame(xiiUInt64 uiFrameIndex, xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, const xiiRenderGraphCompileSettings& settings, xiiStringBuilder* out_pError);

  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiRenderGraphManagerState> s_pState;
};

/// Lightweight owner-facing facade for a subsystem-owned render-graph context.
class XII_GRAPHICSCORE_DLL xiiRenderGraphContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphContext);

public:
  xiiRenderGraphContext() = default;
  ~xiiRenderGraphContext();

  [[nodiscard]] xiiResult Initialize(xiiStringView sName = {});
  void                    Shutdown();
  [[nodiscard]] bool      IsInitialized() const;

  [[nodiscard]] xiiRenderGraph&                    GetGraph();
  [[nodiscard]] const xiiRenderGraph&              GetGraph() const;
  [[nodiscard]] xiiRenderGraphBlackboard&          GetBlackboard();
  [[nodiscard]] const xiiRenderGraphBlackboard&    GetBlackboard() const;
  [[nodiscard]] xiiRenderGraphResourceCache&       GetResourceCache();
  [[nodiscard]] const xiiRenderGraphResourceCache& GetResourceCache() const;
  [[nodiscard]] xiiRenderGraphTimestampProfiler&   GetProfiler();

private:
  xiiRenderGraphContextHandle m_Handle;
};
