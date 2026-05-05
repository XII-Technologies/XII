/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimationPose.h>

namespace
{
  static xiiTransform BlendTransform(const xiiTransform& a, const xiiTransform& b, float fWeight)
  {
    fWeight = xiiMath::Clamp(fWeight, 0.0f, 1.0f);

    xiiTransform result;
    result.m_vPosition = a.m_vPosition + (b.m_vPosition - a.m_vPosition) * fWeight;
    result.m_qRotation = xiiQuat::MakeSlerp(a.m_qRotation, b.m_qRotation, fWeight);
    result.m_vScale    = a.m_vScale + (b.m_vScale - a.m_vScale) * fWeight;
    return result;
  }

  static bool IsJointInSubtree(xiiArrayPtr<const xiiSkeletonJoint> joints, xiiUInt32 uiJointIndex, xiiUInt16 uiRootJoint)
  {
    if (uiRootJoint == xiiMath::MaxValue<xiiUInt16>())
      return true;

    xiiUInt32 uiCurrent = uiJointIndex;
    while (uiCurrent < joints.GetCount())
    {
      if (uiCurrent == uiRootJoint)
        return true;

      const xiiUInt16 uiParent = joints[uiCurrent].m_uiParentIndex;
      if (uiParent >= uiCurrent)
        break;

      uiCurrent = uiParent;
    }

    return false;
  }
} // namespace

void xiiAnimationPose::Clear()
{
  m_LocalTransforms.Clear();
  m_ModelTransforms.Clear();
  m_SkinningMatrices.Clear();
  m_RootMotion = {};
}

void xiiAnimationPose::ResetToRestPose(const xiiSkeletonResource& skeleton)
{
  const xiiArrayPtr<const xiiSkeletonJoint> joints = skeleton.GetJoints();

  m_LocalTransforms.SetCount(joints.GetCount());
  for (xiiUInt32 i = 0; i < joints.GetCount(); ++i)
  {
    m_LocalTransforms[i] = joints[i].m_LocalRestPose;
  }

  m_ModelTransforms.Clear();
  m_SkinningMatrices.Clear();
  m_RootMotion = {};
}

void xiiAnimationPose::BuildModelSpacePose(const xiiSkeletonResource& skeleton)
{
  const xiiArrayPtr<const xiiSkeletonJoint> joints = skeleton.GetJoints();
  if (m_LocalTransforms.GetCount() != joints.GetCount())
  {
    ResetToRestPose(skeleton);
  }

  m_ModelTransforms.SetCount(joints.GetCount());
  for (xiiUInt32 i = 0; i < joints.GetCount(); ++i)
  {
    const xiiMat4 localMatrix = m_LocalTransforms[i].GetAsMat4();
    if (joints[i].m_uiParentIndex < i)
    {
      m_ModelTransforms[i] = m_ModelTransforms[joints[i].m_uiParentIndex] * localMatrix;
    }
    else
    {
      m_ModelTransforms[i] = localMatrix;
    }
  }
}

void xiiAnimationPose::BuildSkinningMatrices(const xiiSkeletonResource& skeleton)
{
  const xiiArrayPtr<const xiiSkeletonJoint> joints = skeleton.GetJoints();
  if (m_ModelTransforms.GetCount() != joints.GetCount())
  {
    BuildModelSpacePose(skeleton);
  }

  m_SkinningMatrices.SetCount(joints.GetCount());
  for (xiiUInt32 i = 0; i < joints.GetCount(); ++i)
  {
    m_SkinningMatrices[i] = m_ModelTransforms[i] * joints[i].m_InverseBindPose;
  }
}

void xiiAnimationPose::Blend(const xiiAnimationPose& a, const xiiAnimationPose& b, float fWeight)
{
  const xiiUInt32 uiCount = xiiMath::Min(a.m_LocalTransforms.GetCount(), b.m_LocalTransforms.GetCount());

  m_LocalTransforms.SetCount(uiCount);
  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    m_LocalTransforms[i] = BlendTransform(a.m_LocalTransforms[i], b.m_LocalTransforms[i], fWeight);
  }

  m_ModelTransforms.Clear();
  m_SkinningMatrices.Clear();
}

void xiiAnimationPose::AdditiveBlend(const xiiAnimationPose& basePose, const xiiAnimationPose& additivePose, float fWeight)
{
  const xiiUInt32 uiCount = xiiMath::Min(basePose.m_LocalTransforms.GetCount(), additivePose.m_LocalTransforms.GetCount());

  m_LocalTransforms.SetCount(uiCount);
  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    const xiiTransform& base = basePose.m_LocalTransforms[i];
    const xiiTransform& add  = additivePose.m_LocalTransforms[i];

    xiiTransform result;
    result.m_vPosition   = base.m_vPosition + add.m_vPosition * fWeight;
    result.m_qRotation   = xiiQuat::MakeSlerp(xiiQuat::MakeIdentity(), add.m_qRotation, xiiMath::Clamp(fWeight, 0.0f, 1.0f)) * base.m_qRotation;
    result.m_vScale      = base.m_vScale + (add.m_vScale - xiiVec3(1.0f)) * fWeight;
    m_LocalTransforms[i] = result;
  }

  m_ModelTransforms.Clear();
  m_SkinningMatrices.Clear();
}

void xiiAnimationPose::LayeredBlend(const xiiAnimationPose& basePose, const xiiAnimationPose& layerPose, const xiiSkeletonResource& skeleton, xiiUInt16 uiRootJoint, float fWeight)
{
  const xiiArrayPtr<const xiiSkeletonJoint> joints         = skeleton.GetJoints();
  const xiiUInt32                           uiCount        = xiiMath::Min(joints.GetCount(), xiiMath::Min(basePose.m_LocalTransforms.GetCount(), layerPose.m_LocalTransforms.GetCount()));
  const float                               fClampedWeight = xiiMath::Clamp(fWeight, 0.0f, 1.0f);

  m_LocalTransforms.SetCount(uiCount);
  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    if (IsJointInSubtree(joints, i, uiRootJoint))
    {
      m_LocalTransforms[i] = BlendTransform(basePose.m_LocalTransforms[i], layerPose.m_LocalTransforms[i], fClampedWeight);
    }
    else
    {
      m_LocalTransforms[i] = basePose.m_LocalTransforms[i];
    }
  }

  m_ModelTransforms.Clear();
  m_SkinningMatrices.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_AnimationPose);
