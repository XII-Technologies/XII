/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Material/MaterialSystem.h>

class xiiMaterialManagerState;

/// Process-wide facade for material instances and the frame-sliced GPU material table.
///
/// State is allocated during GraphicsCore startup rather than by a global constructor or renderer
/// owner. This guarantees that Foundation allocators and the default GAL device are available and
/// that all GPU references are released before device and allocator shutdown.
class XII_GRAPHICSCORE_DLL xiiMaterialManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiMaterialManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, MaterialManager);

public:
  xiiMaterialManager() = delete;

  /// Reconfigures the global GPU table. This is only valid before materials are registered.
  /// Before high-level startup the description is retained and applied when the GAL device exists.
  [[nodiscard]] static xiiResult Configure(const xiiMaterialGpuStorageDescription& description);
  [[nodiscard]] static bool      IsSubsystemInitialized();
  [[nodiscard]] static bool      IsInitialized();

  static void BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);
  /// Refreshes resource-backed instances and bindless texture indices after extraction has
  /// acquired new materials for the current frame. Safe to call more than once per frame.
  static void PrepareGpuResources();

  [[nodiscard]] static xiiMaterialGpuHandle RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance);
  static void                               UnregisterMaterial(xiiMaterialGpuHandle handle);

  /// Acquires the resource's immutable default instance in the global GPU table. Repeated
  /// acquisitions share one stable slot; hot reloads replace its contents without changing the
  /// slot index stored in GPU scene instances.
  [[nodiscard]] static xiiMaterialGpuHandle AcquireMaterialResource(const xiiMaterialResourceHandle& hMaterial);
  static void                               ReleaseMaterialResource(const xiiMaterialResourceHandle& hMaterial);

  [[nodiscard]] static xiiRenderGraphBufferHandle AddUploadPass(xiiRenderGraph& graph);
  [[nodiscard]] static xiiResult                  ExtractRenderData(xiiMaterialGpuHandle handle, xiiMaterialRenderData& out_renderData);

  [[nodiscard]] static xiiMaterialGpuStorage& GetGpuStorage();

  /// Builds a generated material using the same schema validation as resource-backed materials.
  [[nodiscard]] static xiiResult CreateRuntimeMaterial(const xiiMaterialSchemaDescription& description, const xiiMaterialRuntimeState& runtimeState, xiiSharedPtr<xiiMaterialSchema>& out_pSchema, xiiSharedPtr<xiiMaterialInstance>& out_pInstance, xiiStringBuilder* out_pError = nullptr);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  [[nodiscard]] static xiiResult ApplyConfiguration();

  static xiiUniquePtr<xiiMaterialManagerState> s_pState;
};
