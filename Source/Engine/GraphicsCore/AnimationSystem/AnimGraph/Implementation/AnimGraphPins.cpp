#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimController.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphPin, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("NumConnections", m_uiNumConnections)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphInputPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphOutputPin, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiResult xiiAnimGraphPin::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_iPinIndex;
  inout_stream << m_uiNumConnections;
  return XII_SUCCESS;
}

xiiResult xiiAnimGraphPin::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_iPinIndex;
  inout_stream >> m_uiNumConnections;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphTriggerInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphTriggerInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphTriggerOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphTriggerOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiAnimGraphTriggerOutputPin::SetTriggered(xiiAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[xiiAnimGraphPin::Trigger][m_iPinIndex];


  const xiiInt8 offset = +1; // bTriggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_pTriggerInputPinStates[idx] += offset;
  }
}

bool xiiAnimGraphTriggerInputPin::IsTriggered(xiiAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return ref_graph.m_pTriggerInputPinStates[m_iPinIndex] > 0;
}

bool xiiAnimGraphTriggerInputPin::AreAllTriggered(xiiAnimGraphInstance& ref_graph) const
{
  return ref_graph.m_pTriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphNumberInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphNumberInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphNumberOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphNumberOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

double xiiAnimGraphNumberInputPin::GetNumber(xiiAnimGraphInstance& ref_graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return ref_graph.m_pNumberInputPinStates[m_iPinIndex];
}

void xiiAnimGraphNumberOutputPin::SetNumber(xiiAnimGraphInstance& ref_graph, double value) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[xiiAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_pNumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphBoolInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphBoolInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphBoolOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphBoolOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

bool xiiAnimGraphBoolInputPin::GetBool(xiiAnimGraphInstance& ref_graph, bool bFallback /*= false */) const
{
  if (m_iPinIndex < 0)
    return bFallback;

  return ref_graph.m_pBoolInputPinStates[m_iPinIndex];
}

void xiiAnimGraphBoolOutputPin::SetBool(xiiAnimGraphInstance& ref_graph, bool bValue) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[xiiAnimGraphPin::Bool][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_pBoolInputPinStates[idx] = bValue;
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphBoneWeightsInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphBoneWeightsInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphBoneWeightsOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphBoneWeightsOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimGraphPinDataBoneWeights* xiiAnimGraphBoneWeightsInputPin::GetWeights(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_pBoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_controller.m_PinDataBoneWeights[ref_graph.m_pBoneWeightInputPinStates[m_iPinIndex]];
}

void xiiAnimGraphBoneWeightsOutputPin::SetWeights(xiiAnimGraphInstance& ref_graph, xiiAnimGraphPinDataBoneWeights* pWeights) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[xiiAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_pBoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphLocalPoseInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphLocalPoseInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphLocalPoseMultiInputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphLocalPoseMultiInputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphLocalPoseOutputPin, 1, xiiRTTIDefaultAllocator<xiiAnimGraphLocalPoseOutputPin>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiAnimGraphPinDataLocalTransforms* xiiAnimGraphLocalPoseInputPin::GetPose(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &ref_controller.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void xiiAnimGraphLocalPoseMultiInputPin::GetPoses(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiDynamicArray<xiiAnimGraphPinDataLocalTransforms*>& out_poses) const
{
  out_poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_poses.SetCountUninitialized(ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount());
  for (xiiUInt32 i = 0; i < ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount(); ++i)
  {
    out_poses[i] = &ref_controller.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][i]];
  }
}

void xiiAnimGraphLocalPoseOutputPin::SetPose(xiiAnimGraphInstance& ref_graph, xiiAnimGraphPinDataLocalTransforms* pPose) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[xiiAnimGraphPin::LocalPose][m_iPinIndex];

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

xiiAnimGraphPinDataModelTransforms* xiiAnimGraphModelPoseInputPin::GetPose(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_pModelPoseInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_controller.m_PinDataModelTransforms[ref_graph.m_pModelPoseInputPinStates[m_iPinIndex]];
}

void xiiAnimGraphModelPoseOutputPin::SetPose(xiiAnimGraphInstance& ref_graph, xiiAnimGraphPinDataModelTransforms* pPose) const
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_pAnimGraph->m_OutputPinToInputPinMapping[xiiAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (xiiUInt16 idx : map)
  {
    ref_graph.m_pModelPoseInputPinStates[idx] = pPose->m_uiOwnIndex;
  }
}


XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);
