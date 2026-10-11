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
    pClip->SampleLocalPose(skeleton, pClip->GetDuration() * xiiMath::Clamp(fNormalizedPosition, 0.0, 1.0), m_Pose);
  else
  {
    m_Pose.ResetToRestPose(skeleton);
    m_Pose.BuildModelSpacePose(skeleton);
    m_Pose.BuildSkinningMatrices(skeleton);
  }

  if (mesh.m_Bones.IsEmpty())
    m_Palette = m_Pose.m_SkinningMatrices;
  else
  {
    xiiUInt32 count = 0;
    for (const auto& bone : mesh.m_Bones)
    {
      if (bone.Value().m_uiBoneIndex == xiiMath::MaxValue<xiiUInt16>() || !bone.Value().m_GlobalInverseRestPoseMatrix.IsValid())
        return XII_FAILURE;
      count = xiiMath::Max(count, static_cast<xiiUInt32>(bone.Value().m_uiBoneIndex) + 1);
    }
    m_Palette.SetCount(count, xiiMat4::MakeIdentity());
    m_UsedPaletteSlots.SetCount(count, false);
    m_UsedPaletteSlots.ClearAllBits();
    for (const auto& bone : mesh.m_Bones)
    {
      const auto joint = skeleton.FindJointByName(bone.Key());
      const auto slot  = bone.Value().m_uiBoneIndex;
      if (joint >= m_Pose.m_ModelTransforms.GetCount() || m_UsedPaletteSlots.IsBitSet(slot))
        return XII_FAILURE;
      m_UsedPaletteSlots.SetBit(slot);
      m_Palette[slot] = m_Pose.m_ModelTransforms[joint] * bone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
  for (const auto& matrix : m_Palette)
    if (!matrix.IsValid())
      return XII_FAILURE;
  success = true;
  return XII_SUCCESS;
}

xiiResult xiiAnimationPreview::UpdateMesh(xiiSkinnedMeshComponent& component, const xiiAnimationClipResourceHandle& hClip, double fNormalizedPosition)
{
  bool success = false;
  XII_SCOPE_EXIT(if (!success) { component.ClearPose(); m_Palette.Clear(); m_Pose.Clear(); });
  if (!component.GetMesh().IsValid() || !xiiMath::IsFinite(fNormalizedPosition))
    return XII_FAILURE;
  xiiResourceLock<xiiMeshResource> mesh(component.GetMesh(), xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (!mesh || mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return XII_FAILURE;
  const auto& descriptor = mesh->GetDescriptor();
  const auto& hSkeleton  = descriptor.m_hDefaultSkeleton;
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
