#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/LerpPosesAnimNode.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiLerpPosesAnimNode, 1, xiiRTTIDefaultAllocator<xiiLerpPosesAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Lerp", m_fLerp)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 3.0f)),
    XII_MEMBER_PROPERTY("InLerp", m_InLerp)->AddAttributes(new xiiHiddenAttribute),
    XII_MEMBER_PROPERTY("PosesCount", m_uiPosesCount)->AddAttributes(new xiiNoTemporaryTransactionsAttribute(), new xiiDynamicPinAttribute(), new xiiDefaultValueAttribute(2)),
    XII_ARRAY_MEMBER_PROPERTY("InPoses", m_InPoses)->AddAttributes(new xiiHiddenAttribute(), new xiiDynamicPinAttribute("PosesCount")),
    XII_MEMBER_PROPERTY("OutPose", m_OutPose)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Pose Blending"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Violet)),
    new xiiTitleAttribute("Lerp Poses"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiLerpPosesAnimNode::xiiLerpPosesAnimNode()  = default;
xiiLerpPosesAnimNode::~xiiLerpPosesAnimNode() = default;

xiiResult xiiLerpPosesAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fLerp;
  stream << m_uiPosesCount;

  XII_SUCCEED_OR_RETURN(m_InLerp.Serialize(stream));
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_InPoses));
  XII_SUCCEED_OR_RETURN(m_OutPose.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiLerpPosesAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fLerp;
  stream >> m_uiPosesCount;

  XII_SUCCEED_OR_RETURN(m_InLerp.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_InPoses));
  XII_SUCCEED_OR_RETURN(m_OutPose.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiLerpPosesAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_OutPose.IsConnected())
    return;

  xiiHybridArray<const xiiAnimGraphLocalPoseInputPin*, 12> pPins;
  for (xiiUInt32 i = 0; i < m_InPoses.GetCount(); ++i)
  {
    pPins.PushBack(&m_InPoses[i]);
  }

  // duplicate pin connections to fill up holes
  for (xiiUInt32 i = 1; i < pPins.GetCount(); ++i)
  {
    if (!pPins[i]->IsConnected())
      pPins[i] = pPins[i - 1];
  }
  for (xiiUInt32 i = pPins.GetCount(); i > 1; --i)
  {
    if (!pPins[i - 2]->IsConnected())
      pPins[i - 2] = pPins[i - 1];
  }

  if (pPins.IsEmpty() || !pPins[0]->IsConnected())
  {
    // this can only be the case if no pin is connected, at all
    return;
  }

  const float fIndex = xiiMath::Clamp((float)m_InLerp.GetNumber(ref_graph, m_fLerp), 0.0f, (float)pPins.GetCount() - 1.0f);

  if (xiiMath::Fraction(fIndex) == 0.0f)
  {
    const xiiAnimGraphLocalPoseInputPin* pPinToForward  = pPins[(xiiInt32)xiiMath::Trunc(fIndex)];
    xiiAnimGraphPinDataLocalTransforms*  pDataToForward = pPinToForward->GetPose(ref_controller, ref_graph);

    xiiAnimGraphPinDataLocalTransforms* pLocalTransforms = ref_controller.AddPinDataLocalTransforms();
    pLocalTransforms->m_CommandID                        = pDataToForward->m_CommandID;
    pLocalTransforms->m_pWeights                         = pDataToForward->m_pWeights;
    pLocalTransforms->m_fOverallWeight                   = pDataToForward->m_fOverallWeight;
    pLocalTransforms->m_vRootMotion                      = pDataToForward->m_vRootMotion;
    pLocalTransforms->m_bUseRootMotion                   = pDataToForward->m_bUseRootMotion;

    m_OutPose.SetPose(ref_graph, pLocalTransforms);
  }
  else
  {
    xiiAnimGraphPinDataLocalTransforms* pPinData = ref_controller.AddPinDataLocalTransforms();

    const float fLerp = xiiMath::Fraction(fIndex);

    auto pPose0 = pPins[(xiiInt32)xiiMath::Trunc(fIndex)]->GetPose(ref_controller, ref_graph);
    auto pPose1 = pPins[(xiiInt32)xiiMath::Trunc(fIndex) + 1]->GetPose(ref_controller, ref_graph);

    auto& cmd = ref_controller.GetPoseGenerator().AllocCommandCombinePoses();
    cmd.m_InputWeights.SetCount(2);
    cmd.m_InputWeights[0] = 1.0f - fLerp;
    cmd.m_InputWeights[1] = fLerp;
    cmd.m_Inputs.SetCount(2);
    cmd.m_Inputs[0] = pPose0->m_CommandID;
    cmd.m_Inputs[1] = pPose1->m_CommandID;

    pPinData->m_CommandID      = cmd.GetCommandID();
    pPinData->m_bUseRootMotion = pPose0->m_bUseRootMotion || pPose1->m_bUseRootMotion;
    pPinData->m_vRootMotion    = xiiMath::Lerp(pPose0->m_vRootMotion, pPose1->m_vRootMotion, fLerp);

    m_OutPose.SetPose(ref_graph, pPinData);
  }
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes2_LerpPosesAnimNode);
