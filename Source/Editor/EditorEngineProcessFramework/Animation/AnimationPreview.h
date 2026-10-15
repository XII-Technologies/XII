/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Containers/Bitfield.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>

class xiiSkinnedMeshComponent;

/// Editor-owned pose sampling and mesh palette mapping for rest-pose and clip previews.
/// Uses runtime animation resources without advancing world simulation. Scratch capacity is reused.
/// No resource pointers survive a call; each update observes the currently loaded definitions.
class XII_EDITORENGINEPROCESSFRAMEWORK_DLL xiiAnimationPreview
{
public:
  /// Samples a clamped normalized time using the runtime clip sampler, or the rest pose.
  /// Mesh bone names resolve skeleton joints; mesh bone indices determine palette slots.
  /// Invalid times, missing joints, duplicate palette slots, and invalid matrices fail and clear output.
  xiiResult Build(const xiiSkeletonResource& skeleton, const xiiMeshResourceDescriptor& mesh, const xiiAnimationClipResource* pClip = nullptr, double fNormalizedPosition = 0.0);

  /// Acquires the mesh and its default skeleton (or an explicitly selected skeleton), samples, and updates a world-owned component.
  /// The caller holds world write access. Loading/missing resources fail and clear stale pose data.
  /// A mesh without a default skeleton permits bind-pose preview only; clip preview fails.
  xiiResult UpdateMesh(xiiSkinnedMeshComponent& component, const xiiAnimationClipResourceHandle& hClip = {}, double fNormalizedPosition = 0.0, const xiiSkeletonResourceHandle& hSkeletonOverride = {});

  xiiArrayPtr<const xiiMat4> GetSkinningMatrices() const { return m_Palette; }
  const xiiAnimationPose&    GetPose() const { return m_Pose; }

private:
  xiiAnimationPose         m_Pose;
  xiiDynamicArray<xiiMat4> m_Palette;
  xiiDynamicBitfield       m_UsedPaletteSlots;
};
