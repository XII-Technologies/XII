/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Declarations.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiWorld;

/// Optional scene editing / thumbnail illumination using the native atmosphere and directional light.
/// The sky has the lowest possible priority; authored skies use the renderer's normal priority ordering.
/// Owns only its two helper objects, never scene-authored components. Must be destroyed before its world.
/// Methods acquire the world's write marker internally.
class XII_SHAREDPLUGINSCENE_DLL xiiScenePreviewLighting
{
public:
  explicit xiiScenePreviewLighting(xiiWorld& world);
  ~xiiScenePreviewLighting();

  XII_DISALLOW_COPY_AND_ASSIGN(xiiScenePreviewLighting);

  /// Creates or repairs the helpers. With bSetEditorTag they are excluded from scene export.
  /// Unless forced, creation is deferred while the world is empty. Repeated calls are idempotent.
  xiiResult EnsureCreated(bool bSetEditorTag, bool bForce = false);

  /// Immediately deletes helpers, including untagged thumbnail lighting, before scene export.
  void Clear();

private:
  xiiWorld&           m_World;
  xiiGameObjectHandle m_hAtmosphere;
  xiiGameObjectHandle m_hDirectionalLight;
};
