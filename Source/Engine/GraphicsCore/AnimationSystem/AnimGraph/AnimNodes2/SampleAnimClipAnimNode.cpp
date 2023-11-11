#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/SampleAnimClipAnimNode.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSampleAnimClipAnimNode, 1, xiiRTTIDefaultAllocator<xiiSampleAnimClipAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Loop", m_bLoop)->AddAttributes(new xiiDefaultValueAttribute(true)),
      XII_MEMBER_PROPERTY("PlaybackSpeed", m_fPlaybackSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, {})),
      XII_MEMBER_PROPERTY("ApplyRootMotion", m_bApplyRootMotion),
      XII_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),

      XII_MEMBER_PROPERTY("InStart", m_InStart)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InLoop", m_InLoop)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InSpeed", m_InSpeed)->AddAttributes(new xiiHiddenAttribute()),

      XII_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnStarted", m_OutOnStarted)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("OutOnFinished", m_OutOnFinished)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Pose Generation"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
      new xiiTitleAttribute("Sample Clip: '{Clip}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSampleAnimClipAnimNode::xiiSampleAnimClipAnimNode()  = default;
xiiSampleAnimClipAnimNode::~xiiSampleAnimClipAnimNode() = default;

xiiResult xiiSampleAnimClipAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_bLoop;
  stream << m_bApplyRootMotion;
  stream << m_fPlaybackSpeed;

  XII_SUCCEED_OR_RETURN(m_InStart.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnStarted.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSampleAnimClipAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
  stream >> m_bLoop;
  stream >> m_bApplyRootMotion;
  stream >> m_fPlaybackSpeed;

  XII_SUCCEED_OR_RETURN(m_InStart.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InLoop.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InSpeed.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnStarted.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFinished.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSampleAnimClipAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  const auto& clipInfo = ref_controller.GetAnimationClipInfo(m_sClip);

  if (!clipInfo.m_hClip.IsValid() || !m_OutPose.IsConnected())
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

  xiiResourceLock<xiiAnimationClipResource> pAnimClip(clipInfo.m_hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  const xiiTime tDuration    = pAnimClip->GetDescriptor().GetDuration();
  const float   fInvDuration = 1.0f / tDuration.AsFloatInSeconds();

  // currently we only support playing clips forwards
  const float fPlaySpeed = xiiMath::Max(0.0f, static_cast<float>(m_InSpeed.GetNumber(ref_graph, m_fPlaybackSpeed)));

  const xiiTime tPrevSamplePos = pState->m_PlaybackTime;
  pState->m_PlaybackTime += tDiff * fPlaySpeed;

  const bool bLoop = m_InLoop.GetBool(ref_graph, m_bLoop);

  const void* pThis   = this;
  auto&       cmd     = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_EventSampling = xiiAnimPoseEventTrackSampleMode::OnlyBetween;

  if (bLoop && pState->m_PlaybackTime > tDuration)
  {
    pState->m_PlaybackTime -= tDuration;
    cmd.m_EventSampling = xiiAnimPoseEventTrackSampleMode::LoopAtEnd;
    m_OutOnStarted.SetTriggered(ref_graph);
  }

  cmd.m_hAnimationClip               = clipInfo.m_hClip;
  cmd.m_fPreviousNormalizedSamplePos = xiiMath::Clamp(tPrevSamplePos.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);
  cmd.m_fNormalizedSamplePos         = xiiMath::Clamp(pState->m_PlaybackTime.AsFloatInSeconds() * fInvDuration, 0.0f, 1.0f);

  {
    xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

    pLocalTransforms->m_pWeights       = nullptr;
    pLocalTransforms->m_bUseRootMotion = m_bApplyRootMotion;
    pLocalTransforms->m_fOverallWeight = 1.0f;
    pLocalTransforms->m_vRootMotion    = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * fPlaySpeed;
    pLocalTransforms->m_CommandID      = cmd.GetCommandID();

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }

  if (cmd.m_fNormalizedSamplePos >= 1.0f && !bLoop)
  {
    m_OutOnFinished.SetTriggered(ref_graph);
    pState->m_bPlaying = false;
  }
}

void xiiSampleAnimClipAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* xiiSampleAnimClipAnimNode::GetClip() const
{
  return m_sClip.GetData();
}

bool xiiSampleAnimClipAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceState>();
  return true;
}
