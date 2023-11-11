#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

/// \brief The skeleton builder class provides the means to build skeleton instances from scratch.
/// This class is not necessary to use skeletons, usually they should be deserialized from data created by the tools.
class XII_RENDERERCORE_DLL xiiSkeletonBuilder
{

public:
  xiiSkeletonBuilder();
  ~xiiSkeletonBuilder();

  /// \brief Adds a joint to the skeleton
  /// Since the only way to add a joint with a parent is through this method the order of joints in the array is guaranteed
  /// so that child joints always come after their parent joints
  xiiUInt16 AddJoint(xiiStringView sName, const xiiTransform& localRestPose, xiiUInt16 uiParentIndex = xiiInvalidJointIndex);

  void SetJointLimit(xiiUInt16 uiJointIndex, const xiiQuat& qLocalOrientation, xiiSkeletonJointType::Enum jointType, xiiAngle halfSwingLimitY, xiiAngle halfSwingLimitZ, xiiAngle twistLimitHalfAngle, xiiAngle twistLimitCenterAngle, float fStiffness);

  void SetJointSurface(xiiUInt16 uiJointIndex, xiiStringView sSurface);
  void SetJointCollisionLayer(xiiUInt16 uiJointIndex, xiiUInt8 uiCollsionLayer);

  /// \brief Creates a skeleton from the accumulated data.
  void BuildSkeleton(xiiSkeleton& ref_skeleton) const;

  /// \brief Returns true if there any joints have been added to the skeleton builder
  bool HasJoints() const;

protected:
  struct BuilderJoint
  {
    xiiTransform                  m_RestPoseLocal;
    xiiTransform                  m_RestPoseGlobal; // this one is temporary and not stored in the final xiiSkeleton
    xiiTransform                  m_InverseRestPoseGlobal;
    xiiUInt16                     m_uiParentIndex = xiiInvalidJointIndex;
    xiiHashedString               m_sName;
    xiiEnum<xiiSkeletonJointType> m_JointType;
    xiiQuat                       m_qLocalJointOrientation = xiiQuat::MakeIdentity();
    xiiAngle                      m_HalfSwingLimitZ;
    xiiAngle                      m_HalfSwingLimitY;
    xiiAngle                      m_TwistLimitHalfAngle;
    xiiAngle                      m_TwistLimitCenterAngle;
    float                         m_fStiffness = 0.0f;

    xiiString m_sSurface;
    xiiUInt8  m_uiCollisionLayer = 0;
  };

  xiiDeque<BuilderJoint> m_Joints;
};
