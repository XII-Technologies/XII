/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <DearImguiPlugin/DearImguiContext.h>
#include <DearImguiPlugin/DearImguiPluginDLL.h>
#include <Foundation/Configuration/StaticSubSystem.h>

class xiiGALTexture;

/// Subsystem-owned context registry. No contexts are created implicitly, and no worlds or views are retained.
/// Context/input/texture registration operations run on the main thread. Immutable frames may be consumed
/// by render setup/execution on other threads. Drain graphs and release snapshots before unloading the plugin.
class XII_DEARIMGUIPLUGIN_DLL xiiImguiSubsystem
{
public:
  static xiiResult CreateContext(xiiImguiContextHandle& out_hContext);
  /// Fails for stale handles or a context pinned by an activation scope. An unfinished frame is cancelled.
  static xiiResult DestroyContext(xiiImguiContextHandle hContext);
  static bool      IsContextValid(xiiImguiContextHandle hContext);

  /// Registers a full, single-sample 2D shader-resource texture for this context. The frame retains ownership.
  /// Color images must be sRGB textures; linear textures are suitable for masks and data already in linear light.
  static xiiResult RegisterTexture(xiiImguiContextHandle hContext, xiiSharedPtr<xiiGALTexture> pTexture, ImTextureID& out_id);
  static xiiResult UnregisterTexture(xiiImguiContextHandle hContext, ImTextureID id);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(DearImguiPlugin, DearImgui);
  friend class xiiImguiScopedContext;
  friend class xiiImguiContext;
  struct State;
  static State*           s_pState;
  static void             Startup();
  static void             Shutdown();
  static void             ClearContexts();
  static xiiImguiContext* AcquireContext(xiiImguiContextHandle hContext);
  static ImTextureID      AllocateTextureId();
};
