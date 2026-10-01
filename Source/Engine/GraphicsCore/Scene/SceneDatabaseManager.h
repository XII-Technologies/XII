/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Scene/SceneDatabase.h>

class xiiSceneDatabaseManagerState;

/// Generation-checked reference to one subsystem-owned render scene.
struct XII_GRAPHICSCORE_DLL xiiSceneDatabaseContextHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
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

private:
  static void Startup();
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

private:
  xiiSceneDatabaseContextHandle m_Handle;
};
