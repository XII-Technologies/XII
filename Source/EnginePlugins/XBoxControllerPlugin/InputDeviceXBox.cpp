#include <Core/Input/InputManager.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Logging/Log.h>
#include <XBoxControllerPlugin/InputDeviceXBox.h>

#include <Xinput.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInputDeviceXBox360, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInputDeviceXBox360::xiiInputDeviceXBox360()
{
  for (xiiInt32 i = 0; i < MaxControllers; ++i)
    m_bControllerConnected[i] = false;
}

xiiInputDeviceXBox360::~xiiInputDeviceXBox360() = default;

void xiiInputDeviceXBox360::RegisterControllerButton(const char* szButton, const char* szName, xiiBitflags<xiiInputSlotFlags> SlotFlags)
{
  xiiStringBuilder s, s2;

  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    s2.SetFormat("Cont {0}: {1}", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void xiiInputDeviceXBox360::SetDeadZone(const char* szButton)
{
  xiiStringBuilder s;

  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    xiiInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
  }
}

void xiiInputDeviceXBox360::RegisterInputSlots()
{
  RegisterControllerButton("button_a", "Button A", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("button_b", "Button B", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("button_x", "Button X", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("button_y", "Button Y", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("button_start", "Start", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("button_back", "Back", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("left_shoulder", "Left Shoulder", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("right_shoulder", "Right Shoulder", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("left_trigger", "Left Trigger", xiiInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("right_trigger", "Right Trigger", xiiInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("pad_up", "Pad Up", xiiInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_down", "Pad Down", xiiInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_left", "Pad Left", xiiInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_right", "Pad Right", xiiInputSlotFlags::IsDPad);
  RegisterControllerButton("left_stick", "Left Stick", xiiInputSlotFlags::IsButton);
  RegisterControllerButton("right_stick", "Right Stick", xiiInputSlotFlags::IsButton);

  RegisterControllerButton("leftstick_negx", "Left Stick Left", xiiInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posx", "Left Stick Right", xiiInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_negy", "Left Stick Down", xiiInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posy", "Left Stick Up", xiiInputSlotFlags::IsAnalogStick);

  RegisterControllerButton("rightstick_negx", "Right Stick Left", xiiInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posx", "Right Stick Right", xiiInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_negy", "Right Stick Down", xiiInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posy", "Right Stick Up", xiiInputSlotFlags::IsAnalogStick);

  SetDeadZone("left_trigger");
  SetDeadZone("right_trigger");
  SetDeadZone("leftstick_negx");
  SetDeadZone("leftstick_posx");
  SetDeadZone("leftstick_negy");
  SetDeadZone("leftstick_posy");
  SetDeadZone("rightstick_negx");
  SetDeadZone("rightstick_posx");
  SetDeadZone("rightstick_negy");
  SetDeadZone("rightstick_posy");

  xiiLog::Success("Initialized XBox 360 Controller.");
}

const char* szControllerName[] = {
  "controller0_",
  "controller1_",
  "controller2_",
  "controller3_",

  "controller4_",
  "controller5_",
  "controller6_",
  "controller7_",
};

XII_CHECK_AT_COMPILETIME(XII_ARRAY_SIZE(szControllerName) >= xiiInputDeviceXBox360::MaxControllers);

void xiiInputDeviceXBox360::SetValue(xiiInt32 iController, const char* szButton, float fValue)
{
  xiiStringBuilder s = szControllerName[iController];
  s.Append(szButton);
  float& fVal = m_InputSlotValues[s];
  fVal        = xiiMath::Max(fVal, fValue);
}

void xiiInputDeviceXBox360::UpdateHardwareState(xiiTime tTimeDifference)
{
  UpdateVibration(tTimeDifference);
}

void xiiInputDeviceXBox360::UpdateInputSlotValues()
{
  // Reset all keys
  for (auto it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
    it.Value() = 0.0f;

  XINPUT_STATE State[MaxControllers];
  bool         bIsAvailable[MaxControllers];

  // Update unconnected controllers only every few milliseconds, apparently it takes quite some time to do this
  // even on unconnected controllers
  static xiiTime tLastControllerSearch;
  const xiiTime  tNow               = xiiTime::Now();
  const bool     bSearchControllers = tNow - tLastControllerSearch > xiiTime::Seconds(0.5);

  if (bSearchControllers)
    tLastControllerSearch = tNow;

  // Retrieve the data from all physical devices
  for (xiiInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
  {
    if (bSearchControllers || m_bControllerConnected[iPhysical])
    {
      bIsAvailable[iPhysical] = (XInputGetState(iPhysical, &State[iPhysical]) == ERROR_SUCCESS);

      if (m_bControllerConnected[iPhysical] != bIsAvailable[iPhysical])
      {
        xiiLog::Info("XBox Controller {0} has been {1}.", iPhysical, bIsAvailable[iPhysical] ? "connected" : "disconnected");

        // This ensures to reset all values below
        if (!bIsAvailable[iPhysical])
          xiiMemoryUtils::ZeroFill(&State[iPhysical], 1);
      }
    }
    else
      bIsAvailable[iPhysical] = m_bControllerConnected[iPhysical];
  }

  // Now update all virtual controllers
  for (xiiUInt8 uiVirtual = 0; uiVirtual < MaxControllers; ++uiVirtual)
  {
    // Check from which physical device to take the input data
    const xiiInt8 iPhysical = GetControllerMapping(uiVirtual);

    // If the mapping is negative (which means 'deactivated'), ignore this controller
    if ((iPhysical < 0) || (iPhysical >= MaxControllers))
      continue;

    // If the controller is not active, no point in updating it
    // If it just got inactive, this will reset it once, because the state is only passed on after this loop
    if (!m_bControllerConnected[iPhysical])
      continue;

    SetValue(uiVirtual, "pad_up", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "pad_down", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "pad_left", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "pad_right", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_start", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_back", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "left_stick", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "right_stick", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "left_shoulder", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "right_shoulder", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_a", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_b", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_x", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_y", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0) ? 1.0f : 0.0f);

    const float fTriggerRange = 255.0f;

    SetValue(uiVirtual, "left_trigger", State[iPhysical].Gamepad.bLeftTrigger / fTriggerRange);
    SetValue(uiVirtual, "right_trigger", State[iPhysical].Gamepad.bRightTrigger / fTriggerRange);

    // All input points have dead-zones, so we can let the state handler do the rest
    SetValue(uiVirtual, "leftstick_negx", (State[iPhysical].Gamepad.sThumbLX < 0) ? (-State[iPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "leftstick_posx", (State[iPhysical].Gamepad.sThumbLX > 0) ? (State[iPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "leftstick_negy", (State[iPhysical].Gamepad.sThumbLY < 0) ? (-State[iPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "leftstick_posy", (State[iPhysical].Gamepad.sThumbLY > 0) ? (State[iPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);

    SetValue(uiVirtual, "rightstick_negx", (State[iPhysical].Gamepad.sThumbRX < 0) ? (-State[iPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "rightstick_posx", (State[iPhysical].Gamepad.sThumbRX > 0) ? (State[iPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "rightstick_negy", (State[iPhysical].Gamepad.sThumbRY < 0) ? (-State[iPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "rightstick_posy", (State[iPhysical].Gamepad.sThumbRY > 0) ? (State[iPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
  }

  for (xiiInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
    m_bControllerConnected[iPhysical] = bIsAvailable[iPhysical];
}

bool xiiInputDeviceXBox360::IsControllerConnected(xiiUInt8 uiPhysical) const
{
  XII_ASSERT_DEV(uiPhysical < MaxControllers, "Invalid Controller Index {0}", uiPhysical);

  return m_bControllerConnected[uiPhysical];
}

void xiiInputDeviceXBox360::ApplyVibration(xiiUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  if (!m_bControllerConnected[uiPhysicalController])
    return;

  static XINPUT_VIBRATION v[MaxControllers];

  if (eMotor == Motor::LeftMotor)
    v[uiPhysicalController].wLeftMotorSpeed = (WORD)(fStrength * 65535.0f);

  if (eMotor == Motor::RightMotor)
  {
    v[uiPhysicalController].wRightMotorSpeed = (WORD)(fStrength * 65535.0f);

    XInputSetState(uiPhysicalController, &v[uiPhysicalController]);
  }
}
