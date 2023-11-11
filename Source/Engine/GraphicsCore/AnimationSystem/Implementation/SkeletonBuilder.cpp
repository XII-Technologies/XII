#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/AnimationSystem/SkeletonBuilder.h>

xiiSkeletonBuilder::xiiSkeletonBuilder()  = default;
xiiSkeletonBuilder::~xiiSkeletonBuilder() = default;

xiiUInt16 xiiSkeletonBuilder::AddJoint(xiiStringView sName, const xiiTransform& localRestPose, xiiUInt16 uiParentIndex /*= xiiInvalidJointIndex*/)
{
  XII_ASSERT_DEV(uiParentIndex == xiiInvalidJointIndex || uiParentIndex < m_Joints.GetCount(), "Invalid parent index for joint");

  auto& joint = m_Joints.ExpandAndGetRef();

  joint.m_RestPoseLocal  = localRestPose;
  joint.m_RestPoseGlobal = localRestPose;
  joint.m_sName.Assign(sName);
  joint.m_uiParentIndex = uiParentIndex;

  if (uiParentIndex != xiiInvalidJointIndex)
  {
    joint.m_RestPoseGlobal = m_Joints[joint.m_uiParentIndex].m_RestPoseGlobal * joint.m_RestPoseLocal;
  }

  joint.m_InverseRestPoseGlobal = joint.m_RestPoseGlobal.GetInverse();

  return static_cast<xiiUInt16>(m_Joints.GetCount() - 1);
}

void xiiSkeletonBuilder::SetJointLimit(xiiUInt16 uiJointIndex, const xiiQuat& qLocalOrientation, xiiSkeletonJointType::Enum jointType, xiiAngle halfSwingLimitY, xiiAngle halfSwingLimitZ, xiiAngle twistLimitHalfAngle, xiiAngle twistLimitCenterAngle, float fStiffness)
{
  auto& j                    = m_Joints[uiJointIndex];
  j.m_qLocalJointOrientation = qLocalOrientation;
  j.m_JointType              = jointType;
  j.m_HalfSwingLimitY        = halfSwingLimitY;
  j.m_HalfSwingLimitZ        = halfSwingLimitZ;
  j.m_TwistLimitHalfAngle    = twistLimitHalfAngle;
  j.m_TwistLimitCenterAngle  = twistLimitCenterAngle;
  j.m_fStiffness             = fStiffness;
}


void xiiSkeletonBuilder::SetJointSurface(xiiUInt16 uiJointIndex, xiiStringView sSurface)
{
  auto& j      = m_Joints[uiJointIndex];
  j.m_sSurface = sSurface;
}

void xiiSkeletonBuilder::SetJointCollisionLayer(xiiUInt16 uiJointIndex, xiiUInt8 uiCollsionLayer)
{
  auto& j              = m_Joints[uiJointIndex];
  j.m_uiCollisionLayer = uiCollsionLayer;
}

void xiiSkeletonBuilder::BuildSkeleton(xiiSkeleton& ref_skeleton) const
{
  // XII_ASSERT_DEV(HasJoints(), "Can't build a skeleton with no joints!");

  const xiiUInt32 numJoints = m_Joints.GetCount();

  // Copy joints to skeleton
  ref_skeleton.m_Joints.SetCount(numJoints);

  for (xiiUInt32 i = 0; i < numJoints; ++i)
  {
    ref_skeleton.m_Joints[i].m_sName         = m_Joints[i].m_sName;
    ref_skeleton.m_Joints[i].m_uiParentIndex = m_Joints[i].m_uiParentIndex;
    ref_skeleton.m_Joints[i].m_RestPoseLocal = m_Joints[i].m_RestPoseLocal;

    ref_skeleton.m_Joints[i].m_JointType              = m_Joints[i].m_JointType;
    ref_skeleton.m_Joints[i].m_qLocalJointOrientation = m_Joints[i].m_qLocalJointOrientation;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitY        = m_Joints[i].m_HalfSwingLimitY;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitZ        = m_Joints[i].m_HalfSwingLimitZ;
    ref_skeleton.m_Joints[i].m_TwistLimitHalfAngle    = m_Joints[i].m_TwistLimitHalfAngle;
    ref_skeleton.m_Joints[i].m_TwistLimitCenterAngle  = m_Joints[i].m_TwistLimitCenterAngle;

    ref_skeleton.m_Joints[i].m_uiCollisionLayer = m_Joints[i].m_uiCollisionLayer;
    ref_skeleton.m_Joints[i].m_hSurface         = xiiResourceManager::LoadResource<xiiSurfaceResource>(m_Joints[i].m_sSurface);
    ref_skeleton.m_Joints[i].m_fStiffness       = m_Joints[i].m_fStiffness;
  }
}

bool xiiSkeletonBuilder::HasJoints() const
{
  return !m_Joints.IsEmpty();
}



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_SkeletonBuilder);
