/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <SharedPluginAssets/SkeletonAsset/EditableSkeleton.h>

namespace
{
  // Clear all child arrays before deleting anything. Besides avoiding recursive destruction, this permits
  // safe cleanup of a hierarchy rejected by conversion for repeated pointers or cycles.
  void DeleteJoints(xiiHybridArray<xiiEditableSkeletonJoint*, 4>& inout_roots, xiiEditableSkeletonJoint* pOwner = nullptr)
  {
    if (inout_roots.IsEmpty())
      return;

    xiiDynamicArray<xiiEditableSkeletonJoint*> pending;
    xiiDynamicArray<xiiEditableSkeletonJoint*> joints;
    xiiHashSet<xiiEditableSkeletonJoint*>      visited;
    if (pOwner != nullptr)
      visited.Insert(pOwner);
    pending.PushBackRange(inout_roots);
    inout_roots.Clear();
    while (!pending.IsEmpty())
    {
      auto* pJoint = pending.PeekBack();
      pending.PopBack();
      if (pJoint == nullptr || visited.Insert(pJoint))
        continue;

      joints.PushBack(pJoint);
      pending.PushBackRange(pJoint->m_Children);
      pJoint->m_Children.Clear();
    }
    for (auto* pJoint : joints)
      XII_DEFAULT_DELETE(pJoint);
  }

  // Import matrices can reflect an axis. MakeFromMat4 loses the scale sign and would pass a reflection
  // to quaternion construction; explicitly move it to the X scale and require orthogonal TRS columns.
  xiiResult DecomposeRootPose(const xiiMat4& matrix, xiiTransform& out_pose)
  {
    if (!matrix.IsValid())
      return XII_FAILURE;
    xiiMat3 rotation = matrix.GetRotationalPart();
    xiiVec3 scale    = rotation.GetScalingFactors();
    if (!scale.IsValid() || scale.x <= 0 || scale.y <= 0 || scale.z <= 0)
      return XII_FAILURE;
    if (rotation.GetDeterminant() < 0)
      scale.x = -scale.x;
    rotation.SetColumn(0, rotation.GetColumn(0) / scale.x);
    rotation.SetColumn(1, rotation.GetColumn(1) / scale.y);
    rotation.SetColumn(2, rotation.GetColumn(2) / scale.z);

    if (!rotation.IsValid() || !xiiMath::IsEqual(rotation.GetDeterminant(), 1.0f, 0.001f) || !xiiMath::IsZero(rotation.GetColumn(0).Dot(rotation.GetColumn(1)), 0.001f) || !xiiMath::IsZero(rotation.GetColumn(0).Dot(rotation.GetColumn(2)), 0.001f) || !xiiMath::IsZero(rotation.GetColumn(1).Dot(rotation.GetColumn(2)), 0.001f))
      return XII_FAILURE;

    out_pose = xiiTransform::Make(matrix.GetTranslationVector(), xiiQuat::MakeFromMat3(rotation), scale);
    return out_pose.IsValid() ? XII_SUCCESS : XII_FAILURE;
  }
} // namespace

xiiEditableSkeleton::~xiiEditableSkeleton()
{
  ClearJoints();
}

void xiiEditableSkeleton::ClearJoints()
{
  DeleteJoints(m_Children);
}

xiiEditableSkeletonJoint::~xiiEditableSkeletonJoint()
{
  ClearJoints();
}

void xiiEditableSkeletonJoint::ClearJoints()
{
  DeleteJoints(m_Children, this);
}

const char* xiiEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void xiiEditableSkeletonJoint::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

void xiiEditableSkeletonJoint::CopyPropertiesFrom(const xiiEditableSkeletonJoint* pJoint)
{
  m_BoneShapes               = pJoint->m_BoneShapes;
  m_qLocalJointRotation      = pJoint->m_qLocalJointRotation;
  m_JointType                = pJoint->m_JointType;
  m_SwingLimitY              = pJoint->m_SwingLimitY;
  m_SwingLimitZ              = pJoint->m_SwingLimitZ;
  m_TwistLimitHalfAngle      = pJoint->m_TwistLimitHalfAngle;
  m_TwistLimitCenterAngle    = pJoint->m_TwistLimitCenterAngle;
  m_fStiffness               = pJoint->m_fStiffness;
  m_bOverrideSurface         = pJoint->m_bOverrideSurface;
  m_bOverrideCollisionLayer  = pJoint->m_bOverrideCollisionLayer;
  m_sSurfaceOverride         = pJoint->m_sSurfaceOverride;
  m_uiCollisionLayerOverride = pJoint->m_uiCollisionLayerOverride;
}

xiiResult xiiEditableSkeleton::GetRootTransform(xiiTransform& out_transform) const
{
  out_transform.SetIdentity();
  if (!xiiMath::IsFinite(m_fUniformScaling) || m_fUniformScaling <= 0 || (m_ImportTransform != xiiMeshImportTransform::Blender_YUp && m_ImportTransform != xiiMeshImportTransform::Blender_ZUp && m_ImportTransform != xiiMeshImportTransform::Custom))
    return XII_FAILURE;

  const auto right       = xiiMeshImportTransform::GetRightDir(m_ImportTransform, m_RightDir);
  const auto up          = xiiMeshImportTransform::GetUpDir(m_ImportTransform, m_UpDir);
  const bool flipForward = xiiMeshImportTransform::GetFlipForward(m_ImportTransform, m_bFlipForwardDir);
  if (right < xiiBasisAxis::PositiveX || right > xiiBasisAxis::NegativeZ || up < xiiBasisAxis::PositiveX || up > xiiBasisAxis::NegativeZ || xiiMath::Abs(xiiBasisAxis::GetBasisVector(right).Dot(xiiBasisAxis::GetBasisVector(up))) > 0.5f)
    return XII_FAILURE;

  const auto forward = xiiBasisAxis::GetOrthogonalAxis(right, up, !flipForward);
  const auto matrix  = xiiBasisAxis::CalculateTransformationMatrix(forward, right, up, m_fUniformScaling);
  return DecomposeRootPose(xiiMat4::MakeTransformation(matrix, xiiVec3::MakeZero()), out_transform);
}

xiiResult xiiEditableSkeleton::FillResourceDescriptor(xiiSkeletonResourceDescriptor& out_descriptor) const
{
  out_descriptor.Clear();
  xiiTransform root;
  XII_SUCCEED_OR_RETURN(GetRootTransform(root));
  const auto                    rootMatrix = root.GetAsMat4();
  xiiSkeletonResourceDescriptor descriptor;
  struct PendingJoint
  {
    XII_DECLARE_POD_TYPE();
    const xiiEditableSkeletonJoint* m_pJoint;
    xiiUInt16                       m_uiParent;
  };
  constexpr auto                              invalidJoint = xiiMath::MaxValue<xiiUInt16>();
  xiiDynamicArray<PendingJoint>               pending;
  xiiHashSet<const xiiEditableSkeletonJoint*> visited;
  xiiHashSet<xiiString>                       names;
  xiiDynamicArray<xiiVec3>                    positions;
  for (xiiUInt32 i = m_Children.GetCount(); i > 0; --i)
    pending.PushBack({m_Children[i - 1], invalidJoint});

  while (!pending.IsEmpty())
  {
    const auto entry = pending.PeekBack();
    pending.PopBack();
    const auto* pJoint = entry.m_pJoint;
    if (pJoint == nullptr || visited.Insert(pJoint) || pJoint->m_sName.IsEmpty() || names.Insert(pJoint->m_sName.GetString()) || descriptor.m_Joints.GetCount() >= invalidJoint || !pJoint->m_LocalTransform.IsValid())
      return XII_FAILURE;

    auto localPose = pJoint->m_LocalTransform;
    if (entry.m_uiParent == invalidJoint)
      XII_SUCCEED_OR_RETURN(DecomposeRootPose(rootMatrix * localPose.GetAsMat4(), localPose));
    const auto localMatrix = localPose.GetAsMat4();
    const auto modelMatrix = entry.m_uiParent == invalidJoint ? localMatrix : descriptor.m_Joints[entry.m_uiParent].m_ModelRestPose * localMatrix;
    auto       inverse     = modelMatrix;
    if (!modelMatrix.IsValid() || inverse.Invert(0.0f).Failed() || !inverse.IsValid())
      return XII_FAILURE;

    const xiiUInt16 index   = descriptor.AddJoint(pJoint->m_sName.GetString(), entry.m_uiParent, localPose);
    auto&           joint   = descriptor.m_Joints[index];
    joint.m_ModelRestPose   = modelMatrix;
    joint.m_InverseBindPose = inverse;
    positions.PushBack(modelMatrix.GetTranslationVector());
    for (xiiUInt32 i = pJoint->m_Children.GetCount(); i > 0; --i)
      pending.PushBack({pJoint->m_Children[i - 1], index});
  }

  descriptor.m_Bounds = positions.IsEmpty() ? xiiBoundingBoxSphere::MakeZero() : xiiBoundingBoxSphere::MakeFromPoints(positions.GetData(), positions.GetCount());
  if (!descriptor.m_Bounds.IsValid())
    return XII_FAILURE;
  descriptor.ComputeRuntimeHash();
  out_descriptor = std::move(descriptor);
  return XII_SUCCESS;
}
