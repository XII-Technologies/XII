#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/maths/soa_transform.h>

class xiiSkeleton;
class xiiAnimationPose;
struct xiiSkeletonResourceDescriptor;
class xiiEditableSkeletonJoint;
struct xiiAnimationClipResourceDescriptor;

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

#define xiiInvalidJointIndex static_cast<xiiUInt16>(0xFFFFu)

namespace ozz::animation
{
  class Skeleton;
}

struct xiiSkeletonJointGeometryType
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,

    Default = None
  };
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose is being prepared.
///
/// The pose matrices are still in local space and in the ozz internal structure-of-arrays format.
/// At this point individual bones can still be modified, to propagate the effect to the child bones.
struct XII_RENDERERCORE_DLL xiiMsgAnimationPosePreparing : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationPosePreparing, xiiMessage);

  const xiiSkeleton*                   m_pSkeleton = nullptr;
  xiiArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct XII_RENDERERCORE_DLL xiiMsgAnimationPoseUpdated : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationPoseUpdated, xiiMessage);

  static void ComputeFullBoneTransform(const xiiMat4& rootTransform, const xiiMat4& modelTransform, xiiMat4& fullTransform, xiiQuat& rotationOnly);
  void        ComputeFullBoneTransform(xiiUInt32 uiJointIndex, xiiMat4& fullTransform) const;
  void        ComputeFullBoneTransform(xiiUInt32 uiJointIndex, xiiMat4& fullTransform, xiiQuat& rotationOnly) const;

  const xiiTransform*        m_pRootTransform = nullptr;
  const xiiSkeleton*         m_pSkeleton      = nullptr;
  xiiArrayPtr<const xiiMat4> m_ModelTransforms;
  bool                       m_bContinueAnimating = true;
};

struct XII_RENDERERCORE_DLL xiiMsgAnimationPoseProposal : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgAnimationPoseProposal, xiiMessage);

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
struct XII_RENDERERCORE_DLL xiiMsgRopePoseUpdated : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgRopePoseUpdated, xiiMessage);

  xiiArrayPtr<const xiiTransform> m_LinkTransforms;
};

/// \brief The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct XII_RENDERERCORE_DLL xiiMsgQueryAnimationSkeleton : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgQueryAnimationSkeleton, xiiMessage);

  xiiSkeletonResourceHandle m_hSkeleton;
};

/// \brief This message is sent when animation root motion data is available.
///
/// Listening components can use this to move a character.
struct XII_RENDERERCORE_DLL xiiMsgApplyRootMotion : public xiiMessage
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
struct XII_RENDERERCORE_DLL xiiMsgRetrieveBoneState : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgRetrieveBoneState, xiiMessage);

  // maps from bone name to its local transform
  xiiMap<xiiString, xiiTransform> m_BoneTransforms;
};
