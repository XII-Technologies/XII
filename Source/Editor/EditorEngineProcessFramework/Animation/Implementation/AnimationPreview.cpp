/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/Animation/AnimationPreview.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

xiiResult xiiAnimationPreview::Build(const xiiSkeletonResource& skeleton, const xiiMeshResourceDescriptor& mesh,
                                     const xiiAnimationClipResource* pClip, double fNormalizedPosition)
{
  m_Palette.Clear();
  m_Pose.Clear();
  bool success = false;
  XII_SCOPE_EXIT(if (!success) { m_Palette.Clear(); m_Pose.Clear(); });
  if (!xiiMath::IsFinite(fNormalizedPosition))
    return XII_FAILURE;
  if (pClip)
  {
    pClip->SampleLocalPose(skeleton, pClip->GetDuration() * xiiMath::Clamp(fNormalizedPosition, 0.0, 1.0), m_Pose);
    if (pClip->IsAdditive())
    {
      xiiAnimationPose restPose;
      restPose.ResetToRestPose(skeleton);
      m_Pose.AdditiveBlend(restPose, m_Pose, 1.0f);
      m_Pose.BuildModelSpacePose(skeleton);
      m_Pose.BuildSkinningMatrices(skeleton);
    }
  }
  else
  {
    m_Pose.ResetToRestPose(skeleton);
    m_Pose.BuildModelSpacePose(skeleton);
    m_Pose.BuildSkinningMatrices(skeleton);
  }

  XII_SUCCEED_OR_RETURN(m_Pose.BuildMeshSkinningPalette(skeleton, mesh, m_Palette, m_UsedPaletteSlots));
  success = true;
  return XII_SUCCESS;
}

xiiResult xiiAnimationPreview::UpdateMesh(xiiSkinnedMeshComponent& component, const xiiAnimationClipResourceHandle& hClip, double fNormalizedPosition, const xiiSkeletonResourceHandle& hSkeletonOverride)
{
  bool success = false;
  XII_SCOPE_EXIT(if (!success) { component.ClearPose(); m_Palette.Clear(); m_Pose.Clear(); });
  if (!component.GetMesh().IsValid() || !xiiMath::IsFinite(fNormalizedPosition))
    return XII_FAILURE;
  xiiResourceLock<xiiMeshResource> mesh(component.GetMesh(), xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (!mesh || mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;
  const auto& descriptor = mesh->GetDescriptor();
  const auto& hSkeleton  = hSkeletonOverride.IsValid() ? hSkeletonOverride : descriptor.m_hDefaultSkeleton;
  if (!hSkeleton.IsValid())
  {
    if (hClip.IsValid())
      return XII_FAILURE;
    component.SetSkeleton({});
    component.ClearPose();
    m_Palette.Clear();
    m_Pose.Clear();
    success = true;
    return XII_SUCCESS;
  }
  xiiResourceLock<xiiSkeletonResource> skeleton(hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (!skeleton || skeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;
  if (hClip.IsValid())
  {
    xiiResourceLock<xiiAnimationClipResource> clip(hClip, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!clip || clip.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return XII_FAILURE;
    XII_SUCCEED_OR_RETURN(Build(*skeleton.GetPointer(), descriptor, clip.GetPointer(), fNormalizedPosition));
  }
  else
    XII_SUCCEED_OR_RETURN(Build(*skeleton.GetPointer(), descriptor));
  component.SetSkeleton(hSkeleton);
  component.SetSkinningMatrices(m_Palette);
  success = true;
  return XII_SUCCESS;
}
