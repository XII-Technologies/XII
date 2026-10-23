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

/// Generation-checked reference to a render graph owned by the render graph manager.
class XII_GRAPHICSCORE_DLL xiiRenderGraphHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRenderGraphHandle, xiiRenderGraphId);

  friend class xiiRenderGraphManagerState;
  friend class xiiRenderGraphManager;
};

/// Serializes device-wide frame preparation and submission across rendering workloads.
/// Begin acquires the rendering lock, waits for reusable frame slots, and advances the shared material/visibility/shadow/bindless timeline.
/// End captures all submitted queues, including failed or empty frames, and releases the lock.
/// Destruction calls End.
/// Use on one thread; nesting fails.
class XII_GRAPHICSCORE_DLL xiiRenderGraphFrame
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphFrame);

public:
  xiiRenderGraphFrame();
  ~xiiRenderGraphFrame();

  /// The requested submission window is bounded by the smallest live material, geometry, visibility, shadow or ray-tracing buffer ring.
  /// Zero, an already active scope, or nested frames return failure.
  [[nodiscard]] xiiResult Begin(xiiUInt32 uiFramesInFlight = 3U);

  /// Ends the frame and releases the rendering lock.
  void End();

  /// Returns true after Begin() and before End().
  XII_ALWAYS_INLINE bool IsActive() const { return m_uiFrameIndex != 0U; }

  /// Returns the device-wide frame index, or zero when inactive.
  XII_ALWAYS_INLINE xiiUInt64 GetFrameIndex() const { return m_uiFrameIndex; }

  /// Returns the last completed GPU frame observed when this frame began.
  XII_ALWAYS_INLINE xiiUInt64 GetCompletedFrame() const { return m_uiCompletedFrame; }

private:
  friend class xiiRenderGraphManager;

  xiiUInt64 m_uiFrameIndex     = 0U;
  xiiUInt64 m_uiCompletedFrame = 0U;
};

/// Owns render graphs and the shared GPU resources used to execute them.
/// Graph execution is explicit: registering a graph does not schedule it.
/// Each graph owns its blackboard; transient physical resources are shared per device.
/// Graph creation and destruction are serialized with frame recording.
class XII_GRAPHICSCORE_DLL xiiRenderGraphManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRenderGraphManager);

public:
  using BuildDelegate = xiiDelegate<xiiResult(xiiRenderGraph&, xiiRenderGraphBlackboard&, const xiiView*)>;

  /// Creates a manager-owned render graph. The delegate is invoked whenever ExecuteGraph() is called.
  /// A separate graph instance should be created for each view that requires independent graph or temporal state.
  [[nodiscard]] static xiiRenderGraphHandle CreateGraph(xiiStringView sName, BuildDelegate buildDelegate);

  /// Invalidates the handle immediately. During a GPU frame, graph-owned resources are retained
  /// until End() waits for that frame to complete; otherwise waits for prior submissions before release.
  [[nodiscard]] static xiiResult DestroyGraph(xiiRenderGraphHandle hGraph);

  /// Borrowed graph and blackboard access. Returns null for stale handles.
  /// The caller must serialize access with execution and destruction of this graph.
  [[nodiscard]] static xiiRenderGraph*           GetGraph(xiiRenderGraphHandle hGraph);
  [[nodiscard]] static xiiRenderGraphBlackboard* GetBlackboard(xiiRenderGraphHandle hGraph);

  /// Builds, compiles, and executes exactly one graph for the supplied view within an active shared GPU frame.
  /// Calls must be made on the thread that began the frame. Graphs are executed in caller-specified order.
  [[nodiscard]] static xiiResult ExecuteGraph(const xiiRenderGraphFrame& frame, xiiRenderGraphHandle hGraph, const xiiView* pView, const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

  /// Returns the device-scoped transient resource cache. Its frame lifecycle is managed by xiiRenderGraphFrame.
  [[nodiscard]] static xiiRenderGraphResourceCache* GetResourceCache();

  /// Returns the shared GPU timestamp profiler, which is used when GPU profiling is enabled.
  [[nodiscard]] static xiiRenderGraphTimestampProfiler* GetProfiler();

  /// Returns conservative queue completion for the currently active shared GPU frame.
  /// Allows pass-owned histories to retire images after all consumers have completed.
  /// Does not wait or advance the timeline.
  [[nodiscard]] static xiiResult GetFrameCompletion(xiiUInt64 uiFrameIndex, xiiUInt64& out_uiCompletedFrame);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderGraphManager);

  friend class xiiRenderGraphFrame;
  friend class xiiRenderGraphManagerState;

  [[nodiscard]] static xiiResult BeginFrame(xiiRenderGraphFrame& frame, xiiUInt32 uiFramesInFlight);
  static void                    EndFrame(xiiRenderGraphFrame& frame);

  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiRenderGraphManagerState> s_pState;
};
