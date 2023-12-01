#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Input/InputManager.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiControllerInputAnimNode, 1, xiiRTTIDefaultAllocator<xiiControllerInputAnimNode>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("LeftStickX", m_OutLeftStickX)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("LeftStickY", m_OutLeftStickY)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("RightStickX", m_OutRightStickX)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RightStickY", m_OutRightStickY)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("LeftTrigger", m_OutLeftTrigger)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RightTrigger", m_OutRightTrigger)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("ButtonA", m_OutButtonA)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ButtonB", m_OutButtonB)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ButtonX", m_OutButtonX)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("ButtonY", m_OutButtonY)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("LeftShoulder", m_OutLeftShoulder)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("RightShoulder", m_OutRightShoulder)->AddAttributes(new xiiHiddenAttribute()),

    XII_MEMBER_PROPERTY("PadLeft", m_OutPadLeft)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PadRight", m_OutPadRight)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PadUp", m_OutPadUp)->AddAttributes(new xiiHiddenAttribute()),
    XII_MEMBER_PROPERTY("PadDown", m_OutPadDown)->AddAttributes(new xiiHiddenAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Input"),
    new xiiColorAttribute(xiiColorScheme::DarkUI(xiiColorScheme::Pink)),
    new xiiTitleAttribute("Controller"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiControllerInputAnimNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_OutLeftStickX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutLeftStickY.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightStickX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightStickY.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutLeftTrigger.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightTrigger.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonA.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonB.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonX.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonY.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutLeftShoulder.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightShoulder.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadLeft.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadRight.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadUp.Serialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadDown.Serialize(stream));

  return XII_SUCCESS;
}

xiiResult xiiControllerInputAnimNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  XII_SUCCEED_OR_RETURN(m_OutLeftStickX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutLeftStickY.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightStickX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightStickY.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutLeftTrigger.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightTrigger.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonA.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonB.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonX.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutButtonY.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutLeftShoulder.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutRightShoulder.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadLeft.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadRight.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadUp.Deserialize(stream));
  XII_SUCCEED_OR_RETURN(m_OutPadDown.Deserialize(stream));

  return XII_SUCCESS;
}

void xiiControllerInputAnimNode::Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const
{
  {
    float fValue1 = 0.0f;
    float fValue2 = 0.0f;

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_NegX, &fValue1);
    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_PosX, &fValue2);
    m_OutLeftStickX.SetNumber(ref_graph, -fValue1 + fValue2);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_NegY, &fValue1);
    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftStick_PosY, &fValue2);
    m_OutLeftStickY.SetNumber(ref_graph, -fValue1 + fValue2);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_NegX, &fValue1);
    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_PosX, &fValue2);
    m_OutRightStickX.SetNumber(ref_graph, -fValue1 + fValue2);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_NegY, &fValue1);
    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightStick_PosY, &fValue2);
    m_OutRightStickY.SetNumber(ref_graph, -fValue1 + fValue2);
  }

  {
    float fValue = 0.0f;
    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonA, &fValue);
    m_OutButtonA.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonB, &fValue);
    m_OutButtonB.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonX, &fValue);
    m_OutButtonX.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_ButtonY, &fValue);
    m_OutButtonY.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftShoulder, &fValue);
    m_OutLeftShoulder.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_LeftTrigger, &fValue);
    m_OutLeftTrigger.SetNumber(ref_graph, fValue);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightShoulder, &fValue);
    m_OutRightShoulder.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_RightTrigger, &fValue);
    m_OutRightTrigger.SetNumber(ref_graph, fValue);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadLeft, &fValue);
    m_OutPadLeft.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadRight, &fValue);
    m_OutPadRight.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadUp, &fValue);
    m_OutPadUp.SetBool(ref_graph, fValue > 0);

    xiiInputManager::GetInputSlotState(xiiInputSlot_Controller0_PadDown, &fValue);
    m_OutPadDown.SetBool(ref_graph, fValue > 0);
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
