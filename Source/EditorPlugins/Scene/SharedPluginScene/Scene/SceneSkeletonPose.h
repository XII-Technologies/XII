/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Transform.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class xiiGameObject;

/// Copies an evaluated native animation pose for simulation pullback. Animated components take precedence over static poses.
/// Fails and clears both outputs when no current pose is available. Requires the world's read marker.
XII_SHAREDPLUGINSCENE_DLL xiiResult xiiCollectSceneSkeletonPose(const xiiGameObject& object, xiiString& out_sSkeletonResource,
                                                                xiiMap<xiiString, xiiTransform>& out_transforms);
