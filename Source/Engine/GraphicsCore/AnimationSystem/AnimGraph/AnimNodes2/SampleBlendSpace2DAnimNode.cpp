#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/SampleBlendSpace2DAnimNode.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAnimationClip2D, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAnimationClip2D>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    XII_MEMBER_PROPERTY("Position", m_vPosition),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSampleBlendSpace2DAnimNode, 1, xiiRTTIDefaultAllocator<xiiSampleBlendSpace2DAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, {})),
      XII_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      XII_MEMBER_PROPERTY("InputResponse", m_InputResponse)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::MakeFromMilliseconds(100))),
    XII_ACCESSOR_PROPERTY("CenterClip", GetCenterClipFile, SetCenterClipFile)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      XII_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      XII_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("X", m_InCoordX)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("Y", m_InCoordY)->AddAttributes(new xiiHiddenAttribute()),

      XII_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Pose Generation"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
      new xiiTitleAttribute("BlendSpace 2D: '{CenterClip}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiAnimationClip2D::SetAnimationFile(const char* szFile)
{
  m_sClip.Assign(szFile);
}

const char* xiiAnimationClip2D::GetAnimationFile() const
{
  return m_sClip;
}

xiiSampleBlendSpace2DAnimNode::xiiSampleBlendSpace2DAnimNode()  = default;
xiiSampleBlendSpace2DAnimNode::~xiiSampleBlendSpace2DAnimNode() = default;

xiiResult xiiSampleBlendSpace2DAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sCenterClip;

  stream << m_Clips.GetCount();
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_sClip;
    stream << m_Clips[i].m_vPosition;
  }

  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;
  stream << m_InputResponse;

  XII_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InCoordX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InCoordY.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSampleBlendSpace2DAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sCenterClip;

  xiiUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_sClip;
    stream >> m_Clips[i].m_vPosition;
  }

  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;
  stream >> m_InputResponse;

  XII_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InCoordX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InCoordY.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSampleBlendSpace2DAnimNode::SetCenterClipFile(const char* szFile)
{
  m_sCenterClip.Assign(szFile);
}

const char* xiiSampleBlendSpace2DAnimNode::GetCenterClipFile() const
{
  return m_sCenterClip;
}

void xiiSampleBlendSpace2DAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || (!m_InCoordX.IsConnected() && !m_InCoordY.IsConnected()) || m_Clips.IsEmpty())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_CenterPlaybackTime    = xiiTime::MakeZero();
    pState->m_fOtherPlaybackPosNorm = 0.0f;
    pState->m_bPlaying              = true;

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  if (!pState->m_bPlaying)
    return;

  const float x = static_cast<float>(m_InCoordX.GetNumber(ref_graph));
  const float y = static_cast<float>(m_InCoordY.GetNumber(ref_graph));

  if (m_InputResponse.IsZeroOrNegative())
  {
    pState->m_fLastValueX = x;
    pState->m_fLastValueY = y;
  }
  else
  {
    const float lerp      = static_cast<float>(xiiMath::Min(1.0, tDiff.GetSeconds() * (1.0 / m_InputResponse.GetSeconds())));
    pState->m_fLastValueX = xiiMath::Lerp(pState->m_fLastValueX, x, lerp);
    pState->m_fLastValueY = xiiMath::Lerp(pState->m_fLastValueY, y, lerp);
  }

  const auto& centerInfo = ref_controller.GetAnimationClipInfo(m_sCenterClip);

  xiiUInt32                     uiMaxWeightClip = 0;
  xiiHybridArray<ClipToPlay, 8> clips;
  ComputeClipsAndWeights(centerInfo, xiiVec2(pState->m_fLastValueX, pState->m_fLastValueY), clips, uiMaxWeightClip);

  PlayClips(ref_controller, centerInfo, pState, ref_graph, tDiff, clips, uiMaxWeightClip);
}

void xiiSampleBlendSpace2DAnimNode::ComputeClipsAndWeights(const xiiAnimController::AnimClipInfo& centerInfo, const xiiVec2& p, xiiDynamicArray<ClipToPlay>& clips, xiiUInt32& out_uiMaxWeightClip) const
{
  out_uiMaxWeightClip = 0;
  float fMaxWeight    = -1.0f;

  if (m_Clips.GetCount() == 1 && !centerInfo.m_hClip.IsValid())
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
      if (centerInfo.m_hClip.IsValid())
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
    if (centerInfo.m_hClip.IsValid())
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

void xiiSampleBlendSpace2DAnimNode::PlayClips(xiiAnimController& ref_controller, const xiiAnimController::AnimClipInfo& centerInfo, InstanceState* pState, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, xiiArrayPtr<ClipToPlay> clips, xiiUInt32 uiMaxWeightClip) const
{
  const bool  bLoop  = m_InLoop.GetBool(ref_graph, m_bLoop);
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

  xiiTime tAvgDuration = xiiTime::MakeZero();

  xiiHybridArray<xiiAnimPoseGeneratorCommandSampleTrack*, 8> pSampleTrack;
  pSampleTrack.SetCountUninitialized(clips.GetCount());

  xiiVec3   vRootMotion   = xiiVec3::MakeZero();
  xiiUInt32 uiNumAvgClips = 0;

  for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    const auto& c = clips[i];

    const xiiHashedString sClip = c.m_uiIndex >= 0xFF ? m_sCenterClip : m_Clips[c.m_uiIndex].m_sClip;

    const auto& clipInfo = ref_controller.GetAnimationClipInfo(sClip);

    xiiResourceLock<xiiAnimationClipResource> pClip(clipInfo.m_hClip, xiiResourceAcquireMode::BlockTillLoaded);

    if (c.m_uiIndex < 0xFF) // center clip should not contribute to the average time
    {
      ++uiNumAvgClips;
      tAvgDuration += pClip->GetDescriptor().GetDuration();
    }

    const void* pThis          = this;
    auto&       cmd            = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), i));
    cmd.m_hAnimationClip       = clipInfo.m_hClip;
    cmd.m_fNormalizedSamplePos = pClip->GetDescriptor().GetDuration().AsFloatInSeconds(); // will be combined with actual pos below

    pSampleTrack[i] = &cmd;
    vRootMotion += pClip->GetDescriptor().m_vConstantRootMotion * c.m_fWeight;
  }

  if (uiNumAvgClips > 0)
  {
    tAvgDuration = tAvgDuration / uiNumAvgClips;
  }

  tAvgDuration = xiiMath::Max(tAvgDuration, xiiTime::MakeFromMilliseconds(16));

  const xiiTime fPrevCenterPlaybackPos = pState->m_CenterPlaybackTime;
  const float   fPrevPlaybackPosNorm   = pState->m_fOtherPlaybackPosNorm;

  xiiAnimPoseEventTrackSampleMode eventSamplingCenter = xiiAnimPoseEventTrackSampleMode::OnlyBetween;
  xiiAnimPoseEventTrackSampleMode eventSampling       = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  const float fInvAvgDuration = 1.0f / tAvgDuration.AsFloatInSeconds();
  const float tDiffNorm       = tDiff.AsFloatInSeconds() * fInvAvgDuration;

  // now that we know the duration, we can finally update the playback state
  pState->m_fOtherPlaybackPosNorm += tDiffNorm * fSpeed;
  while (pState->m_fOtherPlaybackPosNorm >= 1.0f)
  {
    if (bLoop)
    {
      pState->m_fOtherPlaybackPosNorm -= 1.0f;
      m_OutOnStarted.SetTriggered(ref_graph);
      eventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
    else
    {
      pState->m_fOtherPlaybackPosNorm = 1.0f;
      pState->m_bPlaying              = false;
      m_OutOnFinished.SetTriggered(ref_graph);
      break;
    }
  }

  UpdateCenterClipPlaybackTime(centerInfo, pState, ref_graph, tDiff, eventSamplingCenter);

  for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    if (pSampleTrack[i]->m_hAnimationClip == centerInfo.m_hClip)
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevCenterPlaybackPos.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_fNormalizedSamplePos         = pState->m_CenterPlaybackTime.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_EventSampling                = uiMaxWeightClip == i ? eventSamplingCenter : xiiAnimPoseEventTrackSampleMode::None;
    }
    else
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevPlaybackPosNorm;
      pSampleTrack[i]->m_fNormalizedSamplePos         = pState->m_fOtherPlaybackPosNorm;
      pSampleTrack[i]->m_EventSampling                = uiMaxWeightClip == i ? eventSampling : xiiAnimPoseEventTrackSampleMode::None;
    }
  }

  xiiAnimGraphPinDataLocalTransforms* pOutputTransform = ref_controller.AddPinDataLocalTransforms();

  if (m_bApplyRootMotion)
  {
    pOutputTransform->m_bUseRootMotion = true;

    const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

    pOutputTransform->m_vRootMotion = tDiff.AsFloatInSeconds() * vRootMotion * fSpeed;
  }

  if (clips.GetCount() == 1)
  {
    pOutputTransform->m_CommandID = pSampleTrack[0]->GetCommandID();
  }
  else
  {
    auto& cmdCmb                  = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    cmdCmb.m_InputWeights.SetCountUninitialized(clips.GetCount());
    cmdCmb.m_Inputs.SetCountUninitialized(clips.GetCount());

    for (xiiUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      cmdCmb.m_InputWeights[i] = clips[i].m_fWeight;
      cmdCmb.m_Inputs[i]       = pSampleTrack[i]->GetCommandID();
    }
  }

  m_OutPose.SetPose(ref_graph, pOutputTransform);
}

void xiiSampleBlendSpace2DAnimNode::UpdateCenterClipPlaybackTime(const xiiAnimController::AnimClipInfo& centerInfo, InstanceState* pState, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, xiiAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const
{
  const float fSpeed = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed));

  if (centerInfo.m_hClip.IsValid())
  {
    xiiResourceLock<xiiAnimationClipResource> pClip(centerInfo.m_hClip, xiiResourceAcquireMode::BlockTillLoaded);

    const xiiTime tDur = pClip->GetDescriptor().GetDuration();

    pState->m_CenterPlaybackTime += tDiff * fSpeed;

    // always loop the center clip
    while (pState->m_CenterPlaybackTime > tDur)
    {
      pState->m_CenterPlaybackTime -= tDur;
      out_eventSamplingCenter = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
  }
}

bool xiiSampleBlendSpace2DAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}
