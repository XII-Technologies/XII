/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/Meshes/MeshResource.h>

xiiResult xiiAnimationPose::BuildMeshSkinningPalette(const xiiSkeletonResource& skeleton, const xiiMeshResourceDescriptor& mesh,
                                                     xiiDynamicArray<xiiMat4>& out_palette, xiiDynamicBitfield& inout_usedSlots) const
{
  out_palette.Clear();
  bool success = false;
  XII_SCOPE_EXIT(if (!success) out_palette.Clear());
  if (m_ModelTransforms.GetCount() != skeleton.GetJointCount())
    return XII_FAILURE;

  if (mesh.m_Bones.IsEmpty())
  {
    if (m_SkinningMatrices.GetCount() != skeleton.GetJointCount())
      return XII_FAILURE;
    out_palette = m_SkinningMatrices;
  }
  else
  {
    xiiUInt32 count = 0;
    for (const auto& bone : mesh.m_Bones)
    {
      if (bone.Value().m_uiBoneIndex == xiiMath::MaxValue<xiiUInt16>() || !bone.Value().m_GlobalInverseRestPoseMatrix.IsValid())
        return XII_FAILURE;
      count = xiiMath::Max(count, static_cast<xiiUInt32>(bone.Value().m_uiBoneIndex) + 1);
    }
    out_palette.SetCount(count, xiiMat4::MakeIdentity());
    inout_usedSlots.SetCount(count, false);
    inout_usedSlots.ClearAllBits();
    for (const auto& bone : mesh.m_Bones)
    {
      const auto joint = skeleton.FindJointByName(bone.Key());
      const auto slot  = bone.Value().m_uiBoneIndex;
      if (joint >= m_ModelTransforms.GetCount() || inout_usedSlots.IsBitSet(slot))
        return XII_FAILURE;
      inout_usedSlots.SetBit(slot);
      out_palette[slot] = m_ModelTransforms[joint] * bone.Value().m_GlobalInverseRestPoseMatrix;
    }
  }
  for (const auto& matrix : out_palette)
    if (!matrix.IsValid())
      return XII_FAILURE;
  success = true;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_AnimationPoseMesh);
