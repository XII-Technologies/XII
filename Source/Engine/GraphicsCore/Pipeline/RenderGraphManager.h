/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsCore/Pipeline/RenderGraph.h>

class xiiRenderGraphManagerState;
class xiiRenderGraphResourceCache;
class xiiRenderGraphTimestampProfiler;

using xiiRenderGraphContextId = xiiGenericId<24, 8>;

/// Generation-checked reference to an independently owned render-graph runtime context.
class XII_GRAPHICSCORE_DLL xiiRenderGraphContextHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRenderGraphContextHandle, xiiRenderGraphContextId);

  friend class xiiRenderGraphManagerState;
  friend class xiiRenderGraphManager;
};

/// Serializable registration metadata. Category, priority and registration order form the stable
/// execution key; frequency only decides whether an entry participates in a frame.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphRegistrationDescription
{
  xiiHashedString m_sName;
  xiiInt32        m_iPriority = 0;
};

/// Serializes device-wide frame preparation and submission across worlds and registered graphs.
/// Begin acquires the rendering lock, waits for reusable frame slots, and advances the shared material/visibility/shadow/bindless timeline.
/// End snapshots all queues, including failed or empty frames, then releases the lock.
/// Destruction calls End.
/// Use on one thread; nesting fails.
class XII_GRAPHICSCORE_DLL xiiRenderGraphFrame
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphFrame);

public:
  xiiRenderGraphFrame();
  ~xiiRenderGraphFrame();

  /// The requested submission window is bounded by the smallest live material, geometry, visibility, shadow or ray-tracing buffer ring. Zero, an already active scope, or nested frames return failure.
  xiiResult Begin(xiiUInt32 uiFramesInFlight = 3U);

  /// Ends the frame and releases the rendering lock.
  void End();

  /// Returns true if the frame is currently active. A frame is active after Begin() and before End().
  XII_ALWAYS_INLINE bool IsActive() const { return m_uiFrameIndex != 0U; }

  /// Returns the device-wide frame index of this frame. The index is monotonically increasing and unique for each frame.
  XII_ALWAYS_INLINE xiiUInt64 GetFrameIndex() const { return m_uiFrameIndex; }

  /// Returns the last completed frame index. This is the last frame that has finished execution and is safe to read from.
  XII_ALWAYS_INLINE xiiUInt64 GetCompletedFrame() const { return m_uiCompletedFrame; }

private:
  friend class xiiRenderGraphManager;

  xiiUInt64 m_uiFrameIndex     = 0U;
  xiiUInt64 m_uiCompletedFrame = 0U;
};

/// Process-wide deterministic scheduler for multiple render graphs in a frame.
///
/// The manager's state is created by the GraphicsCore startup system after Foundation allocators are available.
/// Keeping the public facade stateless prevents global constructors from touching allocators and guarantees that graphs and GPU resources are released during engine shutdown.
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderGraphManager);

public:
  using BuildDelegate = xiiDelegate<void(xiiRenderGraph&, xiiRenderGraphBlackboard&)>;

  xiiRenderGraphManager() = delete;

  /// Creates a new render-graph context. Contexts are independent and can be used on different threads. The name is optional and used for debugging.
  [[nodiscard]] static xiiRenderGraphContextHandle CreateContext(xiiStringView sName = {});
  static void                                      DestroyContext(xiiRenderGraphContextHandle hContext);

  [[nodiscard]] static xiiRenderGraph*                  GetGraph(xiiRenderGraphContextHandle hContext);
  [[nodiscard]] static xiiRenderGraphBlackboard*        GetBlackboard(xiiRenderGraphContextHandle hContext);
  [[nodiscard]] static xiiRenderGraphResourceCache*     GetResourceCache(xiiRenderGraphContextHandle hContext);
  [[nodiscard]] static xiiRenderGraphTimestampProfiler* GetProfiler(xiiRenderGraphContextHandle hContext);

  [[nodiscard]] static xiiRenderGraphGraphId RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate);
  static void                                UnregisterGraph(xiiRenderGraphGraphId id);

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
