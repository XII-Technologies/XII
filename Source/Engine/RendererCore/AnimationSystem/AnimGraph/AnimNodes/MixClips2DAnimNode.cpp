#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MixClips2DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAnimClip2D, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAnimClip2D>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_MEMBER_PROPERTY("Position", m_vPosition),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMixClips2DAnimNode, 1, xiiRTTIDefaultAllocator<xiiMixClips2DAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Common", m_State),

    XII_MEMBER_PROPERTY("InputResponse", m_InputResponse),
    XII_ACCESSOR_PROPERTY("CenterClip", GetCenterClipFile, SetCenterClipFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    XII_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

    XII_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("X", m_XCoordPin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("Y", m_YCoordPin)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Animation Sampling"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
    new xiiTitleAttribute("Mix2D '{CenterClip}'"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiAnimClip2D::SetAnimationFile(const char* szSz)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szSz))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(szSz);
  }

  m_hAnimation = hResource;
}

const char* xiiAnimClip2D::GetAnimationFile() const
{
  if (m_hAnimation.IsValid())
  {
    return m_hAnimation.GetResourceID();
  }

  return "";
}

xiiResult xiiMixClips2DAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Serialize(stream));

  stream << m_hCenterClip;

  stream << m_Clips.GetCount();
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_hAnimation;
    stream << m_Clips[i].m_vPosition;
  }

  XII_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_XCoordPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_YCoordPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  stream << m_InputResponse;

  return XII_SUCCESS;
}

xiiResult xiiMixClips2DAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_State.Deserialize(stream));

  stream >> m_hCenterClip;

  xiiUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_hAnimation;
    stream >> m_Clips[i].m_vPosition;
  }

  XII_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_XCoordPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_YCoordPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  stream >> m_InputResponse;

  return XII_SUCCESS;
}

void xiiMixClips2DAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || (!m_XCoordPin.IsConnected() && !m_YCoordPin.IsConnected()) || m_Clips.IsEmpty())
    return;

  if (m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
    return;

  m_State.m_bTriggerActive       = m_ActivePin.IsTriggered(graph);
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  const float x = static_cast<float>(m_XCoordPin.GetNumber(graph));
  const float y = static_cast<float>(m_YCoordPin.GetNumber(graph));

  if (m_InputResponse.IsZeroOrNegative())
  {
    m_fLastValueX = x;
    m_fLastValueY = y;
  }
  else
  {
    const float lerp = static_cast<float>(xiiMath::Min(1.0, tDiff.GetSeconds() * (1.0 / m_InputResponse.GetSeconds())));
    m_fLastValueX    = xiiMath::Lerp(m_fLastValueX, x, lerp);
    m_fLastValueY    = xiiMath::Lerp(m_fLastValueY, y, lerp);
  }

  xiiUInt32                     uiMaxWeightClip = 0;
  xiiHybridArray<ClipToPlay, 8> clips;
  ComputeClipsAndWeights(xiiVec2(m_fLastValueX, m_fLastValueY), clips, uiMaxWeightClip);

  PlayClips(graph, tDiff, clips, uiMaxWeightClip);

  if (m_State.GetCurrentState() == xiiAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }
}

void xiiMixClips2DAnimNode::ComputeClipsAndWeights(const xiiVec2& p, xiiDynamicArray<ClipToPlay>& clips, xiiUInt32& out_uiMaxWeightClip)
{
  out_uiMaxWeightClip = 0;
  float fMaxWeight    = -1.0f;

  if (m_Clips.GetCount() == 1 && !m_hCenterClip.IsValid())
  {
    clips.ExpandAndGetRef().m_uiIndex = 0;
  }
  else
  {
    // this algorithm is taken from http://runevision.com/thesis chapter 6.3 "Gradient Band Interpolation"
    // also see http://answers.unity.com/answers/1208837/view.html

    float fWeightNormalization = 0.0f;

    for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const xiiVec2 pi         = m_Clips[i].m_vPosition;
      float         fMinWeight = 1.0f;

      for (xiiUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const xiiVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr     = (pi - pj).GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi - pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight          = xiiMath::Min(fMinWeight, fWeight);
      }

      // also check against center clip
      if (m_hCenterClip.IsValid())
      {
        const float fLenSqr     = pi.GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight          = xiiMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c     = clips.ExpandAndGetRef();
        c.m_uiIndex = i;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    // also compute weight for center clip
    if (m_hCenterClip.IsValid())
    {
      float fMinWeight = 1.0f;

      for (xiiUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const xiiVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr     = pj.GetLengthSquared();
        const float fProjLenSqr = (-p).Dot(-pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight          = xiiMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c     = clips.ExpandAndGetRef();
        c.m_uiIndex = 0xFFFFFFFF;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    fWeightNormalization = 1.0f / fWeightNormalization;

    for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      auto& c = clips[i];

      c.m_fWeight *= fWeightNormalization;

      if (c.m_fWeight > fMaxWeight)
      {
        fMaxWeight          = c.m_fWeight;
        out_uiMaxWeightClip = i;
      }
    }
  }
}

void xiiMixClips2DAnimNode::SetCenterClipFile(const char* szSz)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szSz))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(szSz);
  }

  m_hCenterClip = hResource;
}

const char* xiiMixClips2DAnimNode::GetCenterClipFile() const
{
  if (!m_hCenterClip.IsValid())
    return nullptr;

  return m_hCenterClip.GetResourceID();
}

void xiiMixClips2DAnimNode::UpdateCenterClipPlaybackTime(xiiAnimGraph& graph, xiiTime tDiff, xiiAnimPoseEventTrackSampleMode& out_eventSamplingCenter)
{
  const float fSpeed = m_State.m_fPlaybackSpeed * static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  if (m_hCenterClip.IsValid())
  {
    xiiResourceLock<xiiAnimationClipResource> pClip(m_hCenterClip, xiiResourceAcquireMode::BlockTillLoaded);

    const xiiTime tDur = pClip->GetDescriptor().GetDuration();

    m_CenterPlaybackTime += tDiff * fSpeed;

    // always loop the center clip
    while (m_CenterPlaybackTime > tDur)
    {
      m_CenterPlaybackTime -= tDur;
      out_eventSamplingCenter = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
    while (m_CenterPlaybackTime < xiiTime::Zero())
    {
      m_CenterPlaybackTime += tDur;
      out_eventSamplingCenter = xiiAnimPoseEventTrackSampleMode::LoopAtStart;
    }
  }
}

void xiiMixClips2DAnimNode::PlayClips(xiiAnimGraph& graph, xiiTime tDiff, xiiArrayPtr<ClipToPlay> clips, xiiUInt32 uiMaxWeightClip)
{
  xiiTime tAvgDuration = xiiTime::Zero();

  xiiHybridArray<xiiAnimPoseGeneratorCommandSampleTrack*, 8> pSampleTrack;
  pSampleTrack.SetCountUninitialized(clips.GetCount());

  xiiVec3   vRootMotion   = xiiVec3::ZeroVector();
  xiiUInt32 uiNumAvgClips = 0;

  for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    const auto& c = clips[i];

    const xiiAnimationClipResourceHandle& hClip = c.m_uiIndex >= 0xFF ? m_hCenterClip : m_Clips[c.m_uiIndex].m_hAnimation;

    xiiResourceLock<xiiAnimationClipResource> pClip(hClip, xiiResourceAcquireMode::BlockTillLoaded);

    if (c.m_uiIndex < 0xFF) // center clip should not contribute to the average time
    {
      ++uiNumAvgClips;
      tAvgDuration += pClip->GetDescriptor().GetDuration();
    }

    void* pThis                = this;
    auto& cmd                  = graph.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), i));
    cmd.m_hAnimationClip       = hClip;
    cmd.m_fNormalizedSamplePos = pClip->GetDescriptor().GetDuration().AsFloatInSeconds(); // will be combined with actual pos below

    pSampleTrack[i] = &cmd;
    vRootMotion += pClip->GetDescriptor().m_vConstantRootMotion * c.m_fWeight;
  }

  if (uiNumAvgClips > 0)
  {
    tAvgDuration = tAvgDuration / uiNumAvgClips;
  }

  const xiiTime fPrevCenterPlaybackPos = m_CenterPlaybackTime;
  const float   fPrevPlaybackPos       = m_State.GetNormalizedPlaybackPosition();

  // now that we know the duration, we can finally update the playback state
  m_State.m_Duration = xiiMath::Max(tAvgDuration, xiiTime::Milliseconds(16));
  m_State.UpdateState(tDiff);

  if (m_State.GetWeight() <= 0.0f)
    return;

  xiiAnimPoseEventTrackSampleMode eventSamplingCenter = xiiAnimPoseEventTrackSampleMode::OnlyBetween;
  xiiAnimPoseEventTrackSampleMode eventSampling       = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  UpdateCenterClipPlaybackTime(graph, tDiff, eventSamplingCenter);

  if (m_State.HasLoopedStart())
    eventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    eventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;

  for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
  {

    if (pSampleTrack[i]->m_hAnimationClip == m_hCenterClip)
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevCenterPlaybackPos.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_fNormalizedSamplePos         = m_CenterPlaybackTime.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_EventSampling                = uiMaxWeightClip == i ? eventSamplingCenter : xiiAnimPoseEventTrackSampleMode::None;
    }
    else
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      pSampleTrack[i]->m_fNormalizedSamplePos         = m_State.GetNormalizedPlaybackPosition();
      pSampleTrack[i]->m_EventSampling                = uiMaxWeightClip == i ? eventSampling : xiiAnimPoseEventTrackSampleMode::None;
    }
  }

  xiiAnimGraphPinDataLocalTransforms* pOutputTransform = graph.AddPinDataLocalTransforms();
  pOutputTransform->m_fOverallWeight                   = m_State.GetWeight();
  pOutputTransform->m_pWeights                         = m_WeightsPin.GetWeights(graph);

  if (m_State.m_bApplyRootMotion)
  {
    pOutputTransform->m_bUseRootMotion = true;

    const float fSpeed = m_State.m_fPlaybackSpeed * static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

    pOutputTransform->m_vRootMotion = tDiff.AsFloatInSeconds() * vRootMotion * fSpeed;
  }

  if (clips.GetCount() == 1)
  {
    pOutputTransform->m_CommandID = pSampleTrack[0]->GetCommandID();
  }
  else
  {
    auto& cmdCmb                  = graph.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    cmdCmb.m_InputWeights.SetCountUninitialized(clips.GetCount());
    cmdCmb.m_Inputs.SetCountUninitialized(clips.GetCount());

    for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      cmdCmb.m_InputWeights[i] = clips[i].m_fWeight;
      cmdCmb.m_Inputs[i]       = pSampleTrack[i]->GetCommandID();
    }
  }

  m_LocalPosePin.SetPose(graph, pOutputTransform);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips2DAnimNode);
