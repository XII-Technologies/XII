/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

struct XII_GRAPHICSCORE_DLL xiiRenderGraphCategory
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    ScenePreparation,
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
    EveryFrame,
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

/// Generation-checked reference to an independently owned render-graph runtime context.
struct XII_GRAPHICSCORE_DLL xiiRenderGraphContextHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRenderGraphContextHandle);

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

  /// Waits for a reusable frame slot, advances frame-scoped rendering subsystems and deferred
  /// bindless descriptor collection, then returns the latest completed GPU frame. Frame indices
  /// start at one; zero is reserved as the conservative "none completed" sentinel.
  [[nodiscard]] static xiiUInt64 PrepareFrame(xiiUInt64 uiFrameIndex, xiiUInt32 uiFramesInFlight);

  /// Executes eligible graphs using the default GAL device and subsystem-owned cache/profiler.
  /// The completed frame is used to retire transient resources without reusing in-flight memory.
  [[nodiscard]] static xiiResult ExecuteFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, const xiiView* pView, const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

  /// Builds, compiles and executes all graphs eligible for this frame in deterministic order.
  [[nodiscard]] static xiiResult ExecuteFrame(xiiUInt64 uiFrameIndex, xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, const xiiRenderGraphCompileSettings& settings = {}, xiiStringBuilder* out_pError = nullptr);

private:
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
