#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPoseResultAnimNode, 1, xiiRTTIDefaultAllocator<xiiPoseResultAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FadeDuration", m_FadeDuration)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::MakeFromMilliseconds(200)), new xiiClampValueAttribute(xiiTime::Zero(), xiiTime::MakeFromSeconds(10))),
    XII_MEMBER_PROPERTY("InPose", m_InPose)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("InTargetWeight", m_InTargetWeight)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("InFadeDuration", m_InFadeDuration)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutOnFadedOut", m_OutOnFadedOut)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutOnFadedIn", m_OutOnFadedIn)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("OutCurrentWeight", m_OutCurrentWeight)->AddAttributes(new xiiHiddenAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Output"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Grape)),
    new xiiTitleAttribute("Pose Result"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPoseResultAnimNode::xiiPoseResultAnimNode()  = default;
xiiPoseResultAnimNode::~xiiPoseResultAnimNode() = default;

xiiResult xiiPoseResultAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_FadeDuration;

  XII_SUCCEED_OR_RETURN(m_InPose.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InTargetWeight.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InFadeDuration.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_InWeights.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFadedOut.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFadedIn.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutCurrentWeight.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiPoseResultAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_FadeDuration;

  XII_SUCCEED_OR_RETURN(m_InPose.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InTargetWeight.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InFadeDuration.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_InWeights.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFadedOut.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutOnFadedIn.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutCurrentWeight.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiPoseResultAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_InPose.IsConnected())
    return;

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bWasInterpolating = pInstance->m_PlayTime < pInstance->m_EndTime;

  float fCurrentWeight = 1.0f;
  pInstance->m_PlayTime += tDiff;

  if (pInstance->m_PlayTime >= pInstance->m_EndTime)
  {
    fCurrentWeight = pInstance->m_fEndWeight;

    if (bWasInterpolating && fCurrentWeight <= 0.0f)
    {
      m_OutOnFadedOut.SetTriggered(ref_graph);
    }
    if (bWasInterpolating && fCurrentWeight >= 1.0f)
    {
      m_OutOnFadedIn.SetTriggered(ref_graph);
    }
  }
  else
  {
    const float f  = (float)(pInstance->m_PlayTime.GetSeconds() / pInstance->m_EndTime.GetSeconds());
    fCurrentWeight = xiiMath::Lerp(pInstance->m_fStartWeight, pInstance->m_fEndWeight, f);
  }

  const float fNewTargetWeight = m_InTargetWeight.GetNumber(ref_graph, 1.0f);

  if (pInstance->m_fEndWeight != fNewTargetWeight)
  {
    pInstance->m_fStartWeight = fCurrentWeight;
    pInstance->m_fEndWeight   = fNewTargetWeight;
    pInstance->m_PlayTime     = xiiTime::Zero();
    pInstance->m_EndTime      = xiiTime::MakeFromSeconds(m_InFadeDuration.GetNumber(ref_graph, m_FadeDuration.GetSeconds()));
  }

  m_OutCurrentWeight.SetNumber(ref_graph, fCurrentWeight);

  if (fCurrentWeight <= 0.0f)
    return;

  if (auto pCurrentLocalTransforms = m_InPose.GetPose(ref_controller, ref_graph))
  {
    if (pCurrentLocalTransforms->m_CommandID != xiiInvalidIndex)
    {
      xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_CommandID      = pCurrentLocalTransforms->m_CommandID;
      pLocalTransforms->m_pWeights       = m_InWeights.GetWeights(ref_controller, ref_graph);
      pLocalTransforms->m_fOverallWeight = pCurrentLocalTransforms->m_fOverallWeight * fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = pCurrentLocalTransforms->m_bUseRootMotion;
      pLocalTransforms->m_vRootMotion    = pCurrentLocalTransforms->m_vRootMotion;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
  else
  {
    // if we are active, but the incoming pose isn't valid (anymore), use a rest pose as placeholder
    // this assumes that many animations return to the rest pose and if they are played up to the very end before fading out
    // they can be faded out by using the rest pose

    const void* pThis = this;
    auto&       cmd   = ref_controller.GetPoseGenerator().AllocCommandRestPose();

    {
      xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();

      pLocalTransforms->m_CommandID      = cmd.GetCommandID();
      pLocalTransforms->m_pWeights       = m_InWeights.GetWeights(ref_controller, ref_graph);
      pLocalTransforms->m_fOverallWeight = fCurrentWeight;
      pLocalTransforms->m_bUseRootMotion = false;

      ref_controller.AddOutputLocalTransforms(pLocalTransforms);
    }
  }
}

bool xiiPoseResultAnimNode::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}
