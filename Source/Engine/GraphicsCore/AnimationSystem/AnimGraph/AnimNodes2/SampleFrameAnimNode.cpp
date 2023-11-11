#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <GraphicsCore/AnimationSystem/AnimPoseGenerator.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSampleFrameAnimNode, 1, xiiRTTIDefaultAllocator<xiiSampleFrameAnimNode>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
      XII_MEMBER_PROPERTY("NormPos", m_fNormalizedSamplePosition)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, 1.0f)),

      XII_MEMBER_PROPERTY("InNormPos", m_InNormalizedSamplePosition)->AddAttributes(new xiiHiddenAttribute()),
      XII_MEMBER_PROPERTY("InAbsPos", m_InAbsoluteSamplePosition)->AddAttributes(new xiiHiddenAttribute()),

      XII_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new xiiHiddenAttribute()),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_ATTRIBUTES
    {
      new xiiCategoryAttribute("Pose Generation"),
      new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Blue)),
      new xiiTitleAttribute("Sample Frame: '{Clip}'"),
    }
    XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSampleFrameAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sClip;
  stream << m_fNormalizedSamplePosition;

  XII_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSampleFrameAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sClip;
  stream >> m_fNormalizedSamplePosition;

  XII_SUCCEED_OR_RETURN(m_InNormalizedSamplePosition.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InAbsoluteSamplePosition.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSampleFrameAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  const auto& clip = ref_controller.GetAnimationClipInfo(m_sClip);

  if (clip.m_hClip.IsValid())
  {
    xiiResourceLock<xiiAnimationClipResource> pAnimClip(clip.m_hClip, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pAnimClip.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    float fNormPos = fNormPos = m_InNormalizedSamplePosition.GetNumber(ref_graph, m_fNormalizedSamplePosition);

    if (m_InAbsoluteSamplePosition.IsConnected())
    {
      const xiiTime tDuration    = pAnimClip->GetDescriptor().GetDuration();
      const float   fInvDuration = 1.0f / tDuration.AsFloatInSeconds();
      fNormPos                   = m_InAbsoluteSamplePosition.GetNumber(ref_graph) * fInvDuration;
    }

    fNormPos = xiiMath::Clamp(fNormPos, 0.0f, 1.0f);

    const void* pThis = this;
    auto&       cmd   = ref_controller.GetPoseGenerator().AllocCommandSampleTrack(xiiHashingUtils::xxHash32(&pThis, sizeof(pThis)));

    cmd.m_hAnimationClip               = clip.m_hClip;
    cmd.m_fPreviousNormalizedSamplePos = fNormPos;
    cmd.m_fNormalizedSamplePos         = fNormPos;
    cmd.m_EventSampling                = xiiAnimPoseEventTrackSampleMode::None;

    {
      xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights       = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID      = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
  else
  {
    const void* pThis = this;
    auto&       cmd   = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_pWeights       = nullptr;
      pLocalTransforms->m_bUseRootMotion = false;
      pLocalTransforms->m_fOverallWeight = 1.0f;
      pLocalTransforms->m_CommandID      = cmd.GetCommandID();

      m_OutPose.SetPose(ref_graph, pLocalTransforms);
    }
  }
}

void xiiSampleFrameAnimNode::SetClip(const char* szClip)
{
  m_sClip.Assign(szClip);
}

const char* xiiSampleFrameAnimNode::GetClip() const
{
  return m_sClip.GetData();
}
