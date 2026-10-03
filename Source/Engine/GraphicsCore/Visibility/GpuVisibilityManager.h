/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>

class xiiGpuVisibilityManagerState;

/// Generation-checked reference to an independently configured GPU visibility context.
///
/// Contexts keep frame-sliced scene buffers and visibility-set state isolated while their
/// allocator and GAL-resource lifetime remains owned by the GraphicsCore subsystem.
struct XII_GRAPHICSCORE_DLL xiiGpuVisibilityContextHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
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

  /// Creates an isolated visibility context on the current default GAL device.
  [[nodiscard]] static xiiGpuVisibilityContextHandle CreateContext(const xiiGpuVisibilityDescription& description = {});
  static void                                        DestroyContext(xiiGpuVisibilityContextHandle handle);
  [[nodiscard]] static bool                          IsValid(xiiGpuVisibilityContextHandle handle);

  [[nodiscard]] static xiiGpuVisibilityOutputs AddPasses(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiSceneDatabaseContextHandle sceneHandle, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ = {});

  /// Prepares subsystem-owned temporal Hi-Z storage for this context. A resolution change
  /// recreates and invalidates the history, so the next visibility pass remains conservative.
  [[nodiscard]] static xiiResult PrepareHiZHistory(xiiGpuVisibilityContextHandle handle, xiiUInt32 uiWidth, xiiUInt32 uiHeight);
  static void                    InvalidateHiZHistory(xiiGpuVisibilityContextHandle handle);
  [[nodiscard]] static xiiRenderGraphTextureHandle ImportPreviousHiZ(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);
  static void AddHiZBuildPass(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex,
                              xiiRenderGraphTextureHandle hSceneDepth, bool bAsyncCompute = true);
  [[nodiscard]] static xiiUInt32 GetHiZMipLevelCount(xiiGpuVisibilityContextHandle handle);

  [[nodiscard]] static xiiUInt32 GetMeshDispatchGroupCountX(xiiGpuVisibilityContextHandle handle);
  [[nodiscard]] static xiiUInt32 GetMeshDispatchGroupCountY(xiiGpuVisibilityContextHandle handle);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiGpuVisibilitySystem* GetSystem(xiiGpuVisibilityContextHandle handle);

  static xiiUniquePtr<xiiGpuVisibilityManagerState> s_pState;
};
