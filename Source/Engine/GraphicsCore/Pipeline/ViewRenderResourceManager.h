/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

class xiiView;
class xiiViewRenderResourceManagerState;

/// Generation-checked reference to one view's persistent render resources.
///
/// The handle contains no allocator-backed state and remains safe after GraphicsCore shutdown.
/// A stale handle never resolves to a newly allocated view-resource context.
struct XII_GRAPHICSCORE_DLL xiiViewRenderResourceContextHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiViewRenderResourceContextHandle);

/// Owns persistent per-view GAL objects independently of world and camera lifetime.
///
/// Views still receive isolated state, but pipelines, history textures, buffers, and subordinate
/// contexts are destroyed during high-level subsystem shutdown while the GAL device and allocators
/// are valid. The opaque accessors are intentionally restricted to xiiView, which defines the
/// private resource layout.
class XII_GRAPHICSCORE_DLL xiiViewRenderResourceManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiViewRenderResourceManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, ViewRenderResourceManager);

public:
  xiiViewRenderResourceManager() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

private:
  friend class xiiView;

  [[nodiscard]] static xiiViewRenderResourceContextHandle CreateContext();
  static void                                             DestroyContext(xiiViewRenderResourceContextHandle handle);
  [[nodiscard]] static bool                               IsValid(xiiViewRenderResourceContextHandle handle);
  [[nodiscard]] static void*                              GetContext(xiiViewRenderResourceContextHandle handle);
  [[nodiscard]] static const void*                        GetContextConst(xiiViewRenderResourceContextHandle handle);

  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<xiiViewRenderResourceManagerState> s_pState;
};
