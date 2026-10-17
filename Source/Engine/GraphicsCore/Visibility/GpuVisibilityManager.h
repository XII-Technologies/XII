/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

class xiiGpuVisibilityManagerState;

/// Generation-checked reference to an independently configured GPU visibility context.
///
/// Contexts keep frame-sliced scene buffers and visibility-set state isolated while their
/// allocator and GAL-resource lifetime remains owned by the GraphicsCore subsystem.
using xiiGpuVisibilityContextId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiGpuVisibilityContextHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGpuVisibilityContextHandle, xiiGpuVisibilityContextId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVisibilityContextHandle);

/// Process-wide subsystem facade for GPU-driven visibility contexts.
///
/// Applications retain only generation-checked handles. The subsystem constructs CPU state after
/// Foundation startup and releases every pipeline and buffer during high-level shutdown, before
/// the default GAL device and Foundation allocators disappear.
class XII_GRAPHICSCORE_DLL xiiGpuVisibilityManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuVisibilityManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, GpuVisibilityManager);

public:
  xiiGpuVisibilityManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  /// Consumes completed asynchronous LOD readbacks for every live context before geometry
  /// streaming is scheduled for the new frame.
  static void BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);

  /// Smallest frame-buffer ring across live contexts; zero when no contexts exist.
  [[nodiscard]] static xiiUInt32 GetMinimumFramesInFlight();

  /// Creates an isolated visibility context on the current default GAL device.
  [[nodiscard]] static xiiGpuVisibilityContextHandle CreateContext(const xiiGpuVisibilityDescription& description = {});
  static void                                        DestroyContext(xiiGpuVisibilityContextHandle handle);
  [[nodiscard]] static bool                          IsValid(xiiGpuVisibilityContextHandle handle);

  [[nodiscard]] static xiiGpuVisibilityOutputs AddPasses(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const xiiGpuInstanceData& instances, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ = {});

  [[nodiscard]] static xiiUInt32                      GetMeshDispatchGroupCountX(xiiGpuVisibilityContextHandle handle);
  [[nodiscard]] static xiiUInt32                      GetMeshDispatchGroupCountY(xiiGpuVisibilityContextHandle handle);
  [[nodiscard]] static xiiGpuVisibilityStreamingStats GetStreamingStats(xiiGpuVisibilityContextHandle handle);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiGpuVisibilitySystem* GetSystem(xiiGpuVisibilityContextHandle handle);

  static xiiUniquePtr<xiiGpuVisibilityManagerState> s_pState;
};
