#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MixClips1DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAnimClip1D, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAnimClip1D>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_MEMBER_PROPERTY("Position", m_fPosition),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMixClips1DAnimNode, 1, xiiRTTIDefaultAllocator<xiiMixClips1DAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Common", m_State),

    XII_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Lerp", m_LerpPin)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation Sampling"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
    new xiiTitleAttribute("Mix1D: '{AnimationClip0}' '{AnimationClip1}' '{AnimationClip2}' '{AnimationClip3}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiAnimClip1D::SetAnimationFile(const char* sz)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(sz))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(sz);
  }

  m_hAnimation = hResource;
}

const char* xiiAnimClip1D::GetAnimationFile() const
{
  if (m_hAnimation.IsValid())
  {
    return m_hAnimation.GetResourceID();
  }

  return "";
}

xiiResult xiiMixClips1DAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Serialize(stream));

  stream << m_Clips.GetCount();
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_hAnimation;
    stream << m_Clips[i].m_fPosition;
  }

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LerpPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiMixClips1DAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Deserialize(stream));

  xiiUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_hAnimation;
    stream >> m_Clips[i].m_fPosition;
  }

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LerpPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiMixClips1DAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_LerpPin.IsConnected() || m_Clips.IsEmpty())
    return;

  if (m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
    return;

  xiiUInt32 uiClip1 = 0;
  xiiUInt32 uiClip2 = 0;

  const float fLerpPos = (float)m_LerpPin.GetNumber(graph);

  if (m_Clips.GetCount() > 1)
  {
    float fDist1 = 1000000.0f;
    float fDist2 = 1000000.0f;

    for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const float dist = xiiMath::Abs(m_Clips[i].m_fPosition - fLerpPos);

      if (dist < fDist1)
      {
        fDist2  = fDist1;
        uiClip2 = uiClip1;

        fDist1  = dist;
        uiClip1 = i;
      }
      else if (dist < fDist2)
      {
        fDist2  = dist;
        uiClip2 = i;
      }
    }

    if (xiiMath::IsZero(fDist1, xiiMath::SmallEpsilon<float>()))
    {
      uiClip2 = uiClip1;
    }
  }

  if (!m_Clips[uiClip1].m_hAnimation.IsValid() || !m_Clips[uiClip2].m_hAnimation.IsValid())
    return;

  float fLerpFactor = 0.0f;

  if (uiClip1 != uiClip2)
  {
    const float len = m_Clips[uiClip2].m_fPosition - m_Clips[uiClip1].m_fPosition;
    fLerpFactor     = (fLerpPos - m_Clips[uiClip1].m_fPosition) / len;

    // clamp and reduce to single sample when possible
    if (fLerpFactor <= 0.0f)
    {
      fLerpFactor = 0.0f;
      uiClip2     = uiClip1;
    }
    else if (fLerpFactor >= 1.0f)
    {
      fLerpFactor = 1.0f;
      uiClip1     = uiClip2;
    }
  }

  xiiResourceLock<xiiAnimationClipResource> pAnimClip1(m_Clips[uiClip1].m_hAnimation, xiiResourceAcquireMode::BlockTillLoaded);
  xiiResourceLock<xiiAnimationClipResource> pAnimClip2(m_Clips[uiClip2].m_hAnimation, xiiResourceAcquireMode::BlockTillLoaded);

  if (pAnimClip1.GetAcquireResult() != xiiResourceAcquireResult::Final || pAnimClip2.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  const auto& animDesc1 = pAnimClip1->GetDescriptor();
  const auto& animDesc2 = pAnimClip2->GetDescriptor();

  const xiiTime avgDuration = xiiMath::Lerp(animDesc1.GetDuration(), animDesc2.GetDuration(), fLerpFactor);

  const float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_bTriggerActive       = m_ActivePin.IsTriggered(graph);
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));
  m_State.m_Duration             = avgDuration;

  m_State.UpdateState(tDiff);

  if (m_State.GetCurrentState() == xiiAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }

  if (m_State.GetWeight() <= 0.0f)
    return;

  xiiAnimGraphPinDataLocalTransforms* pOutputTransform = graph.AddPinDataLocalTransforms();

  xiiAnimPoseEventTrackSampleMode eventSampling = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  if (m_State.HasLoopedStart())
    eventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    eventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;

  auto& poseGen = graph.GetPoseGenerator();

  if (uiClip1 == uiClip2)
  {
    void* pThis                        = this;
    auto& cmd                          = poseGen.AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
    cmd.m_hAnimationClip               = m_Clips[uiClip1].m_hAnimation;
    cmd.m_fNormalizedSamplePos         = m_State.GetNormalizedPlaybackPosition();
    cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
    cmd.m_EventSampling                = eventSampling;

    pOutputTransform->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    auto& cmdCmb                  = poseGen.AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    // sample animation 1
    {
      void* pThis                        = this;
      auto& cmd                          = poseGen.AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
      cmd.m_hAnimationClip               = m_Clips[uiClip1].m_hAnimation;
      cmd.m_fNormalizedSamplePos         = m_State.GetNormalizedPlaybackPosition();
      cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmd.m_EventSampling                = fLerpFactor <= 0.5f ? eventSampling : xiiAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(1.0f - fLerpFactor);
    }

    // sample animation 2
    {
      void* pThis                        = this;
      auto& cmd                          = poseGen.AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), 1));
      cmd.m_hAnimationClip               = m_Clips[uiClip2].m_hAnimation;
      cmd.m_fNormalizedSamplePos         = m_State.GetNormalizedPlaybackPosition();
      cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmd.m_EventSampling                = fLerpFactor > 0.5f ? eventSampling : xiiAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(fLerpFactor);
    }
  }

  // send to output
  {
    pOutputTransform->m_fOverallWeight = m_State.GetWeight();
    pOutputTransform->m_pWeights       = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pOutputTransform->m_bUseRootMotion = true;

      pOutputTransform->m_vRootMotion = xiiMath::Lerp(animDesc1.m_vConstantRootMotion, animDesc2.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    m_LocalPosePin.SetPose(graph, pOutputTransform);
  }
}
