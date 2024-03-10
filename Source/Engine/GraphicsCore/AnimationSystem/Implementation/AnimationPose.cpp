#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimationPose.h>
#include <GraphicsCore/AnimationSystem/Skeleton.h>
#include <GraphicsFoundation/Shader/Types.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationPosePreparing);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationPosePreparing, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationPosePreparing>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationPoseUpdated);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationPoseUpdated, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationPoseUpdated>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationPoseProposal);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationPoseProposal, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationPoseProposal>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgRopePoseUpdated);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgRopePoseUpdated, 1, xiiRTTIDefaultAllocator<xiiMsgRopePoseUpdated>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgQueryAnimationSkeleton);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgQueryAnimationSkeleton, 1, xiiRTTIDefaultAllocator<xiiMsgQueryAnimationSkeleton>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgApplyRootMotion);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgApplyRootMotion, 1, xiiRTTIDefaultAllocator<xiiMsgApplyRootMotion>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Translation", m_vTranslation),
    XII_MEMBER_PROPERTY("RotationX", m_RotationX),
    XII_MEMBER_PROPERTY("RotationY", m_RotationY),
    XII_MEMBER_PROPERTY("RotationZ", m_RotationZ),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgRetrieveBoneState);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgRetrieveBoneState, 1, xiiRTTIDefaultAllocator<xiiMsgRetrieveBoneState>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiExcludeFromScript()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiAnimationInvisibleUpdateRate, 1)
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::FullUpdate),
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::Max60FPS),
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::Max30FPS),
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::Max15FPS),
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::Max10FPS),
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::Max5FPS),
  XII_ENUM_CONSTANT(xiiAnimationInvisibleUpdateRate::Pause),
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiTime xiiAnimationInvisibleUpdateRate::GetTimeStep(xiiAnimationInvisibleUpdateRate::Enum value)
{
  switch (value)
  {
    case xiiAnimationInvisibleUpdateRate::FullUpdate:
      return xiiTime::Zero();
    case xiiAnimationInvisibleUpdateRate::Max60FPS:
      return xiiTime::Seconds(1.0 / 60.0);
    case xiiAnimationInvisibleUpdateRate::Max30FPS:
      return xiiTime::Seconds(1.0 / 30.0);
    case xiiAnimationInvisibleUpdateRate::Max15FPS:
      return xiiTime::Seconds(1.0 / 15.0);
    case xiiAnimationInvisibleUpdateRate::Max10FPS:
      return xiiTime::Seconds(1.0 / 10.0);

    case xiiAnimationInvisibleUpdateRate::Max5FPS:
    case xiiAnimationInvisibleUpdateRate::Pause: // full pausing should be handled separately, and if something isn't fully paused, it should behave like a very low update rate
      return xiiTime::Seconds(1.0 / 5.0);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiTime::Zero();
}

void xiiMsgAnimationPoseUpdated::ComputeFullBoneTransform(xiiUInt32 uiJointIndex, xiiMat4& ref_mFullTransform) const
{
  ref_mFullTransform = m_pRootTransform->GetAsMat4() * m_ModelTransforms[uiJointIndex];
}

void xiiMsgAnimationPoseUpdated::ComputeFullBoneTransform(const xiiMat4& mRootTransform, const xiiMat4& mModelTransform, xiiMat4& ref_mFullTransform, xiiQuat& ref_qRotationOnly)
{
  ref_mFullTransform = mRootTransform * mModelTransform;

  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  ref_qRotationOnly.ReconstructFromMat4(ref_mFullTransform);
}

void xiiMsgAnimationPoseUpdated::ComputeFullBoneTransform(xiiUInt32 uiJointIndex, xiiMat4& ref_mFullTransform, xiiQuat& ref_qRotationOnly) const
{
  ComputeFullBoneTransform(m_pRootTransform->GetAsMat4(), m_ModelTransforms[uiJointIndex], ref_mFullTransform, ref_qRotationOnly);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_Implementation_AnimationPose);
