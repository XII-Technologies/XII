#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlaySequenceAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPlaySequenceAnimNode, 1, xiiRTTIDefaultAllocator<xiiPlaySequenceAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Common", m_State),

    XII_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_ARRAY_ACCESSOR_PROPERTY("MiddleClips", MiddleClips_GetCount, MiddleClips_GetValue, MiddleClips_SetValue, MiddleClips_Insert, MiddleClips_Remove)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("MiddleClipIndex", m_ClipIndexPin)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OnNextClip", m_OnNextClipPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PlayingClipIndex", m_PlayingClipIndexPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation Sampling"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Indigo)),
    new xiiTitleAttribute("Sequence: '{StartClip}' '{MiddleClips[0]}' '{EndClip}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiPlaySequenceAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Serialize(stream));
  stream << m_hStartClip;
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_hMiddleClips));
  stream << m_hEndClip;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnNextClipPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_PlayingClipIndexPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiPlaySequenceAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Deserialize(stream));
  stream >> m_hStartClip;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_hMiddleClips));
  stream >> m_hEndClip;

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnNextClipPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_PlayingClipIndexPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  // make sure there are no invalid clips in the middle clip array
  for (xiiUInt32 i = m_hMiddleClips.GetCount(); i > 0; i--)
  {
    if (!m_hMiddleClips[i - 1].IsValid())
    {
      m_hMiddleClips.RemoveAtAndSwap(i - 1);
    }
  }

  return XII_SUCCESS;
}

void xiiPlaySequenceAnimNode::SetStartClip(const char* szFile)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(szFile);
  }

  m_hStartClip = hResource;
}

const char* xiiPlaySequenceAnimNode::GetStartClip() const
{
  if (!m_hStartClip.IsValid())
    return "";

  return m_hStartClip.GetResourceID();
}

xiiUInt32 xiiPlaySequenceAnimNode::MiddleClips_GetCount() const
{
  return m_hMiddleClips.GetCount();
}

const char* xiiPlaySequenceAnimNode::MiddleClips_GetValue(xiiUInt32 uiIndex) const
{
  const auto& hMat = m_hMiddleClips[uiIndex];

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void xiiPlaySequenceAnimNode::MiddleClips_SetValue(xiiUInt32 uiIndex, const char* value)
{
  if (xiiStringUtils::IsNullOrEmpty(value))
    m_hMiddleClips[uiIndex] = xiiAnimationClipResourceHandle();
  else
    m_hMiddleClips[uiIndex] = xiiResourceManager::LoadResource<xiiAnimationClipResource>(value);
}

void xiiPlaySequenceAnimNode::MiddleClips_Insert(xiiUInt32 uiIndex, const char* value)
{
  xiiAnimationClipResourceHandle hMat;

  if (!xiiStringUtils::IsNullOrEmpty(value))
    hMat = xiiResourceManager::LoadResource<xiiAnimationClipResource>(value);

  m_hMiddleClips.Insert(hMat, uiIndex);
}

void xiiPlaySequenceAnimNode::MiddleClips_Remove(xiiUInt32 uiIndex)
{
  m_hMiddleClips.RemoveAtAndCopy(uiIndex);
}

void xiiPlaySequenceAnimNode::SetEndClip(const char* szFile)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(szFile);
  }

  m_hEndClip = hResource;
}

const char* xiiPlaySequenceAnimNode::GetEndClip() const
{
  if (!m_hEndClip.IsValid())
    return "";

  return m_hEndClip.GetResourceID();
}

void xiiPlaySequenceAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  const bool bActive = m_ActivePin.IsTriggered(graph);

  if (!m_ActivePin.IsConnected() || !m_LocalPosePin.IsConnected() || m_hMiddleClips.IsEmpty() || m_State.WillStateBeOff(bActive))
  {
    m_uiClipToPlay     = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  xiiUInt8 uiNextClip = static_cast<xiiUInt8>(m_ClipIndexPin.GetNumber(graph, m_uiNextClipToPlay));

  if (uiNextClip >= m_hMiddleClips.GetCount())
  {
    uiNextClip = static_cast<xiiUInt8>(pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_hMiddleClips.GetCount()));
  }

  m_uiNextClipToPlay = uiNextClip;

  if (m_uiClipToPlay >= m_hMiddleClips.GetCount())
  {
    m_uiClipToPlay     = uiNextClip;
    m_uiNextClipToPlay = 0xFF; // make sure the next update will pick another random clip
  }

  const bool bWasLooped = m_State.m_bLoop;
  XII_SCOPE_EXIT(m_State.m_bLoop = bWasLooped);

  if (m_Phase == Phase::Off)
  {
    m_Phase = Phase::Start;
  }

  float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0f));
  m_State.m_DurationOfQueued.SetZero();
  m_State.m_bTriggerActive = m_State.m_bImmediateFadeOut ? bActive : true;

  xiiAnimationClipResourceHandle hCurrentClip;
  xiiAnimPoseGeneratorCommandID  inputCmd = 0xFFFFFFFF;

  if (m_Phase == Phase::Start)
  {
    xiiAnimationClipResourceHandle hStartClip  = m_hStartClip.IsValid() ? m_hStartClip : m_hMiddleClips[m_uiClipToPlay];
    xiiAnimationClipResourceHandle hMiddleClip = m_hMiddleClips[uiNextClip]; // don't use m_uiNextClipToPlay here, it can be 0xFF

    xiiResourceLock<xiiAnimationClipResource> pClipStart(hStartClip, xiiResourceAcquireMode::BlockTillLoaded);

    m_State.m_bLoop    = true;
    m_State.m_Duration = pClipStart->GetDescriptor().GetDuration();

    if (m_State.GetCurrentState() == xiiAnimState::State::Running)
    {
      xiiResourceLock<xiiAnimationClipResource> pClipMiddle(hMiddleClip, xiiResourceAcquireMode::BlockTillLoaded);

      m_State.m_DurationOfQueued = pClipMiddle->GetDescriptor().GetDuration();
    }

    m_State.UpdateState(tDiff);

    if (m_State.HasTransitioned())
    {
      // guarantee that all animation events from the just finished first clip get evaluated and sent
      {
        auto& cmdE                          = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
        cmdE.m_hAnimationClip               = hStartClip;
        cmdE.m_EventSampling                = xiiAnimPoseEventTrackSampleMode::OnlyBetween;
        cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
        cmdE.m_fNormalizedSamplePos         = 1.1f;
        inputCmd                            = cmdE.GetCommandID();
      }

      m_Phase          = Phase::Middle;
      hCurrentClip     = hMiddleClip;
      fPrevPlaybackPos = 0.0f;

      m_uiClipToPlay     = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
      m_uiNextClipToPlay = 0xFF;

      m_OnNextClipPin.SetTriggered(graph, true);
    }
    else
    {
      hCurrentClip = hStartClip;
    }
  }
  else if (m_Phase == Phase::Middle)
  {
    xiiAnimationClipResourceHandle hMiddleClip1 = m_hMiddleClips[m_uiClipToPlay];
    xiiAnimationClipResourceHandle hMiddleClip2 = (bWasLooped && bActive) ? m_hMiddleClips[uiNextClip] : m_hEndClip; // invalid end clip handled below

    if (!hMiddleClip2.IsValid())
      hMiddleClip2 = hMiddleClip1; // in case end clip doesn't exist

    xiiResourceLock<xiiAnimationClipResource> pClipMiddle1(hMiddleClip1, xiiResourceAcquireMode::BlockTillLoaded);
    xiiResourceLock<xiiAnimationClipResource> pClipMiddle2(hMiddleClip2, xiiResourceAcquireMode::BlockTillLoaded);

    m_State.m_bLoop            = true;
    m_State.m_Duration         = pClipMiddle1->GetDescriptor().GetDuration();
    m_State.m_DurationOfQueued = pClipMiddle2->GetDescriptor().GetDuration();

    m_State.UpdateState(tDiff);

    hCurrentClip = hMiddleClip1;

    if (m_State.HasTransitioned())
    {
      m_Phase = (bWasLooped && bActive) ? Phase::Middle : Phase::End;

      // guarantee that all animation events from the just finished first clip get evaluated and sent
      {
        auto& cmdE                          = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
        cmdE.m_hAnimationClip               = m_hMiddleClips[m_uiClipToPlay];
        cmdE.m_EventSampling                = xiiAnimPoseEventTrackSampleMode::OnlyBetween;
        cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
        cmdE.m_fNormalizedSamplePos         = 1.1f;
        inputCmd                            = cmdE.GetCommandID();
      }

      fPrevPlaybackPos = 0.0f;
      hCurrentClip     = hMiddleClip2;

      m_uiClipToPlay     = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
      m_uiNextClipToPlay = 0xFF;

      m_OnNextClipPin.SetTriggered(graph, true);
    }
  }
  else if (m_Phase == Phase::End)
  {
    hCurrentClip = m_hEndClip.IsValid() ? m_hEndClip : m_hMiddleClips[m_uiClipToPlay];

    xiiResourceLock<xiiAnimationClipResource> pClipEnd(hCurrentClip, xiiResourceAcquireMode::BlockTillLoaded);

    m_State.m_bTriggerActive = bActive;
    m_State.m_bLoop          = false;
    m_State.m_Duration       = pClipEnd->GetDescriptor().GetDuration();

    m_State.UpdateState(tDiff);

    if (m_State.GetCurrentState() == xiiAnimState::State::StartedRampDown)
    {
      m_OnFadeOutPin.SetTriggered(graph, true);
    }
  }

  if (m_State.GetWeight() <= 0.0f || !hCurrentClip.IsValid())
  {
    m_Phase = Phase::Off;

    m_uiClipToPlay     = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  void* pThis                        = this;
  auto& cmd                          = graph.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_hAnimationClip               = hCurrentClip;
  cmd.m_fNormalizedSamplePos         = m_State.GetNormalizedPlaybackPosition();
  cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
  cmd.m_EventSampling                = xiiAnimPoseEventTrackSampleMode::OnlyBetween; // if there is a loop or transition, we handle that manually

  if (inputCmd != 0xFFFFFFFF)
  {
    cmd.m_Inputs.PushBack(inputCmd);
  }

  switch (m_Phase)
  {
    case xiiPlaySequenceAnimNode::Phase::Start:
      m_PlayingClipIndexPin.SetNumber(graph, -1);
      break;
    case xiiPlaySequenceAnimNode::Phase::Middle:
      m_PlayingClipIndexPin.SetNumber(graph, m_uiClipToPlay);
      break;
    case xiiPlaySequenceAnimNode::Phase::End:
      m_PlayingClipIndexPin.SetNumber(graph, -2);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  {
    xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_fOverallWeight = m_State.GetWeight();
    pLocalTransforms->m_pWeights       = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      xiiResourceLock<xiiAnimationClipResource> pClip(hCurrentClip, xiiResourceAcquireMode::BlockTillLoaded);

      pLocalTransforms->m_vRootMotion = pClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlaySequenceAnimNode);
