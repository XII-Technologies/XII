#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/AnimationSystem/Implementation/OzzUtils.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiSkeletonJointType, 1)
  XII_ENUM_CONSTANTS(xiiSkeletonJointType::None, xiiSkeletonJointType::Fixed, xiiSkeletonJointType::SwingTwist)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

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

void xiiSkeleton::Save(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(7);

  const xiiUInt32 uiNumJoints = m_Joints.GetCount();
  inout_stream << uiNumJoints;

  for (xiiUInt32 i = 0; i < uiNumJoints; ++i)
  {
    inout_stream << m_Joints[i].m_sName;
    inout_stream << m_Joints[i].m_uiParentIndex;
    inout_stream << m_Joints[i].m_RestPoseLocal;

    inout_stream << m_Joints[i].m_qLocalJointOrientation;
    inout_stream << m_Joints[i].m_HalfSwingLimitZ;
    inout_stream << m_Joints[i].m_HalfSwingLimitY;
    inout_stream << m_Joints[i].m_TwistLimitHalfAngle;
    inout_stream << m_Joints[i].m_TwistLimitCenterAngle;

    inout_stream << m_Joints[i].m_JointType;
    inout_stream << m_Joints[i].m_hSurface;
    inout_stream << m_Joints[i].m_uiCollisionLayer;
    inout_stream << m_Joints[i].m_fStiffness;
  }

  inout_stream << m_BoneDirection;
}

void xiiSkeleton::Load(xiiStreamReader& inout_stream)
{
  const xiiTypeVersion version = inout_stream.ReadVersion(7);
  if (version < 3)
    return;

  m_Joints.Clear();

  xiiUInt32 uiNumJoints = 0;
  inout_stream >> uiNumJoints;

  m_Joints.Reserve(uiNumJoints);

  for (xiiUInt32 i = 0; i < uiNumJoints; ++i)
  {
    xiiSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

    inout_stream >> joint.m_sName;
    inout_stream >> joint.m_uiParentIndex;
    inout_stream >> joint.m_RestPoseLocal;

    if (version >= 5)
    {
      inout_stream >> m_Joints[i].m_qLocalJointOrientation;
      inout_stream >> m_Joints[i].m_HalfSwingLimitZ;
      inout_stream >> m_Joints[i].m_HalfSwingLimitY;
      inout_stream >> m_Joints[i].m_TwistLimitHalfAngle;
      inout_stream >> m_Joints[i].m_TwistLimitCenterAngle;
    }

    if (version >= 6)
    {
      inout_stream >> m_Joints[i].m_JointType;
      inout_stream >> m_Joints[i].m_hSurface;
      inout_stream >> m_Joints[i].m_uiCollisionLayer;
    }

    if (version >= 7)
    {
      inout_stream >> m_Joints[i].m_fStiffness;
    }
  }

  if (version >= 4)
  {
    inout_stream >> m_BoneDirection;
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

static void BuildRawOzzSkeleton(const xiiSkeleton& skeleton, xiiUInt16 uiExpectedParent, ozz::animation::offline::RawSkeleton::Joint::Children& ref_dstBones)
{
  xiiHybridArray<xiiUInt16, 6> children;

  for (xiiUInt16 i = 0; i < skeleton.GetJointCount(); ++i)
  {
    if (skeleton.GetJointByIndex(i).GetParentIndex() == uiExpectedParent)
    {
      children.PushBack(i);
    }
  }

  ref_dstBones.resize((size_t)children.GetCount());

  for (xiiUInt16 i = 0; i < children.GetCount(); ++i)
  {
    const auto& srcJoint     = skeleton.GetJointByIndex(children[i]);
    const auto& srcTransform = srcJoint.GetRestPoseLocalTransform();
    auto&       dstJoint     = ref_dstBones[i];

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
  return xiiMath::Max(xiiAngle::MakeFromDegree(-179), m_TwistLimitCenterAngle - m_TwistLimitHalfAngle);
}

xiiAngle xiiSkeletonJoint::GetTwistLimitHigh() const
{
  return xiiMath::Min(xiiAngle::MakeFromDegree(179), m_TwistLimitCenterAngle + m_TwistLimitHalfAngle);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_Skeleton);
