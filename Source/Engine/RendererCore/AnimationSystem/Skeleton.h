#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class xiiStreamWriter;
class xiiStreamReader;
class xiiSkeletonBuilder;
class xiiSkeleton;

using xiiSurfaceResourceHandle = xiiTypedResourceHandle<class xiiSurfaceResource>;

namespace ozz::animation
{
  class Skeleton;
}

/// \brief Describes a single joint.
/// The transforms of the joints are in their local space and thus need to be correctly multiplied with their parent transforms to get the
/// final transform.
class XII_RENDERERCORE_DLL xiiSkeletonJoint
{
public:
  const xiiTransform& GetBindPoseLocalTransform() const { return m_BindPoseLocal; }

  /// \brief Returns xiiInvalidJointIndex if no parent
  xiiUInt16 GetParentIndex() const { return m_uiParentIndex; }

  bool                   IsRootJoint() const { return m_uiParentIndex == xiiInvalidJointIndex; }
  const xiiHashedString& GetName() const { return m_sName; }

  xiiAngle                      GetHalfSwingLimitY() const { return m_HalfSwingLimitY; }
  xiiAngle                      GetHalfSwingLimitZ() const { return m_HalfSwingLimitZ; }
  xiiAngle                      GetTwistLimitHalfAngle() const { return m_TwistLimitHalfAngle; }
  xiiAngle                      GetTwistLimitCenterAngle() const { return m_TwistLimitCenterAngle; }
  xiiAngle                      GetTwistLimitLow() const;
  xiiAngle                      GetTwistLimitHigh() const;
  xiiEnum<xiiSkeletonJointType> GetJointType() const { return m_JointType; }

  xiiQuat GetLocalOrientation() const { return m_qLocalJointOrientation; }

  xiiSurfaceResourceHandle GetSurface() const { return m_hSurface; }
  xiiUInt8                 GetCollisionLayer() const { return m_uiCollisionLayer; }

protected:
  friend xiiSkeleton;
  friend xiiSkeletonBuilder;

  xiiTransform    m_BindPoseLocal;
  xiiUInt16       m_uiParentIndex = xiiInvalidJointIndex;
  xiiHashedString m_sName;

  xiiSurfaceResourceHandle m_hSurface;
  xiiUInt8                 m_uiCollisionLayer = 0;

  xiiEnum<xiiSkeletonJointType> m_JointType;
  xiiQuat                       m_qLocalJointOrientation = xiiQuat::IdentityQuaternion();
  xiiAngle                      m_HalfSwingLimitY;
  xiiAngle                      m_HalfSwingLimitZ;
  xiiAngle                      m_TwistLimitHalfAngle;
  xiiAngle                      m_TwistLimitCenterAngle;
};

/// \brief The skeleton class encapsulates the information about the joint structure for a model.
class XII_RENDERERCORE_DLL xiiSkeleton
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSkeleton);

public:
  xiiSkeleton();
  xiiSkeleton(xiiSkeleton&& rhs);
  ~xiiSkeleton();

  void operator=(xiiSkeleton&& rhs);

  /// \brief Returns the number of joints in the skeleton.
  xiiUInt16 GetJointCount() const { return static_cast<xiiUInt16>(m_Joints.GetCount()); }

  /// \brief Returns the nth joint.
  const xiiSkeletonJoint& GetJointByIndex(xiiUInt16 uiIndex) const { return m_Joints[uiIndex]; }

  /// \brief Allows to find a specific joint in the skeleton by name. Returns xiiInvalidJointIndex if not found
  xiiUInt16 FindJointByName(const xiiTempHashedString& sName) const;

  /// \brief Checks if two skeletons are compatible (same joint count and hierarchy)
  // bool IsCompatibleWith(const xiiSkeleton& other) const;

  /// \brief Saves the skeleton in a given stream.
  void Save(xiiStreamWriter& ref_stream) const;

  /// \brief Loads the skeleton from the given stream.
  void Load(xiiStreamReader& ref_stream);

  bool IsJointDescendantOf(xiiUInt16 uiJoint, xiiUInt16 uiExpectedParent) const;

  const ozz::animation::Skeleton& GetOzzSkeleton() const;

  xiiUInt64 GetHeapMemoryUsage() const;

  /// \brief The direction in which the bones shall point for visualization
  xiiEnum<xiiBasisAxis> m_BoneDirection;

protected:
  friend xiiSkeletonBuilder;

  xiiDynamicArray<xiiSkeletonJoint>              m_Joints;
  mutable xiiUniquePtr<ozz::animation::Skeleton> m_pOzzSkeleton;
};
