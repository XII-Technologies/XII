/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiCamera;
class xiiCameraComponent;
class xiiView;

/// Copies an active, initialized component's projection, world pose and exposure into a camera.
/// Works independently of the component's usage hint; does not alter view tags or render scale.
/// Stereo components use the Editor's mono 45-degree perspective preview. Failure leaves output
/// unchanged. Caller holds the component World's read access.
XII_SHAREDPLUGINSCENE_DLL xiiResult xiiConfigureSceneCamera(const xiiCameraComponent& component, xiiCamera& inout_camera);

/// Uses the World's active CullingOnly camera when enabled, otherwise copies the view's render camera.
/// A missing CullingOnly camera falls back successfully; malformed component settings return failure
/// with the render-camera fallback installed, so deletion/deactivation never leaves a stale pose.
/// Rejects output aliasing the rendering camera. Does not retain component pointers or modify that
/// camera. Caller holds World read access.
XII_SHAREDPLUGINSCENE_DLL xiiResult xiiConfigureSceneCullingCamera(const xiiView& view, bool bUseComponent, xiiCamera& inout_camera);
