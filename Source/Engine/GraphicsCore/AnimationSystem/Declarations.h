#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <ozz/base/maths/soa_transform.h>

class xiiSkeleton;
class xiiAnimationPose;
struct xiiSkeletonResourceDescriptor;
class xiiEditableSkeletonJoint;
struct xiiAnimationClipResourceDescriptor;
class xiiAnimPoseGenerator;
class xiiGameObject;

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

#define xiiInvalidJointIndex static_cast<xiiUInt16>(0xFFFFu)

namespace ozz::animation
{
  class Skeleton;
}

/// \brief What shape is used to approximate a bone's geometry
struct xiiSkeletonJointGeometryType
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,
    ConvexMesh, ///< A convex mesh is extracted from the mesh file.

    Default = None
  };
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose is being prepared.
///
/// The pose matrices are still in local space and in the ozz internal structure-of-arrays format.
/// At this point individual bones can still be modified, to propagate the effect to the child bones.
struct XII_GRAPHICSCORE_DLL xiiMsgAnimationPosePreparing : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationPosePreparing, xiiMessage);

  const xiiSkeleton*                   m_pSkeleton = nullptr;
  xiiArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// \brief Sent to objects when a parent component is generating an animation pose, to inject additional pose commands, for instance to apply inverse kinematics (IK).
///
/// The message contains the xiiAnimPoseGenerator that is currently being built.
/// Usually it has already been executed once and generated a pose (in model space), which can be queried to build upon.
/// Additional commands can then be added to modify the pose.
/// This is mainly meant for inverse kinematics use cases.
struct XII_GRAPHICSCORE_DLL xiiMsgAnimationPoseGeneration : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationPoseGeneration, xiiMessage);

  xiiAnimPoseGenerator* m_pGenerator = nullptr;
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct XII_GRAPHICSCORE_DLL xiiMsgAnimationPoseUpdated : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationPoseUpdated, xiiMessage);

  static void ComputeFullBoneTransform(const xiiMat4& mRootTransform, const xiiMat4& mModelTransform, xiiMat4& ref_mFullTransform, xiiQuat& ref_qRotationOnly);
  void        ComputeFullBoneTransform(xiiUInt32 uiJointIndex, xiiMat4& ref_mFullTransform) const;
  void        ComputeFullBoneTransform(xiiUInt32 uiJointIndex, xiiMat4& ref_mFullTransform, xiiQuat& ref_qRotationOnly) const;

  const xiiTransform*        m_pRootTransform = nullptr;
  const xiiSkeleton*         m_pSkeleton      = nullptr;
  xiiArrayPtr<const xiiMat4> m_ModelTransforms;
  bool                       m_bContinueAnimating = true;
};

/// \brief Used by components that do rope simulation and rendering.
///
/// The rope simulation component sends this message to components attached to the same game object,
/// every time there is a new rope pose. There is no skeleton information, since all joints/bones are
/// connected as one long string.
///
/// For a rope with N segments, N+1 poses are sent. The last pose may use the same rotation as the one before.
struct XII_GRAPHICSCORE_DLL xiiMsgRopePoseUpdated : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgRopePoseUpdated, xiiMessage);

  xiiArrayPtr<const xiiTransform> m_LinkTransforms;
};

/// \brief The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct XII_GRAPHICSCORE_DLL xiiMsgQueryAnimationSkeleton : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgQueryAnimationSkeleton, xiiMessage);

  xiiSkeletonResourceHandle m_hSkeleton;
};

/// \brief This message is sent when animation root motion data is available.
///
/// Listening components can use this to move a character.
struct XII_GRAPHICSCORE_DLL xiiMsgApplyRootMotion : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgApplyRootMotion, xiiMessage);

  xiiVec3  m_vTranslation;
  xiiAngle m_RotationX;
  xiiAngle m_RotationY;
  xiiAngle m_RotationZ;
};

/// \brief Queries the local transforms of each bone in an object with a skeleton
///
/// Used to retrieve the pose of a ragdoll after simulation.
struct XII_GRAPHICSCORE_DLL xiiMsgRetrieveBoneState : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgRetrieveBoneState, xiiMessage);

  // maps from bone name to its local transform
  xiiMap<xiiString, xiiTransform> m_BoneTransforms;
};

/// \brief What type of physics constraint to use for a bone.
struct xiiSkeletonJointType
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    None,  ///< The bone is not constrained, at all. It will not be connected to another bone and fall down separately.
    Fixed, ///< The bone is joined to the parent bone by a fixed joint type and can't move, at all.
    //  Hinge,
    SwingTwist, ///< The bone is joined to the parent bone and can swing and twist relative to it in limited fashion.

    Default = None,
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSkeletonJointType);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an animated object is not visible.
///
/// It is often important to still update animated meshes, so that animation events get handled.
/// Also even though a mesh may be invisible itself, its shadow or reflection may still be visible.
struct XII_GRAPHICSCORE_DLL xiiAnimationInvisibleUpdateRate
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    FullUpdate,
    Max60FPS,
    Max30FPS,
    Max15FPS,
    Max10FPS,
    Max5FPS,
    Pause,

    Default = Max5FPS
  };

  static xiiTime GetTimeStep(xiiAnimationInvisibleUpdateRate::Enum value);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAnimationInvisibleUpdateRate);
