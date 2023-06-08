#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Shader/Types.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationPosePreparing);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationPosePreparing, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationPosePreparing>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationPoseUpdated);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationPoseUpdated, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationPoseUpdated>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgAnimationPoseProposal);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgAnimationPoseProposal, 1, xiiRTTIDefaultAllocator<xiiMsgAnimationPoseProposal>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgRopePoseUpdated);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgRopePoseUpdated, 1, xiiRTTIDefaultAllocator<xiiMsgRopePoseUpdated>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgQueryAnimationSkeleton);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgQueryAnimationSkeleton, 1, xiiRTTIDefaultAllocator<xiiMsgQueryAnimationSkeleton>)
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
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);
