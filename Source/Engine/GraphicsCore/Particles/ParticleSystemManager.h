/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Particles/ParticleSystem.h>

class xiiParticleSystemManagerState;

/// Subsystem facade that owns every GPU particle runtime.
///
/// Components retain generation handles. GPU buffers are released during
/// high-level GraphicsCore shutdown, before the GAL device and allocators.
class XII_GRAPHICSCORE_DLL xiiParticleSystemManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiParticleSystemManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, ParticleSystemManager);

public:
  xiiParticleSystemManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  [[nodiscard]] static xiiParticleSystemRuntimeHandle CreateRuntime(xiiSharedPtr<xiiGALDevice> pDevice, const xiiParticleSystemDescriptor& descriptor);
  static void                                         DestroyRuntime(xiiParticleSystemRuntimeHandle handle);
  [[nodiscard]] static bool                           IsValid(xiiParticleSystemRuntimeHandle handle);

  /// Returns a borrowed pointer valid until DestroyRuntime() or subsystem shutdown.
  [[nodiscard]] static xiiParticleSystemRuntime* GetRuntime(xiiParticleSystemRuntimeHandle handle);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiParticleSystemManagerState> s_pState;
};
