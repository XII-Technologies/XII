/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <SharedPluginScene/SharedPluginSceneDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Transform.h>
#include <SharedPluginScene/Scene/SceneMeshPickingPass.h>

/// Frame-local editor bounds for meshes whose resource or buffer is unavailable.
/// Uses existing world/component identities; owns no resources or persistent registration.
struct XII_SHAREDPLUGINSCENE_DLL xiiSceneMeshPlaceholder
{
  xiiGameObjectHandle m_hObject;
  xiiComponentHandle  m_hComponent;
  xiiBoundingBox      m_LocalBounds;
  xiiTransform        m_Transform;

  /// Requires read access to the view's world. Applies the view's object tag filters.
  static void Collect(const xiiView& view, xiiDynamicArray<xiiSceneMeshPlaceholder>& out_bounds);
  static void Draw(const xiiView& view);
  /// Picks bounds without waiting for GPU picking images. Normalized viewport coordinates.
  static xiiResult Pick(const xiiView& view, float u, float v, xiiSceneMeshPickingPass::PickingResult& out_hit);
  static void      PickObjectsInRect(const xiiView& view, const xiiRectU32& rectangle, xiiDynamicArray<xiiGameObjectHandle>& inout_objects);
};
