/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiLightingManagerState;
class xiiGALTexture;
class xiiGALFence;

/// Device-owned immutable split-sum BRDF lookup and readiness. Allocated only on demand.
class XII_GRAPHICSCORE_DLL xiiLightingManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiLightingManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, LightingManager);

public:
  xiiLightingManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  /// Ensures the process-wide split-sum BRDF lookup texture exists. The texture
  /// is owned by this subsystem so views cannot outlive the allocator or GAL
  /// device that created it.
  [[nodiscard]] static xiiResult                   EnsureBRDFLUTResources();
  [[nodiscard]] static xiiSharedPtr<xiiGALTexture> GetBRDFLUT();
  /// Completion of the immutable lookup's generation (value 1). Consumers must
  /// declare a device-side wait before using a previously published lookup.
  [[nodiscard]] static xiiSharedPtr<xiiGALFence> GetBRDFLUTReadyFence();
  [[nodiscard]] static bool                      IsBRDFLUTGenerationPending();
  static void                                    MarkBRDFLUTGenerated();

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiLightingManagerState> s_pState;
};
