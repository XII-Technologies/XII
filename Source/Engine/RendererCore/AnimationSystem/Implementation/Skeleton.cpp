#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

xiiSkeleton::xiiSkeleton()  = default;
xiiSkeleton::~xiiSkeleton() = default;

xiiSkeleton::xiiSkeleton(xiiSkeleton&& rhs)
{
  *this = std::move(rhs);
}

void xiiSkeleton::operator=(xiiSkeleton&& rhs)
{
  m_Joints       = std::move(rhs.m_Joints);
  m_pOzzSkeleton = std::move(rhs.m_pOzzSkeleton);
}

xiiUInt16 xiiSkeleton::FindJointByName(const xiiTempHashedString& sJointName) const
{
  const xiiUInt16 uiJointCount = static_cast<xiiUInt16>(m_Joints.GetCount());
  for (xiiUInt16 i = 0; i < uiJointCount; ++i)
  {
    if (m_Joints[i].GetName() == sJointName)
    {
      return i;
    }
  }

  return xiiInvalidJointIndex;
}

//bool xiiSkeleton::IsCompatibleWith(const xiiSkeleton& other) const
//{
//  if (this == &other)
//    return true;
//
//  if (other.GetJointCount() != GetJointCount())
//    return false;
//
//  // TODO: This only checks the joint hierarchy, maybe it should check names or hierarchy based on names
//  const xiiUInt16 uiNumJoints = static_cast<xiiUInt16>(m_Joints.GetCount());
//  for (xiiUInt32 i = 0; i < uiNumJoints; ++i)
//  {
//    if (other.m_Joints[i].GetParentIndex() != m_Joints[i].GetParentIndex())
//    {
//      return false;
//    }
//  }
//
//  return true;
//}

void xiiSkeleton::Save(xiiStreamWriter& stream) const
{
  stream.WriteVersion(5);

  const xiiUInt32 uiNumJoints = m_Joints.GetCount();
  stream << uiNumJoints;

  for (xiiUInt32 i = 0; i < uiNumJoints; ++i)
  {
    stream << m_Joints[i].m_sName;
    stream << m_Joints[i].m_uiParentIndex;
    stream << m_Joints[i].m_BindPoseLocal;

    stream << m_Joints[i].m_qLocalJointOrientation;
    stream << m_Joints[i].m_HalfSwingLimitZ;
    stream << m_Joints[i].m_HalfSwingLimitY;
    stream << m_Joints[i].m_TwistLimitHalfAngle;
    stream << m_Joints[i].m_TwistLimitCenterAngle;
  }

  stream << m_BoneDirection;
}

void xiiSkeleton::Load(xiiStreamReader& stream)
{
  const xiiTypeVersion version = stream.ReadVersion(5);
  if (version < 3)
    return;

  m_Joints.Clear();

  xiiUInt32 uiNumJoints = 0;
  stream >> uiNumJoints;

  m_Joints.Reserve(uiNumJoints);

  for (xiiUInt32 i = 0; i < uiNumJoints; ++i)
  {
    xiiSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

    stream >> joint.m_sName;
    stream >> joint.m_uiParentIndex;
    stream >> joint.m_BindPoseLocal;

    if (version >= 5)
    {
      stream >> m_Joints[i].m_qLocalJointOrientation;
      stream >> m_Joints[i].m_HalfSwingLimitZ;
      stream >> m_Joints[i].m_HalfSwingLimitY;
      stream >> m_Joints[i].m_TwistLimitHalfAngle;
      stream >> m_Joints[i].m_TwistLimitCenterAngle;
    }
  }

  if (version >= 4)
  {
    stream >> m_BoneDirection;
  }
}

bool xiiSkeleton::IsJointDescendantOf(xiiUInt16 uiJoint, xiiUInt16 uiExpectedParent) const
{
  if (uiExpectedParent == xiiInvalidJointIndex)
    return true;

  while (uiJoint != xiiInvalidJointIndex)
  {
    if (uiJoint == uiExpectedParent)
      return true;

    uiJoint = m_Joints[uiJoint].m_uiParentIndex;
  }

  return false;
}

static void BuildRawOzzSkeleton(const xiiSkeleton& skeleton, xiiUInt16 uiExpectedParent, ozz::animation::offline::RawSkeleton::Joint::Children& dstBones)
{
  xiiHybridArray<xiiUInt16, 6> children;

  for (xiiUInt16 i = 0; i < skeleton.GetJointCount(); ++i)
  {
    if (skeleton.GetJointByIndex(i).GetParentIndex() == uiExpectedParent)
    {
      children.PushBack(i);
    }
  }

  dstBones.resize((size_t)children.GetCount());

  for (xiiUInt16 i = 0; i < children.GetCount(); ++i)
  {
    const auto& srcJoint     = skeleton.GetJointByIndex(children[i]);
    const auto& srcTransform = srcJoint.GetBindPoseLocalTransform();
    auto&       dstJoint     = dstBones[i];

    dstJoint.name = srcJoint.GetName().GetData();

    dstJoint.transform.translation.x = srcTransform.m_vPosition.x;
    dstJoint.transform.translation.y = srcTransform.m_vPosition.y;
    dstJoint.transform.translation.z = srcTransform.m_vPosition.z;
    dstJoint.transform.rotation.x    = srcTransform.m_qRotation.v.x;
    dstJoint.transform.rotation.y    = srcTransform.m_qRotation.v.y;
    dstJoint.transform.rotation.z    = srcTransform.m_qRotation.v.z;
    dstJoint.transform.rotation.w    = srcTransform.m_qRotation.w;
    dstJoint.transform.scale.x       = srcTransform.m_vScale.x;
    dstJoint.transform.scale.y       = srcTransform.m_vScale.y;
    dstJoint.transform.scale.z       = srcTransform.m_vScale.z;

    BuildRawOzzSkeleton(skeleton, children[i], dstJoint.children);
  }
}

const ozz::animation::Skeleton& xiiSkeleton::GetOzzSkeleton() const
{
  if (m_pOzzSkeleton)
    return *m_pOzzSkeleton.Borrow();

  // caching the skeleton isn't thread-safe
  static xiiMutex cacheSkeletonMutex;
  XII_LOCK(cacheSkeletonMutex);

  // skip this, if the skeleton has been created in the mean-time
  if (m_pOzzSkeleton == nullptr)
  {
    ozz::animation::offline::RawSkeleton rawSkeleton;
    BuildRawOzzSkeleton(*this, xiiInvalidJointIndex, rawSkeleton.roots);

    ozz::animation::offline::SkeletonBuilder skeletonBuilder;
    const auto                               pOzzSkeleton = skeletonBuilder(rawSkeleton);

    auto ozzSkeleton = XII_DEFAULT_NEW(ozz::animation::Skeleton);

    xiiOzzUtils::CopySkeleton(ozzSkeleton, pOzzSkeleton.get());

    // since the pointer is read outside the mutex, only assign it, once it is fully ready for use
    m_pOzzSkeleton = ozzSkeleton;
  }

  return *m_pOzzSkeleton.Borrow();
}

xiiUInt64 xiiSkeleton::GetHeapMemoryUsage() const
{
  return m_Joints.GetHeapMemoryUsage(); // TODO: + ozz skeleton
}

xiiAngle xiiSkeletonJoint::GetTwistLimitLow() const
{
  return xiiMath::Max(xiiAngle::Degree(-179), m_TwistLimitCenterAngle - m_TwistLimitHalfAngle);
}

xiiAngle xiiSkeletonJoint::GetTwistLimitHigh() const
{
  return xiiMath::Min(xiiAngle::Degree(179), m_TwistLimitCenterAngle + m_TwistLimitHalfAngle);
}

XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_Skeleton);
