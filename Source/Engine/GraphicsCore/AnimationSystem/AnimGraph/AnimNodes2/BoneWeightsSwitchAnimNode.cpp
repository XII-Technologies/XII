#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes2/BoneWeightsSwitchAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSwitchBoneWeightsAnimNode, 1, xiiRTTIDefaultAllocator<xiiSwitchBoneWeightsAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("InIndex", m_InIndex)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("WeightsCount", m_uiWeightsCount)->AddAttributes(new xiiNoTemporaryTransactionsAttribute(), new xiiDynamicPinAttribute(), new xiiDefaultValueAttribute(2)),
    XII_ARRAY_MEMBER_PROPERTY("InWeights", m_InWeights)->AddAttributes(new xiiHiddenAttribute(), new xiiDynamicPinAttribute("WeightsCount")),
    XII_MEMBER_PROPERTY("OutWeights", m_OutWeights)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Weights"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Teal)),
    new xiiTitleAttribute("Bone Weights Switch"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiSwitchBoneWeightsAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_uiWeightsCount;

  XII_SUCCEED_OR_RETURN(m_InIndex.Serialize(stream));
  XII_SUCCEED_OR_RETURN(stream.WriteArray(m_InWeights));
  XII_SUCCEED_OR_RETURN(m_OutWeights.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiSwitchBoneWeightsAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  XII_IGNORE_UNUSED(version);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_uiWeightsCount;

  XII_SUCCEED_OR_RETURN(m_InIndex.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(stream.ReadArray(m_InWeights));
  XII_SUCCEED_OR_RETURN(m_OutWeights.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiSwitchBoneWeightsAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  if (!m_OutWeights.IsConnected() || !m_InIndex.IsConnected() || m_InWeights.IsEmpty())
    return;

  const xiiInt32 iIndex = xiiMath::Clamp((xiiInt32)m_InIndex.GetNumber(ref_graph), 0, (xiiInt32)m_InWeights.GetCount() - 1);

  if (!m_InWeights[iIndex].IsConnected())
    return;

  m_OutWeights.SetWeights(ref_graph, m_InWeights[iIndex].GetWeights(ref_controller, ref_graph));
}
