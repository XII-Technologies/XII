#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipSequenceAnimNode.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSampleAnimClipSequenceAnimNode, 1, xiiRTTIDefaultAllocator<xiiSampleAnimClipSequenceAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, {})),
      XII_MEMBER_PROPERTY("Loop", m_bLoop),
      //XII_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      XII_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      XII_ARRAY_ACCESSOR_PROPERTY("MiddleClips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      XII_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      XII_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new xiiHiddenAttribute()),

      XII_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnMiddleStarted", m_OutOnMiddleStarted)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnEndStarted", m_OutOnEndStarted)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Pose Generation"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
      new xiiTitleAttribute("Sample Sequence: '{StartClip}' '{Clip}' '{EndClip}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSampleAnimClipSequenceAnimNode::xiiSampleAnimClipSequenceAnimNode()  = default;
xiiSampleAnimClipSequenceAnimNode::~xiiSampleAnimClipSequenceAnimNode() = default;

xiiResult xiiSampleAnimClipSequenceAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sStartClip;
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));
  stream << m_sEndClip;
  stream << m_bApplyRootMotion;
  stream << m_bLoop;
  stream << m_fPlaybackSpeed;

  XII_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnEndStarted.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSampleAnimClipSequenceAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sStartClip;
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));
  stream >> m_sEndClip;
  stream >> m_bApplyRootMotion;
  stream >> m_bLoop;
  stream >> m_fPlaybackSpeed;

  XII_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnMiddleStarted.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnEndStarted.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return XII_SUCCESS;
}

xiiUInt32 xiiSampleAnimClipSequenceAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* xiiSampleAnimClipSequenceAnimNode::Clips_GetValue(xiiUInt32 uiIndex) const
{
  return m_Clips[uiIndex];
}

void xiiSampleAnimClipSequenceAnimNode::Clips_SetValue(xiiUInt32 uiIndex, const char* szValue)
{
  m_Clips[uiIndex].Assign(szValue);
}

void xiiSampleAnimClipSequenceAnimNode::Clips_Insert(xiiUInt32 uiIndex, const char* szValue)
{
  xiiHashedString s;
  s.Assign(szValue);
  m_Clips.Insert(s, uiIndex);
}

void xiiSampleAnimClipSequenceAnimNode::Clips_Remove(xiiUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}

void xiiSampleAnimClipSequenceAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  InstanceState* pState = ref_graph.GetAnimNodeInstanceData<InstanceState>(*this);

  if ((!m_InStart.IsConnected() && pState->m_uiState == 0) || m_InStart.IsTriggered(ref_graph))
  {
    pState->m_PlaybackTime = xiiTime::Zero();
    pState->m_uiState      = 1;
  }

  if (pState->m_uiState == 0)
    return;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  // currently we only support playing clips forwards
  const float fPlaySpeed = xiiMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  xiiTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  xiiAnimationClipResourceHandle hCurClip;
  xiiTime                        tCurDuration;

  while (pState->m_uiState != 0)
  {
    if (pState->m_uiState == 1)
    {
      const auto& startClip = ref_controller.GetAnimationClipInfo(m_sStartClip);

      if (!startClip.m_hClip.IsValid())
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_uiState = 2;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      xiiResourceLock<xiiAnimationClipResource> pAnimClip(startClip.m_hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
      {
        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }

        pState->m_uiState = 2;
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      XII_ASSERT_DEBUG(tCurDuration >= xiiTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnMiddleStarted.SetTriggered(ref_graph);
        tPrevSamplePos = xiiTime::Zero();
        pState->m_PlaybackTime -= tCurDuration;
        pState->m_uiState = 2;

        if (!m_Clips.IsEmpty())
        {
          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        continue;
      }

      hCurClip = startClip.m_hClip;
      break;
    }

    if (pState->m_uiState == 2)
    {
      const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_Clips[pState->m_uiMiddleClipIdx]);

      if (m_Clips.IsEmpty() || !clipInfo.m_hClip.IsValid())
      {
        pState->m_uiState = 3;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      xiiResourceLock<xiiAnimationClipResource> pAnimClip(clipInfo.m_hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
      {
        pState->m_uiState = 3;
        m_OutOnEndStarted.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      XII_ASSERT_DEBUG(tCurDuration >= xiiTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        tPrevSamplePos = xiiTime::Zero();
        pState->m_PlaybackTime -= tCurDuration;

        if (bLoop)
        {
          m_OutOnMiddleStarted.SetTriggered(ref_graph);
          pState->m_uiState = 2;

          pState->m_uiMiddleClipIdx = m_ClipIndexPin.GetNumber(ref_graph, 0xFF);
          if (pState->m_uiMiddleClipIdx >= m_Clips.GetCount())
          {
            pState->m_uiMiddleClipIdx = pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount());
          }
        }
        else
        {
          m_OutOnEndStarted.SetTriggered(ref_graph);
          pState->m_uiState = 3;
        }
        continue;
      }

      hCurClip = clipInfo.m_hClip;
      break;
    }

    if (pState->m_uiState == 3)
    {
      const auto& endClip = ref_controller.GetAnimationClipInfo(m_sEndClip);

      if (!endClip.m_hClip.IsValid())
      {
        pState->m_uiState = 0;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      xiiResourceLock<xiiAnimationClipResource> pAnimClip(endClip.m_hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pAnimClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
      {
        pState->m_uiState = 0;
        m_OutOnFinished.SetTriggered(ref_graph);
        continue;
      }

      tCurDuration = pAnimClip->GetDescriptor().GetDuration();
      XII_ASSERT_DEBUG(tCurDuration >= xiiTime::MakeFromMilliseconds(5), "Too short clip");

      if (pState->m_PlaybackTime >= tCurDuration)
      {
        // TODO: sample anim events of previous clip
        m_OutOnFinished.SetTriggered(ref_graph);
        pState->m_uiState = 0;
        continue;
      }

      hCurClip = endClip.m_hClip;
      break;
    }
  }

  if (!hCurClip.IsValid())
    return;

  const float fInvDuration = 1.0f / tCurDuration.AsFloatInSeconds();

  const void* pThis   = this;
  auto&       cmd     = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  cmd.m_hAnimationClip               = hCurClip;
  cmd.m_fPreviousNormalizedSamplePos = xiiMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos         = xiiMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  {
    xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights       = nullptr;
    pLocalTransforms->m_bUseRootMotion = false; // m_bApplyRootMotion;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    // pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed;
    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
}

void xiiSampleAnimClipSequenceAnimNode::SetStartClip(const char* szClip)
{
  m_sStartClip.Assign(szClip);
}

const char* xiiSampleAnimClipSequenceAnimNode::GetStartClip() const
{
  return m_sStartClip;
}

void xiiSampleAnimClipSequenceAnimNode::SetEndClip(const char* szClip)
{
  m_sEndClip.Assign(szClip);
}

const char* xiiSampleAnimClipSequenceAnimNode::GetEndClip() const
{
  return m_sEndClip;
}

bool xiiSampleAnimClipSequenceAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}
