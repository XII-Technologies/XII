#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlayClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlayClipAnimNode, 1, xiiRTTIDefaultAllocator<xiiPlayClipAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Common", m_State),
      XII_ARRAY_ACCESSOR_PROPERTY("Clips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

      XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new xiiHiddenAttribute()),

      XII_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Animation Sampling"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
      new xiiTitleAttribute("Play: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiPlayClipAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Serialize(stream));
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiPlayClipAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  // make sure there are no invalid clips in the middle clip array
  for (xiiUInt32 i = m_Clips.GetCount(); i > 0; i--)
  {
    if (!m_Clips[i - 1].IsValid())
    {
      m_Clips.RemoveAtAndSwap(i - 1);
    }
  }

  return XII_SUCCESS;
}

void xiiPlayClipAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (m_Clips.IsEmpty() || !m_LocalPosePin.IsConnected() || m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
  {
    m_uiClipToPlay     = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  xiiUInt8 uiNextClip = static_cast<xiiUInt8>(m_ClipIndexPin.GetNumber(graph, m_uiNextClipToPlay));

  if (uiNextClip >= m_Clips.GetCount())
  {
    uiNextClip = static_cast<xiiUInt8>(pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount()));
  }

  if (m_uiNextClipToPlay != uiNextClip)
  {
    xiiResourceLock<xiiAnimationClipResource> pNextClip(m_Clips[uiNextClip], xiiResourceAcquireMode::BlockTillLoaded);
    if (pNextClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    m_uiNextClipToPlay = uiNextClip;
    m_NextClipDuration = pNextClip->GetDescriptor().GetDuration();
  }

  if (m_uiClipToPlay >= m_Clips.GetCount())
  {
    m_uiClipToPlay     = uiNextClip;
    m_uiNextClipToPlay = 0xFF; // make sure the next update will pick another random clip
  }

  xiiResourceLock<xiiAnimationClipResource> pAnimClip(m_Clips[m_uiClipToPlay], xiiResourceAcquireMode::BlockTillLoaded);
  if (pAnimClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_bTriggerActive       = m_ActivePin.IsTriggered(graph);
  m_State.m_Duration             = pAnimClip->GetDescriptor().GetDuration();
  m_State.m_DurationOfQueued     = m_State.m_bLoop ? m_NextClipDuration : xiiTime::Zero();
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  m_State.UpdateState(tDiff);

  void* pThis = this;
  auto& cmd   = graph.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis)));

  if (m_Clips.GetCount() > 1 && m_State.HasTransitioned())
  {
    // guarantee that all animation events from the just finished first clip get evaluated and sent
    {
      auto& cmdE                          = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
      cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmdE.m_fNormalizedSamplePos         = m_State.GetFinalSpeed() > 0 ? 1.1f : -0.1f;
      cmdE.m_EventSampling                = xiiAnimPoseEventTrackSampleMode::OnlyBetween;
      cmdE.m_hAnimationClip               = m_Clips[m_uiClipToPlay];

      cmd.m_Inputs.PushBack(cmdE.GetCommandID());
    }

    m_uiClipToPlay     = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
    m_uiNextClipToPlay = 0xFF;
    m_NextClipDuration.SetZero();

    fPrevPlaybackPos = 0.0f;
  }

  if (m_State.GetCurrentState() == xiiAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }

  cmd.m_hAnimationClip               = m_Clips[m_uiClipToPlay];
  cmd.m_fNormalizedSamplePos         = m_State.GetNormalizedPlaybackPosition();
  cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;

  if (m_State.HasLoopedStart())
    cmd.m_EventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    cmd.m_EventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
  else
    cmd.m_EventSampling = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  {
    xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_fOverallWeight = m_State.GetWeight();
    pLocalTransforms->m_pWeights       = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }
}

xiiUInt32 xiiPlayClipAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* xiiPlayClipAnimNode::Clips_GetValue(xiiUInt32 uiIndex) const
{
  const auto& hMat = m_Clips[uiIndex];

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void xiiPlayClipAnimNode::Clips_SetValue(xiiUInt32 uiIndex, const char* value)
{
  if (xiiStringUtils::IsNullOrEmpty(value))
    m_Clips[uiIndex] = xiiAnimationClipResourceHandle();
  else
  {
    m_Clips[uiIndex] = xiiResourceManager::LoadResource<xiiAnimationClipResource>(value);
  }
}

void xiiPlayClipAnimNode::Clips_Insert(xiiUInt32 uiIndex, const char* value)
{
  xiiAnimationClipResourceHandle hMat;

  if (!xiiStringUtils::IsNullOrEmpty(value))
    hMat = xiiResourceManager::LoadResource<xiiAnimationClipResource>(value);

  m_Clips.Insert(hMat, uiIndex);
}

void xiiPlayClipAnimNode::Clips_Remove(xiiUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}
