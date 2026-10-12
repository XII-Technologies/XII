/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Scene/SceneDatabase.h>
#include <GraphicsCore/Scene/SceneMeshSource.h>

class xiiSceneDatabaseManagerState;
class xiiExtractedRenderData;

/// Generation-checked reference to one subsystem-owned render scene.
using xiiSceneDatabaseContextId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiSceneDatabaseContextHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiSceneDatabaseContextHandle, xiiSceneDatabaseContextId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSceneDatabaseContextHandle);

/// Process-wide subsystem facade for data-oriented render-scene contexts.
///
/// Database arrays are allocated after Foundation startup and destroyed during
/// GraphicsCore shutdown. Callers retain generation handles rather than owning
/// allocator-backed database storage directly.
class XII_GRAPHICSCORE_DLL xiiSceneDatabaseManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSceneDatabaseManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, SceneDatabaseManager);

public:
  xiiSceneDatabaseManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  [[nodiscard]] static xiiSceneDatabaseContextHandle CreateContext(xiiUInt32 uiInitialCapacity = 0U);
  static void                                        DestroyContext(xiiSceneDatabaseContextHandle handle);
  [[nodiscard]] static bool                          IsValid(xiiSceneDatabaseContextHandle handle);

  /// Returns a borrowed database pointer valid until the context is destroyed
  /// or the GraphicsCore subsystem begins shutdown.
  [[nodiscard]] static xiiSceneDatabase* GetDatabase(xiiSceneDatabaseContextHandle handle);

  /// Synchronizes ordinary xiiMeshRenderData packets into the subsystem-owned GPU scene. Stable
  /// render IDs preserve previous transforms, shared mesh resources reuse residency handles, and
  /// objects absent from the current extraction are retired after their last submitted frame.
  [[nodiscard]] static xiiResult SynchronizeExtractedMeshes(xiiSceneDatabaseContextHandle handle, const xiiExtractedRenderData& extractedData, xiiUInt64 uiFrameIndex);

  /// Resolves a native GPU object identity to its latest extraction owner in expected O(1).
  /// Zero, retired IDs, destroyed contexts and non-extracted scene objects fail and clear output.
  /// Contexts and IDs are paired; call at a CPU frame boundary, never concurrently with mutation.
  /// Owner handles are non-owning and require world validation before use.
  [[nodiscard]] static xiiResult ResolveExtractedMesh(xiiSceneDatabaseContextHandle context, xiiUInt32 uiGpuObjectId, xiiSceneMeshSource& out_source);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiSceneDatabaseManagerState> s_pState;
};

/// Lightweight owner-facing facade over a subsystem-owned scene database.
class XII_GRAPHICSCORE_DLL xiiSceneDatabaseContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSceneDatabaseContext);

public:
  xiiSceneDatabaseContext() = default;
  ~xiiSceneDatabaseContext();

  [[nodiscard]] xiiResult Initialize(xiiUInt32 uiInitialCapacity = 0U);
  void                    Shutdown();
  [[nodiscard]] bool      IsInitialized() const;

  [[nodiscard]] xiiSceneDatabase&             GetDatabase();
  [[nodiscard]] const xiiSceneDatabase&       GetDatabase() const;
  [[nodiscard]] xiiSceneDatabaseContextHandle GetHandle() const { return m_Handle; }

  [[nodiscard]] xiiResult SynchronizeExtractedMeshes(const xiiExtractedRenderData& extractedData, xiiUInt64 uiFrameIndex);

private:
  xiiSceneDatabaseContextHandle m_Handle;
};
