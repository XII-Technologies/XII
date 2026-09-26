/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Configuration/StaticSubSystem.h>
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

  [[nodiscard]] static xiiRenderGraphGraphId RegisterGraph(const xiiRenderGraphRegistrationDescription& description, BuildDelegate buildDelegate);
  static bool                                UnregisterGraph(xiiRenderGraphGraphId id);
  static bool                                RequestExecution(xiiRenderGraphGraphId id);

  [[nodiscard]] static xiiRenderGraph*           GetGraph(xiiRenderGraphGraphId id);
  [[nodiscard]] static xiiRenderGraphBlackboard* GetBlackboard(xiiRenderGraphGraphId id);

  [[nodiscard]] static xiiRenderGraphResourceCache* GetResourceCache();
  [[nodiscard]] static xiiRenderGraphTimestampProfiler* GetProfiler();

  /// Waits before a frame-ring slot is reused and returns the latest fully completed GPU frame.
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
