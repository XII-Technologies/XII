#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

using namespace ozz;
using namespace ozz::animation;
using namespace ozz::math;

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSimpleAnimationComponent, 2, xiiComponentMode::Static);
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_ENUM_MEMBER_PROPERTY("AnimationMode", xiiPropertyAnimMode, m_AnimationMode),
    XII_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ENUM_MEMBER_PROPERTY("RootMotionMode", xiiRootMotionMode, m_RootMotionMode),
    XII_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", xiiAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
      new xiiCategoryAttribute("Animation"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSimpleAnimationComponent::xiiSimpleAnimationComponent()  = default;
xiiSimpleAnimationComponent::~xiiSimpleAnimationComponent() = default;

void xiiSimpleAnimationComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_AnimationMode;
  s << m_fSpeed;
  s << m_hAnimationClip;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
}

void xiiSimpleAnimationComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto&           s         = inout_stream.GetStream();

  s >> m_AnimationMode;
  s >> m_fSpeed;
  s >> m_hAnimationClip;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }
}

void xiiSimpleAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;
}

void xiiSimpleAnimationComponent::SetAnimationClip(const xiiAnimationClipResourceHandle& hResource)
{
  m_hAnimationClip = hResource;
}

const xiiAnimationClipResourceHandle& xiiSimpleAnimationComponent::GetAnimationClip() const
{
  return m_hAnimationClip;
}

void xiiSimpleAnimationComponent::SetAnimationClipFile(const char* szFile)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* xiiSimpleAnimationComponent::GetAnimationClipFile() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
}

void xiiSimpleAnimationComponent::SetNormalizedPlaybackPosition(float fPosition)
{
  m_fNormalizedPlaybackPosition = fPosition;

  // force update next time
  SetUserFlag(1, true);
}

void xiiSimpleAnimationComponent::Update()
{
  if (!m_hSkeleton.IsValid() || !m_hAnimationClip.IsValid())
    return;

  if (m_fSpeed == 0.0f && !GetUserFlag(1))
    return;

  xiiTime            tMinStep = xiiTime::MakeFromSeconds(0);
  xiiVisibilityState visType  = GetOwner()->GetVisibilityState();

  if (visType != xiiVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == xiiAnimationInvisibleUpdateRate::Pause && visType == xiiVisibilityState::Invisible)
      return;

    tMinStep = xiiAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  const bool bVisible = visType != xiiVisibilityState::Invisible;

  xiiResourceLock<xiiAnimationClipResource> pAnimation(m_hAnimationClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  const xiiTime tDiff      = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate = xiiTime::Zero();

  const xiiAnimationClipResourceDescriptor& animDesc = pAnimation->GetDescriptor();

  m_Duration = animDesc.GetDuration();

  const float fPrevPlaybackPos = m_fNormalizedPlaybackPosition;

  xiiAnimPoseEventTrackSampleMode mode = xiiAnimPoseEventTrackSampleMode::None;

  if (!UpdatePlaybackTime(tDiff, animDesc.m_EventTrack, mode))
    return;

  if (animDesc.m_EventTrack.IsEmpty())
  {
    mode = xiiAnimPoseEventTrackSampleMode::None;
  }

  // no need to do anything, if we can't get events and are currently invisible
  if (!bVisible && mode == xiiAnimPoseEventTrackSampleMode::None && m_RootMotionMode == xiiRootMotionMode::Ignore)
    return;

  xiiResourceLock<xiiSkeletonResource> pSkeleton(m_hSkeleton, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  xiiAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer());

  auto& cmdSample                          = poseGen.AllocCommandSampleTrack(0);
  cmdSample.m_hAnimationClip               = m_hAnimationClip;
  cmdSample.m_fNormalizedSamplePos         = m_fNormalizedPlaybackPosition;
  cmdSample.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
  cmdSample.m_EventSampling                = mode;

  if (bVisible)
  {
    auto& cmdL2M                 = poseGen.AllocCommandLocalToModelPose();
    cmdL2M.m_pSendLocalPoseMsgTo = GetOwner();

    if (animDesc.m_bAdditive)
    {
      auto& cmdComb = poseGen.AllocCommandCombinePoses();
      cmdComb.m_Inputs.PushBack(cmdSample.GetCommandID());
      cmdComb.m_InputWeights.PushBack(1.0f);

      cmdL2M.m_Inputs.PushBack(cmdComb.GetCommandID());
    }
    else
    {
      cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
    }

    auto& cmdOut = poseGen.AllocCommandModelPoseToOutput();
    cmdOut.m_Inputs.PushBack(cmdL2M.GetCommandID());
  }

  auto pose = poseGen.GeneratePose(GetOwner());

  if (m_RootMotionMode != xiiRootMotionMode::Ignore)
  {
    xiiVec3 vRootMotion = tDiff.AsFloatInSeconds() * m_fSpeed * animDesc.m_vConstantRootMotion;

    const bool bReverse = GetUserFlag(0);
    if (bReverse)
    {
      vRootMotion = -vRootMotion;
    }

    // only applies positional root motion
    xiiRootMotionMode::Apply(m_RootMotionMode, GetOwner(), vRootMotion, xiiAngle(), xiiAngle(), xiiAngle());
  }

  if (pose.IsEmpty())
    return;

  // inform child nodes/components that a new pose is available
  {
    xiiMsgAnimationPoseProposal msg1;
    msg1.m_pRootTransform  = &pSkeleton->GetDescriptor().m_RootTransform;
    msg1.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;
    msg1.m_ModelTransforms = pose;

    GetOwner()->SendMessage(msg1);

    if (msg1.m_bContinueAnimating)
    {
      xiiMsgAnimationPoseUpdated msg2;
      msg2.m_pRootTransform  = &pSkeleton->GetDescriptor().m_RootTransform;
      msg2.m_pSkeleton       = &pSkeleton->GetDescriptor().m_Skeleton;
      msg2.m_ModelTransforms = pose;

      // recursive, so that objects below the mesh can also listen in on these changes
      // for example bone attachments
      GetOwner()->SendMessageRecursive(msg2);

      if (msg2.m_bContinueAnimating == false)
      {
        SetActiveFlag(false);
      }
    }
  }
}

bool xiiSimpleAnimationComponent::UpdatePlaybackTime(xiiTime tDiff, const xiiEventTrack& eventTrack, xiiAnimPoseEventTrackSampleMode& out_trackSampling)
{
  if (tDiff.IsZero() || m_fSpeed == 0.0f)
  {
    if (GetUserFlag(1))
    {
      SetUserFlag(1, false);
      return true;
    }

    return false;
  }

  out_trackSampling = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  const float tDiffNorm = static_cast<float>(tDiff.GetSeconds() / m_Duration.GetSeconds());
  const float tPrefNorm = m_fNormalizedPlaybackPosition;

  switch (m_AnimationMode)
  {
    case xiiPropertyAnimMode::Once:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;
      m_fNormalizedPlaybackPosition = xiiMath::Clamp(m_fNormalizedPlaybackPosition, 0.0f, 1.0f);
      break;
    }

    case xiiPropertyAnimMode::Loop:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        m_fNormalizedPlaybackPosition += 1.0f;

        out_trackSampling = xiiAnimPoseEventTrackSampleMode::LoopAtStart;
      }
      else if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        m_fNormalizedPlaybackPosition -= 1.0f;

        out_trackSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
      }

      break;
    }

    case xiiPropertyAnimMode::BackAndForth:
    {
      const bool bReverse = GetUserFlag(0);

      if (bReverse)
        m_fNormalizedPlaybackPosition -= tDiffNorm * m_fSpeed;
      else
        m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = 2.0f - m_fNormalizedPlaybackPosition;

        out_trackSampling = xiiAnimPoseEventTrackSampleMode::BounceAtEnd;
      }
      else if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = -m_fNormalizedPlaybackPosition;

        out_trackSampling = xiiAnimPoseEventTrackSampleMode::BounceAtStart;
      }

      break;
    }
  }

  return tPrefNorm != m_fNormalizedPlaybackPosition;
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_SimpleAnimationComponent);
