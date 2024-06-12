#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/SampleBlendSpace1DAnimNode.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAnimationClip1D, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiAnimationClip1D>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    XII_MEMBER_PROPERTY("Position", m_fPosition),
    XII_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSampleBlendSpace1DAnimNode, 1, xiiRTTIDefaultAllocator<xiiSampleBlendSpace1DAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, {})),
      XII_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      XII_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

      XII_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new xiiHiddenAttribute()),

      XII_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Pose Generation"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
      new xiiTitleAttribute("BlendSpace 1D: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiAnimationClip1D::SetAnimationFile(const char* szFile)
{
  m_sClip.Assign(szFile);
}

const char* xiiAnimationClip1D::GetAnimationFile() const
{
  return m_sClip;
}

xiiSampleBlendSpace1DAnimNode::xiiSampleBlendSpace1DAnimNode()  = default;
xiiSampleBlendSpace1DAnimNode::~xiiSampleBlendSpace1DAnimNode() = default;

xiiResult xiiSampleBlendSpace1DAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_Clips.GetCount();
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_sClip;
    stream << m_Clips[i].m_fPosition;
    stream << m_Clips[i].m_fSpeed;
  }

  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;

  XII_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSampleBlendSpace1DAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  xiiUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (xiiUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_sClip;
    stream >> m_Clips[i].m_fPosition;
    stream >> m_Clips[i].m_fSpeed;
  }

  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;

  XII_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSampleBlendSpace1DAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected() || !m_InLerp.IsConnected() || m_Clips.IsEmpty())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && !pState->m_bPlaying) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = xiiTime::MakeZero();
    pState->m_bPlaying     = true;

    m_OutOnStarted.SetTriggered(ref_graph);
  }

  if (!pState->m_bPlaying)
    return;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  xiiUInt32 uiClip1 = 0;
  xiiUInt32 uiClip2 = 0;

  const float fLerpPos = (float)m_InLerp.GetNumber(ref_graph);

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

  const auto& clip1 = ref_controller.GetAnimationClipInfo(m_Clips[uiClip1].m_sClip);
  const auto& clip2 = ref_controller.GetAnimationClipInfo(m_Clips[uiClip2].m_sClip);

  if (!clip1.m_hClip.IsValid() || !clip2.m_hClip.IsValid())
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

  xiiResourceLock<xiiAnimationClipResource> pAnimClip1(clip1.m_hClip, xiiResourceAcquireMode::BlockTillLoaded);
  xiiResourceLock<xiiAnimationClipResource> pAnimClip2(clip2.m_hClip, xiiResourceAcquireMode::BlockTillLoaded);

  if (pAnimClip1.GetAcquireResult() != xiiResourceAcquireResult::Final || pAnimClip2.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  const float fAvgClipSpeed = xiiMath::Lerp(m_Clips[uiClip1].m_fSpeed, m_Clips[uiClip2].m_fSpeed, fLerpFactor);
  const float fSpeed        = static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)) * fAvgClipSpeed;

  const auto& animDesc1 = pAnimClip1->GetDescriptor();
  const auto& animDesc2 = pAnimClip2->GetDescriptor();

  const xiiTime avgDuration  = xiiMath::Lerp(animDesc1.GetDuration(), animDesc2.GetDuration(), fLerpFactor);
  const float   fInvDuration = 1.0f / avgDuration.AsFloatInSeconds();

  const xiiTime tPrevPlayback = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fSpeed;

  xiiAnimPoseEventTrackSampleMode eventSampling = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  if (pState->m_PlaybackTime >= avgDuration)
  {
    if (bLoop)
    {
      pState->m_PlaybackTime -= avgDuration;
      eventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
      m_OutOnStarted.SetTriggered(ref_graph);
    }
    else
    {
      pState->m_PlaybackTime = avgDuration;
      pState->m_bPlaying     = false;
      m_OutOnFinished.SetTriggered(ref_graph);
    }
  }

  xiiAnimGraphPinDataLocalTransforms* pOutputTransform = ref_controller.AddPinDataLocalTransforms();

  auto& poseGen = ref_controller.GetPoseGenerator();

  if (clip1.m_hClip == clip2.m_hClip)
  {
    const void* pThis                  = this;
    auto&       cmd                    = poseGen.AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
    cmd.m_hAnimationClip               = clip1.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
    cmd.m_fNormalizedSamplePos         = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;
    cmd.m_EventSampling                = eventSampling;

    pOutputTransform->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    auto& cmdCmb                  = poseGen.AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    // sample animation 1
    {
      const void* pThis                  = this;
      auto&       cmd                    = poseGen.AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
      cmd.m_hAnimationClip               = clip1.m_hClip;
      cmd.m_fPreviousNormalizedSamplePos = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
      cmd.m_fNormalizedSamplePos         = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;
      cmd.m_EventSampling                = fLerpFactor <= 0.5f ? eventSampling : xiiAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(1.0f - fLerpFactor);
    }

    // sample animation 2
    {
      const void* pThis                  = this;
      auto&       cmd                    = poseGen.AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis), 1));
      cmd.m_hAnimationClip               = clip2.m_hClip;
      cmd.m_fPreviousNormalizedSamplePos = tPrevPlayback.AsFloatInSeconds() * fInvDuration;
      cmd.m_fNormalizedSamplePos         = pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration;
      cmd.m_EventSampling                = fLerpFactor > 0.5f ? eventSampling : xiiAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(fLerpFactor);
    }
  }

  // send to output
  {
    if (m_bApplyRootMotion)
    {
      pOutputTransform->m_bUseRootMotion = true;

      pOutputTransform->m_vRootMotion = xiiMath::Lerp(animDesc1.m_vConstantRootMotion, animDesc2.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds() * fSpeed;
    }

    m_OutPose.SetPose(ref_graph, pOutputTransform);
  }
}

bool xiiSampleBlendSpace1DAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}
