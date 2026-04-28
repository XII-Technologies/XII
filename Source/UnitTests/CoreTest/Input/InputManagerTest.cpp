/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Memory/MemoryUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(Input);

static bool operator==(const xiiInputActionConfig& lhs, const xiiInputActionConfig& rhs)
{
  if (lhs.m_bApplyTimeScaling != rhs.m_bApplyTimeScaling)
    return false;
  if (lhs.m_fFilteredPriority != rhs.m_fFilteredPriority)
    return false;
  if (lhs.m_fFilterXMaxValue != rhs.m_fFilterXMaxValue)
    return false;
  if (lhs.m_fFilterXMinValue != rhs.m_fFilterXMinValue)
    return false;
  if (lhs.m_fFilterYMaxValue != rhs.m_fFilterYMaxValue)
    return false;
  if (lhs.m_fFilterYMinValue != rhs.m_fFilterYMinValue)
    return false;

  if (lhs.m_OnEnterArea != rhs.m_OnEnterArea)
    return false;
  if (lhs.m_OnLeaveArea != rhs.m_OnLeaveArea)
    return false;

  for (int i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    if (lhs.m_sInputSlotTrigger[i] != rhs.m_sInputSlotTrigger[i])
      return false;
    if (lhs.m_fInputSlotScale[i] != rhs.m_fInputSlotScale[i])
      return false;
    if (lhs.m_sFilterByInputSlotX[i] != rhs.m_sFilterByInputSlotX[i])
      return false;
    if (lhs.m_sFilterByInputSlotY[i] != rhs.m_sFilterByInputSlotY[i])
      return false;
  }

  return true;
}

class xiiTestInputDevide : public xiiInputDevice
{
public:
  void ActivateAll()
  {
    m_InputSlotValues["testdevice_button"]     = 0.1f;
    m_InputSlotValues["testdevice_stick"]      = 0.2f;
    m_InputSlotValues["testdevice_wheel"]      = 0.3f;
    m_InputSlotValues["testdevice_touchpoint"] = 0.4f;
    m_uiLastCharacter                          = '\42';
  }

private:
  void InitializeDevice() override {}
  void UpdateInputSlotValues() override {}
  void RegisterInputSlots() override
  {
    RegisterInputSlot("testdevice_button", "", xiiInputSlotFlags::IsButton);
    RegisterInputSlot("testdevice_stick", "", xiiInputSlotFlags::IsAnalogStick);
    RegisterInputSlot("testdevice_wheel", "", xiiInputSlotFlags::IsMouseWheel);
    RegisterInputSlot("testdevice_touchpoint", "", xiiInputSlotFlags::IsTouchPoint);
  }

  void ResetInputSlotValues() override { m_InputSlotValues.Clear(); }
};

XII_CREATE_SIMPLE_TEST(Input, InputManager)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInputSlotDisplayName / GetInputSlotDisplayName")
  {
    xiiInputManager::SetInputSlotDisplayName("test_slot_1", "Test Slot 1 Name");
    xiiInputManager::SetInputSlotDisplayName("test_slot_2", "Test Slot 2 Name");
    xiiInputManager::SetInputSlotDisplayName("test_slot_3", "Test Slot 3 Name");
    xiiInputManager::SetInputSlotDisplayName("test_slot_4", "Test Slot 4 Name");

    XII_TEST_STRING(xiiInputManager::GetInputSlotDisplayName("test_slot_1"), "Test Slot 1 Name");
    XII_TEST_STRING(xiiInputManager::GetInputSlotDisplayName("test_slot_2"), "Test Slot 2 Name");
    XII_TEST_STRING(xiiInputManager::GetInputSlotDisplayName("test_slot_3"), "Test Slot 3 Name");
    XII_TEST_STRING(xiiInputManager::GetInputSlotDisplayName("test_slot_4"), "Test Slot 4 Name");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInputSlotDeadZone / GetInputSlotDisplayName")
  {
    xiiInputManager::SetInputSlotDeadZone("test_slot_1", 0.1f);
    xiiInputManager::SetInputSlotDeadZone("test_slot_2", 0.2f);
    xiiInputManager::SetInputSlotDeadZone("test_slot_3", 0.3f);
    xiiInputManager::SetInputSlotDeadZone("test_slot_4", 0.4f);

    XII_TEST_FLOAT(xiiInputManager::GetInputSlotDeadZone("test_slot_1"), 0.1f, 0.0f);
    XII_TEST_FLOAT(xiiInputManager::GetInputSlotDeadZone("test_slot_2"), 0.2f, 0.0f);
    XII_TEST_FLOAT(xiiInputManager::GetInputSlotDeadZone("test_slot_3"), 0.3f, 0.0f);
    XII_TEST_FLOAT(xiiInputManager::GetInputSlotDeadZone("test_slot_4"), 0.4f, 0.0f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetInputActionConfig / GetInputActionConfig")
  {
    xiiInputActionConfig iac1, iac2;
    iac1.m_bApplyTimeScaling    = true;
    iac1.m_fFilteredPriority    = 23.0f;
    iac1.m_fInputSlotScale[0]   = 2.0f;
    iac1.m_fInputSlotScale[1]   = 3.0f;
    iac1.m_fInputSlotScale[2]   = 4.0f;
    iac1.m_sInputSlotTrigger[0] = xiiInputSlot_Key0;
    iac1.m_sInputSlotTrigger[1] = xiiInputSlot_Key1;
    iac1.m_sInputSlotTrigger[2] = xiiInputSlot_Key2;

    iac2.m_bApplyTimeScaling    = false;
    iac2.m_fFilteredPriority    = 42.0f;
    iac2.m_fInputSlotScale[0]   = 4.0f;
    iac2.m_fInputSlotScale[1]   = 5.0f;
    iac2.m_fInputSlotScale[2]   = 6.0f;
    iac2.m_sInputSlotTrigger[0] = xiiInputSlot_Key3;
    iac2.m_sInputSlotTrigger[1] = xiiInputSlot_Key4;
    iac2.m_sInputSlotTrigger[2] = xiiInputSlot_Key5;

    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac1, true);
    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac2, true);

    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);

    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, false);
    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, false);

    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);

    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, true);
    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, true);

    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_1") != iac1);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_2") != iac2);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    XII_TEST_BOOL(xiiInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);


    xiiInputManager::RemoveInputAction("test_inputset", "test_action_1");
    xiiInputManager::RemoveInputAction("test_inputset", "test_action_2");
    xiiInputManager::RemoveInputAction("test_inputset", "test_action_3");
    xiiInputManager::RemoveInputAction("test_inputset", "test_action_4");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Input Slot State Changes / Dead Zones")
  {
    float f = 0;
    xiiInputManager::InjectInputSlotValue("test_slot_1", 0.0f);
    xiiInputManager::SetInputSlotDeadZone("test_slot_1", 0.25f);

    // just check the first state
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Up);
    XII_TEST_FLOAT(f, 0.0f, 0);

    // value is not yet propagated
    xiiInputManager::InjectInputSlotValue("test_slot_1", 1.0f);
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Up);
    XII_TEST_FLOAT(f, 0.0f, 0);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Pressed);
    XII_TEST_FLOAT(f, 1.0f, 0);

    xiiInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Down);
    XII_TEST_FLOAT(f, 0.5f, 0);

    xiiInputManager::InjectInputSlotValue("test_slot_1", 0.3f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Down);
    XII_TEST_FLOAT(f, 0.3f, 0);

    // below dead zone value
    xiiInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Released);
    XII_TEST_FLOAT(f, 0.0f, 0);

    xiiInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Pressed);
    XII_TEST_FLOAT(f, 0.5f, 0);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Released);
    XII_TEST_FLOAT(f, 0.0f, 0);

    xiiInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    XII_TEST_BOOL(xiiInputManager::GetInputSlotState("test_slot_1", &f) == xiiKeyState::Up);
    XII_TEST_FLOAT(f, 0.0f, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetActionDisplayName / GetActionDisplayName")
  {
    xiiInputManager::SetActionDisplayName("test_action_1", "Test Action 1 Name");
    xiiInputManager::SetActionDisplayName("test_action_2", "Test Action 2 Name");
    xiiInputManager::SetActionDisplayName("test_action_3", "Test Action 3 Name");
    xiiInputManager::SetActionDisplayName("test_action_4", "Test Action 4 Name");

    XII_TEST_STRING(xiiInputManager::GetActionDisplayName("test_action_0"), "test_action_0");
    XII_TEST_STRING(xiiInputManager::GetActionDisplayName("test_action_1"), "Test Action 1 Name");
    XII_TEST_STRING(xiiInputManager::GetActionDisplayName("test_action_2"), "Test Action 2 Name");
    XII_TEST_STRING(xiiInputManager::GetActionDisplayName("test_action_3"), "Test Action 3 Name");
    XII_TEST_STRING(xiiInputManager::GetActionDisplayName("test_action_4"), "Test Action 4 Name");
    XII_TEST_STRING(xiiInputManager::GetActionDisplayName("test_action_5"), "test_action_5");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Input Sets")
  {
    xiiInputActionConfig iac;
    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac, true);
    xiiInputManager::SetInputActionConfig("test_inputset2", "test_action_2", iac, true);

    xiiDynamicArray<xiiString> InputSetNames;
    xiiInputManager::GetAllInputSets(InputSetNames);

    XII_TEST_INT(InputSetNames.GetCount(), 2);

    XII_TEST_STRING(InputSetNames[0].GetData(), "test_inputset");
    XII_TEST_STRING(InputSetNames[1].GetData(), "test_inputset2");

    xiiInputManager::RemoveInputAction("test_inputset", "test_action_1");
    xiiInputManager::RemoveInputAction("test_inputset2", "test_action_2");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetAllInputActions / RemoveInputAction")
  {
    xiiHybridArray<xiiString, 24> InputActions;

    xiiInputManager::GetAllInputActions("test_inputset_3", InputActions);

    XII_TEST_BOOL(InputActions.IsEmpty());

    xiiInputActionConfig iac;
    xiiInputManager::SetInputActionConfig("test_inputset_3", "test_action_1", iac, true);
    xiiInputManager::SetInputActionConfig("test_inputset_3", "test_action_2", iac, true);
    xiiInputManager::SetInputActionConfig("test_inputset_3", "test_action_3", iac, true);

    xiiInputManager::GetAllInputActions("test_inputset_3", InputActions);

    XII_TEST_INT(InputActions.GetCount(), 3);

    XII_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    XII_TEST_STRING(InputActions[1].GetData(), "test_action_2");
    XII_TEST_STRING(InputActions[2].GetData(), "test_action_3");


    xiiInputManager::RemoveInputAction("test_inputset_3", "test_action_2");

    xiiInputManager::GetAllInputActions("test_inputset_3", InputActions);

    XII_TEST_INT(InputActions.GetCount(), 2);

    XII_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    XII_TEST_STRING(InputActions[1].GetData(), "test_action_3");

    xiiInputManager::RemoveInputAction("test_inputset_3", "test_action_1");
    xiiInputManager::RemoveInputAction("test_inputset_3", "test_action_3");

    xiiInputManager::GetAllInputActions("test_inputset_3", InputActions);

    XII_TEST_BOOL(InputActions.IsEmpty());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Input Action State Changes")
  {
    xiiInputActionConfig iac;
    iac.m_bApplyTimeScaling    = false;
    iac.m_sInputSlotTrigger[0] = "test_input_slot_1";
    iac.m_sInputSlotTrigger[1] = "test_input_slot_2";
    iac.m_sInputSlotTrigger[2] = "test_input_slot_3";

    // bind the three slots to this action
    xiiInputManager::SetInputActionConfig("test_inputset", "test_action", iac, true);

    // bind the same three slots to another action
    xiiInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac, false);

    // the first slot to trigger the action is bound to it, the other slots can now trigger other actions
    // but not this one anymore
    xiiInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);

    float   f     = 0;
    xiiInt8 iSlot = 0;
    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Up);
    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Up);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Pressed);
    XII_TEST_INT(iSlot, 1);
    XII_TEST_FLOAT(f, 1.0f, 0.0f);

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Pressed);
    XII_TEST_INT(iSlot, 1);
    XII_TEST_FLOAT(f, 1.0f, 0.0f);

    // inject all three input slots
    xiiInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    xiiInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);
    xiiInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Down);
    XII_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    XII_TEST_FLOAT(f, 1.0f, 0.0f);

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Down);
    XII_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    XII_TEST_FLOAT(f, 1.0f, 0.0f);

    xiiInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    xiiInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Released);
    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Released);

    xiiInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    xiiInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Up);
    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Up);

    xiiInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Pressed);
    XII_TEST_INT(iSlot, 2);
    XII_TEST_FLOAT(f, 1.0f, 0.0f);

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Pressed);
    XII_TEST_INT(iSlot, 2);
    XII_TEST_FLOAT(f, 1.0f, 0.0f);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Released);
    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Released);

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == xiiKeyState::Up);
    XII_TEST_BOOL(xiiInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == xiiKeyState::Up);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetPressedInputSlot")
  {
    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    xiiStringView sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::None, xiiInputSlotFlags::None);
    XII_TEST_BOOL(sSlot.IsEmpty());

    xiiInputManager::InjectInputSlotValue("test_slot", 1.0f);

    sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::None, xiiInputSlotFlags::None);
    XII_TEST_STRING(sSlot, "");

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::None, xiiInputSlotFlags::None);
    XII_TEST_STRING(sSlot, "test_slot");


    {
      xiiTestInputDevide dev;
      dev.ActivateAll();

      xiiInputManager::InjectInputSlotValue("test_slot", 1.0f);

      xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsButton, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "testdevice_button");

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsAnalogStick, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "testdevice_stick");

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsMouseWheel, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "testdevice_wheel");

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsTouchPoint, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "testdevice_touchpoint");

      xiiInputManager::InjectInputSlotValue("test_slot", 1.0f);

      xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsButton, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "");

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsAnalogStick, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "");

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsMouseWheel, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "");

      sSlot = xiiInputManager::GetPressedInputSlot(xiiInputSlotFlags::IsTouchPoint, xiiInputSlotFlags::None);
      XII_TEST_STRING(sSlot, "");
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LastCharacter")
  {
    xiiTestInputDevide dev;
    dev.ActivateAll();

    XII_TEST_BOOL(xiiInputManager::RetrieveLastCharacter(true) == '\0');

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    XII_TEST_BOOL(xiiInputManager::RetrieveLastCharacter(false) == '\42');
    XII_TEST_BOOL(xiiInputManager::RetrieveLastCharacter(true) == '\42');
    XII_TEST_BOOL(xiiInputManager::RetrieveLastCharacter(true) == '\0');
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Time Scaling")
  {
    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    xiiInputActionConfig iac;
    iac.m_bApplyTimeScaling    = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    xiiInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    xiiTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    xiiInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    XII_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 30.0));
    xiiInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    XII_TEST_FLOAT(fVal, 0.1f * (1.0 / 30.0), 0.0001f);


    iac.m_bApplyTimeScaling    = false;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    xiiInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    dev.ActivateAll();

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    xiiInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    XII_TEST_FLOAT(fVal, 0.1f, 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 30.0));
    xiiInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    XII_TEST_FLOAT(fVal, 0.1f, 0.0001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetInputSlotFlags")
  {
    xiiTestInputDevide dev;
    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 30.0));

    XII_TEST_BOOL(xiiInputManager::GetInputSlotFlags("testdevice_button") == xiiInputSlotFlags::IsButton);
    XII_TEST_BOOL(xiiInputManager::GetInputSlotFlags("testdevice_stick") == xiiInputSlotFlags::IsAnalogStick);
    XII_TEST_BOOL(xiiInputManager::GetInputSlotFlags("testdevice_wheel") == xiiInputSlotFlags::IsMouseWheel);
    XII_TEST_BOOL(xiiInputManager::GetInputSlotFlags("testdevice_touchpoint") == xiiInputSlotFlags::IsTouchPoint);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ClearInputMapping")
  {
    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    xiiInputActionConfig iac;
    iac.m_bApplyTimeScaling    = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    xiiInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    xiiTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));
    xiiInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    XII_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    // clear the button from the action
    xiiInputManager::ClearInputMapping("test_inputset", "testdevice_button");

    dev.ActivateAll();

    xiiInputManager::Update(xiiTime::MakeFromSeconds(1.0 / 60.0));

    // should not receive input anymore
    xiiInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    XII_TEST_FLOAT(fVal, 0.0f, 0.0001f);
  }
}
