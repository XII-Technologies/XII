/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

/// A cache from pipeline descriptor to handle which holds a reference to each pipeline that is never freed until shutdown.
class XII_GRAPHICSCORE_DLL xiiGALRenderPassCache
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALRenderPassCache);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderPassCache);

public:
  xiiGALRenderPassCache() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  /// Creates a render pass or retrieves it from the cache.
  static xiiSharedPtr<xiiGALRenderPass> GetRenderPass(const xiiGALRenderPassCreationDescription& description);

private:
  struct RenderPassCacheKey
  {
    xiiUInt32                           m_uiHash = 0U;
    xiiGALRenderPassCreationDescription m_Description;
  };

  struct CacheKeyHasher
  {
    static xiiUInt32 Hash(const RenderPassCacheKey& a);
    static bool      Equal(const RenderPassCacheKey& a, const RenderPassCacheKey& b);
  };

private:
  class State;

  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<State> s_pState;
};
