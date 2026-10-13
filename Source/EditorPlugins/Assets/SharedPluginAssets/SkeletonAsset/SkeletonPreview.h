/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Graphics/Geometry.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <SharedPluginAssets/SkeletonAsset/EditableSkeleton.h>

struct xiiSkeletonPreviewSettings
{
  bool      m_bBones       = true;
  bool      m_bColliders   = true;
  bool      m_bJoints      = false;
  bool      m_bSwingLimits = false;
  bool      m_bTwistLimits = false;
  xiiString m_sHighlightedBones; ///< Exact joint names separated by semicolons.
};

/// Cached tooling visualization submitted through the regular debug render passes.
/// Rebuild when the resource, authoring data or display settings change; no runtime component is involved.
class XII_SHAREDPLUGINASSETS_DLL xiiSkeletonPreview
{
public:
  /// The optional authoring hierarchy must match the native joint order and parent indices.
  /// Failure clears geometry and bounds. No resource or authoring pointers survive this call.
  xiiResult Build(xiiArrayPtr<const xiiSkeletonJoint> joints, const xiiSkeletonPreviewSettings& settings, const xiiEditableSkeleton* pAuthoring = nullptr);
  void      Clear();

  const xiiGeometry&          GetGeometry() const { return m_Geometry; }
  const xiiBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  xiiGeometry          m_Geometry;
  xiiBoundingBoxSphere m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
};
