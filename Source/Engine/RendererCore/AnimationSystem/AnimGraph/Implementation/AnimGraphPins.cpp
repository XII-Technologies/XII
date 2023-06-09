#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphPin>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("NumConnections", m_uiNumConnections)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiAnimGraphPin::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream << m_iPinIndex;
  ref_stream << m_uiNumConnections;
  return XII_SUCCESS;
}

xiiResult xiiAnimGraphPin::Deserialize(xiiStreamReader& ref_stream)
{
  ref_stream >> m_iPinIndex;
  ref_stream >> m_uiNumConnections;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphTriggerInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphTriggerInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphTriggerOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphTriggerOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiAnimGraphTriggerOutputPin::SetTriggered(xiiAnimGraph& ref_graph, bool bTriggered)
{
  if (m_iPinIndex < 0)
    return;

  if (!bTriggered)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[xiiAnimGraphPin::Trigger][m_iPinIndex];


  const xiiInt8 offset = bTriggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_TriggerInputPinStates[idx] += offset;
  }
}

bool xiiAnimGraphTriggerInputPin::IsTriggered(xiiAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return ref_graph.m_TriggerInputPinStates[m_iPinIndex] > 0;
}

bool xiiAnimGraphTriggerInputPin::AreAllTriggered(xiiAnimGraph& ref_graph) const
{
  return ref_graph.m_TriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphNumberInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphNumberInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphNumberOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphNumberOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double xiiAnimGraphNumberInputPin::GetNumber(xiiAnimGraph& ref_graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return ref_graph.m_NumberInputPinStates[m_iPinIndex];
}

void xiiAnimGraphNumberOutputPin::SetNumber(xiiAnimGraph& ref_graph, double value)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[xiiAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_NumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphBoneWeightsInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphBoneWeightsInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphBoneWeightsOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphBoneWeightsOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimGraphPinDataBoneWeights* xiiAnimGraphBoneWeightsInputPin::GetWeights(xiiAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_BoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_graph.m_PinDataBoneWeights[ref_graph.m_BoneWeightInputPinStates[m_iPinIndex]];
}

void xiiAnimGraphBoneWeightsOutputPin::SetWeights(xiiAnimGraph& ref_graph, xiiAnimGraphPinDataBoneWeights* pWeights)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[xiiAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_BoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphLocalPoseInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphLocalPoseInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphLocalPoseMultiInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphLocalPoseMultiInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphLocalPoseOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphLocalPoseOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimGraphPinDataLocalTransforms* xiiAnimGraphLocalPoseInputPin::GetPose(xiiAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &ref_graph.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void xiiAnimGraphLocalPoseMultiInputPin::GetPoses(xiiAnimGraph& ref_graph, xiiDynamicArray<xiiAnimGraphPinDataLocalTransforms*>& out_poses) const
{
  out_poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_poses.SetCountUninitialized(ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount());
  for (xiiUInt32 i = 0; i < ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount(); ++i)
  {
    out_poses[i] = &ref_graph.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][i]];
  }
}

void xiiAnimGraphLocalPoseOutputPin::SetPose(xiiAnimGraph& ref_graph, xiiAnimGraphPinDataLocalTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[xiiAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_LocalPoseInputPinStates[idx].PushBack(pPose->m_uiOwnIndex);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphModelPoseInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphModelPoseInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphModelPoseOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphModelPoseOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimGraphPinDataModelTransforms* xiiAnimGraphModelPoseInputPin::GetPose(xiiAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_ModelPoseInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_graph.m_PinDataModelTransforms[ref_graph.m_ModelPoseInputPinStates[m_iPinIndex]];
}

void xiiAnimGraphModelPoseOutputPin::SetPose(xiiAnimGraph& ref_graph, xiiAnimGraphPinDataModelTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[xiiAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_ModelPoseInputPinStates[idx] = pPose->m_uiOwnIndex;
  }
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);
