/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)

#  include <Core/Input/DeviceTypes/Controller.h>
#  include <Core/Input/InputManager.h>
#  include <Core/System/ControllerInput.h>
#  include <Foundation/Configuration/Startup.h>
#  include <Foundation/Types/UniquePtr.h>

#  include <SDL3/SDL.h>

/// An implementation of xiiInputDeviceController that handles Game Controllers.
class XII_CORE_DLL xiiControllerInputSDL : public xiiInputDeviceController
{
  XII_ADD_DYNAMIC_REFLECTION(xiiControllerInputSDL, xiiInputDeviceController);

public:
  xiiControllerInputSDL();
  ~xiiControllerInputSDL();

  virtual bool IsControllerConnected(xiiUInt8 uiPhysical) const override;

private:
  virtual void ApplyVibration(xiiUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;

  bool m_bControllerConnected[MaxControllers];

  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void UpdateHardwareState(xiiTime tTimeDifference) override;

  void SetValue(xiiInt32 iController, const char* szButton, float fValue);

  static void RegisterControllerButton(const char* szButton, const char* szName, xiiBitflags<xiiInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);
};

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiControllerInputSDL, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  xiiUniquePtr<xiiControllerInputSDL> g_pControllerInputSDL;
  SDL_Gamepad*                        g_GamePads[xiiControllerInputSDL::MaxControllers];

  struct ControllerButtonMapping
  {
    const char*       xiiName;
    SDL_GamepadButton sdlButton;
  };

  const ControllerButtonMapping g_ControllerButtonMappings[] = {
    {"button_a", SDL_GAMEPAD_BUTTON_SOUTH},
    {"button_b", SDL_GAMEPAD_BUTTON_EAST},
    {"button_x", SDL_GAMEPAD_BUTTON_WEST},
    {"button_y", SDL_GAMEPAD_BUTTON_NORTH},
    {"button_start", SDL_GAMEPAD_BUTTON_START},
    {"button_back", SDL_GAMEPAD_BUTTON_BACK},
    {"left_shoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {"right_shoulder", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {"pad_up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {"pad_down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {"pad_left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {"pad_right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"left_stick", SDL_GAMEPAD_BUTTON_LEFT_STICK},
    {"right_stick", SDL_GAMEPAD_BUTTON_RIGHT_STICK}};

} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, InputDeviceXBox360)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", 
    "InputManager",
    "Window"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pControllerInputSDL = XII_DEFAULT_NEW(xiiControllerInputSDL);
    xiiControllerInput::SetDevice(g_pControllerInputSDL.Borrow());
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    // Close all opened game controllers
    for (xiiInt32 i = 0; i < xiiControllerInputSDL::MaxControllers; ++i)
    {
      if (g_GamePads[i] != nullptr)
      {
        SDL_CloseGamepad(g_GamePads[i]);
      }
    }

    if (xiiControllerInput::GetDevice() == g_pControllerInputSDL.Borrow())
    {
      xiiControllerInput::SetDevice(nullptr);
    }
    g_pControllerInputSDL.Clear();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }
 
  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }
 
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiControllerInputSDL::xiiControllerInputSDL()
{
  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    m_bControllerConnected[i] = false;
  }
}

xiiControllerInputSDL::~xiiControllerInputSDL() {}

void xiiControllerInputSDL::RegisterControllerButton(const char* szButton, const char* szName, xiiBitflags<xiiInputSlotFlags> SlotFlags)
{
  xiiStringBuilder s, s2;

  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    s2.SetFormat("Cont {0}: {1}", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void xiiControllerInputSDL::SetDeadZone(const char* szButton)
{
  xiiStringBuilder s;

  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    s.SetFormat("controller{0}_{1}", i, szButton);
    xiiInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
  }
}

void xiiControllerInputSDL::RegisterInputSlots()
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
}

void xiiControllerInputSDL::SetValue(xiiInt32 iController, const char* szButton, float fValue)
{
  xiiStringBuilder s;
  s.SetFormat("Controller{}_{}", iController, szButton);

  float& fVal = m_InputSlotValues[s];
  fVal        = xiiMath::Max(fVal, fValue);
}

void xiiControllerInputSDL::UpdateHardwareState(xiiTime tTimeDifference)
{
  UpdateVibration(tTimeDifference);
}

bool xiiControllerInputSDL::IsControllerConnected(xiiUInt8 uiPhysical) const
{
  XII_ASSERT_DEV(uiPhysical < MaxControllers, "Invalid Controller Index {0}", uiPhysical);

  return m_bControllerConnected[uiPhysical];
}

void xiiControllerInputSDL::UpdateInputSlotValues()
{
  // Reset all keys
  for (auto it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
    it.Value() = 0.0f;

  bool bIsAvailable[MaxControllers];

  // Update not connected controllers only every few milliseconds, apparently it takes quite some time to do this
  // Even on not connected controllers
  static xiiTime tLastControllerSearch;
  const xiiTime  tNow               = xiiTime::Now();
  const bool     bSearchControllers = tNow - tLastControllerSearch > xiiTime::MakeFromSeconds(0.5);

  if (bSearchControllers)
    tLastControllerSearch = tNow;

  // Get the data from all physical devices
  for (xiiInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
  {
    if (bSearchControllers || m_bControllerConnected[iPhysical])
    {
      bool bIsGameController = (SDL_IsGamepad(iPhysical) == true);

      SDL_Gamepad* pGameController = SDL_OpenGamepad(iPhysical);
      if (g_GamePads[iPhysical] != pGameController)
      {
        SDL_CloseGamepad(g_GamePads[iPhysical]);
        g_GamePads[iPhysical] = pGameController;

        xiiLog::Dev("New instance of game controller opened for '{0}'", iPhysical);
      }

      bIsAvailable[iPhysical] = bIsGameController && (g_GamePads[iPhysical] != nullptr);

      if (m_bControllerConnected[iPhysical] != bIsAvailable[iPhysical])
      {
        xiiLog::Info("Controller {0} has been {1}.", iPhysical, bIsAvailable[iPhysical] ? "connected" : "disconnected");
      }
    }
    else
      bIsAvailable[iPhysical] = m_bControllerConnected[iPhysical];
  }

  // Now update all virtual controllers
  for (xiiUInt8 uiVirtual = 0; uiVirtual < MaxControllers; ++uiVirtual)
  {
    // Check from which physical device to take the input data
    const xiiInt8 iPhysical = GetPhysicalControllerMapping(uiVirtual);

    // If the mapping is negative (which means 'deactivated'), ignore this controller
    if ((iPhysical < 0) || (iPhysical >= MaxControllers))
      continue;

    // If the controller is not active, no point in updating it
    // If it just got inactive, this will reset it once, because the state is only passed on after this loop
    if (!m_bControllerConnected[iPhysical])
      continue;

    // Update game controller buttons
    for (size_t buttonIndex = 0; buttonIndex < XII_ARRAY_SIZE(g_ControllerButtonMappings); ++buttonIndex)
    {
      const ControllerButtonMapping mapping     = g_ControllerButtonMappings[buttonIndex];
      xiiUInt8                      buttonState = SDL_GetGamepadButton(g_GamePads[iPhysical], mapping.sdlButton);
      SetValue(iPhysical, mapping.xiiName, (buttonState == 1) ? 1.0f : 0.0f);
    }

    // Update game controller trigger buttons
    float fLeftTriggerState  = static_cast<float>(SDL_GetGamepadAxis(g_GamePads[iPhysical], SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    float fRightTriggerState = static_cast<float>(SDL_GetGamepadAxis(g_GamePads[iPhysical], SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));

    SetValue(iPhysical, "left_trigger", fLeftTriggerState);
    SetValue(iPhysical, "right_trigger", fRightTriggerState);

    // Update game controller left stick
    float lXState = static_cast<float>(SDL_GetGamepadAxis(g_GamePads[iPhysical], SDL_GAMEPAD_AXIS_LEFTX));
    float lYState = static_cast<float>(SDL_GetGamepadAxis(g_GamePads[iPhysical], SDL_GAMEPAD_AXIS_LEFTY));

    SetValue(iPhysical, "leftstick_negx", xiiMath::Max(0.0f, lXState * -1.0f));
    SetValue(iPhysical, "leftstick_posx", xiiMath::Max(0.0f, lXState));
    SetValue(iPhysical, "leftstick_negy", xiiMath::Max(0.0f, lYState));
    SetValue(iPhysical, "leftstick_posy", xiiMath::Max(0.0f, lYState * -1.0f));

    // Update game controller right stick
    float rXState = static_cast<float>(SDL_GetGamepadAxis(g_GamePads[iPhysical], SDL_GAMEPAD_AXIS_RIGHTX));
    float rYState = static_cast<float>(SDL_GetGamepadAxis(g_GamePads[iPhysical], SDL_GAMEPAD_AXIS_RIGHTY));

    SetValue(iPhysical, "rightstick_negx", xiiMath::Max(0.0f, rXState * -1.0f));
    SetValue(iPhysical, "rightstick_posx", xiiMath::Max(0.0f, rXState));
    SetValue(iPhysical, "rightstick_negy", xiiMath::Max(0.0f, rYState));
    SetValue(iPhysical, "rightstick_posy", xiiMath::Max(0.0f, rYState * -1.0f));
  }

  for (xiiInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
  {
    m_bControllerConnected[iPhysical] = bIsAvailable[iPhysical];
  }
}

void xiiControllerInputSDL::ApplyVibration(xiiUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  if (!m_bControllerConnected[uiPhysicalController])
    return;

  switch (eMotor)
  {
    case xiiInputDeviceController::Motor::LeftMotor:
    {
      xiiInt32 result = SDL_RumbleGamepadTriggers(g_GamePads[uiPhysicalController], static_cast<xiiUInt16>(fStrength), 0, 100);
      if (result < 0)
        xiiLog::Info("Encountered an error setting rumble on SDL Controller '{0}'.", SDL_GetError());
    }
    break;

    case xiiInputDeviceController::Motor::RightMotor:
    {
      xiiInt32 result = SDL_RumbleGamepadTriggers(g_GamePads[uiPhysicalController], 0, static_cast<xiiUInt16>(fStrength), 100);
      if (result < 0)
        xiiLog::Info("Encountered an error setting rumble on SDL Controller '{0}'.", SDL_GetError());
    }
    break;

    default:
      break;
  }
}

#endif
