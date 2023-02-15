#include <RendererCore/RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiControllerInputAnimNode, 1, xiiRTTIDefaultAllocator<xiiControllerInputAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LeftTrigger", m_LeftTrigger)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("LeftShoulder", m_LeftShoulder)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("LeftStickX", m_LeftStickX)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("LeftStickY", m_LeftStickY)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("PadLeft", m_PadLeft)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PadRight", m_PadRight)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PadUp", m_PadUp)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PadDown", m_PadDown)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("RightTrigger", m_RightTrigger)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RightShoulder", m_RightShoulder)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("RightStickX", m_RightStickX)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RightStickY", m_RightStickY)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("ButtonA", m_ButtonA)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ButtonB", m_ButtonB)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ButtonX", m_ButtonX)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ButtonY", m_ButtonY)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Pink)),
    new xiiTitleAttribute("XBox Controller"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiControllerInputAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_ButtonA.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ButtonB.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ButtonX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_ButtonY.Serialize(stream));

  XII_SUCCEED_OR_RETURN(m_LeftStickX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LeftStickY.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightStickX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightStickY.Serialize(stream));

  XII_SUCCEED_OR_RETURN(m_LeftTrigger.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightTrigger.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_LeftShoulder.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightShoulder.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadLeft.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadRight.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadUp.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadDown.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiControllerInputAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_ButtonA.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ButtonB.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ButtonX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_ButtonY.Deserialize(stream));

  XII_SUCCEED_OR_RETURN(m_LeftStickX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LeftStickY.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightStickX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightStickY.Deserialize(stream));

  XII_SUCCEED_OR_RETURN(m_LeftTrigger.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightTrigger.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_LeftShoulder.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_RightShoulder.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadLeft.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadRight.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadUp.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_PadDown.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiControllerInputAnimNode::Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget)
{
  float fValue1 = 0.0f;
  float fValue2 = 0.0f;

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_NegX, &fValue1);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_PosX, &fValue2);
  m_LeftStickX.SetNumber(graph, -fValue1 + fValue2);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_NegY, &fValue1);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_PosY, &fValue2);
  m_LeftStickY.SetNumber(graph, -fValue1 + fValue2);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_NegX, &fValue1);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_PosX, &fValue2);
  m_RightStickX.SetNumber(graph, -fValue1 + fValue2);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_NegY, &fValue1);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_PosY, &fValue2);
  m_RightStickY.SetNumber(graph, -fValue1 + fValue2);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonA, &fValue1);
  m_ButtonA.SetTriggered(graph, fValue1 > 0);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonB, &fValue1);
  m_ButtonB.SetTriggered(graph, fValue1 > 0);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonX, &fValue1);
  m_ButtonX.SetTriggered(graph, fValue1 > 0);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonY, &fValue1);
  m_ButtonY.SetTriggered(graph, fValue1 > 0);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftShoulder, &fValue1);
  m_LeftShoulder.SetTriggered(graph, fValue1 > 0);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftTrigger, &fValue1);
  m_LeftTrigger.SetNumber(graph, fValue1);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightShoulder, &fValue1);
  m_RightShoulder.SetTriggered(graph, fValue1 > 0);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightTrigger, &fValue1);
  m_RightTrigger.SetNumber(graph, fValue1);

  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadLeft, &fValue1);
  m_PadLeft.SetTriggered(graph, fValue1 > 0);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadRight, &fValue1);
  m_PadRight.SetTriggered(graph, fValue1 > 0);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadUp, &fValue1);
  m_PadUp.SetTriggered(graph, fValue1 > 0);
  xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadDown, &fValue1);
  m_PadDown.SetTriggered(graph, fValue1 > 0);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
