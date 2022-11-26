#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/UniquePtr.h>

#include <GLFW/glfw3.h>

class xiiControllerInputGlfw : public xiiInputDeviceController
{
public:
  virtual void InitializeDevice() override;

  virtual void UpdateInputSlotValues() override;

  virtual void ResetInputSlotValues() override;

  virtual void RegisterInputSlots() override;

  virtual bool IsControllerConnected(xiiUInt8 uiPhysical) const override;

private:
  virtual void ApplyVibration(xiiUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;

  void RegisterControllerButton(const char* szButton, const char* szName, xiiBitflags<xiiInputSlotFlags> SlotFlags);
  void SetDeadZone(const char* szButton);
  void SetControllerValue(xiiStringBuilder& tmp, xiiUInt8 controllerIndex, const char* inputSlotName, float value);

  bool m_bInitialized = false;
};

namespace
{
  xiiUniquePtr<xiiControllerInputGlfw> g_pControllerInputGlfw;
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Core, ControllerInput)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "InputManager",
    "Window"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pControllerInputGlfw = XII_DEFAULT_NEW(xiiControllerInputGlfw);
    xiiControllerInput::SetDevice(g_pControllerInputGlfw.Borrow());
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (xiiControllerInput::GetDevice() == g_pControllerInputGlfw.Borrow())
    {
      xiiControllerInput::SetDevice(nullptr);
    }
    g_pControllerInputGlfw.Clear();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace
{
  struct ControllerButtonMapping
  {
    const char* xiiName;
    int glfwIndex;
  };

  const ControllerButtonMapping g_ControllerButtonMappings[] = {
    {"button_a", GLFW_GAMEPAD_BUTTON_A},
    {"button_b", GLFW_GAMEPAD_BUTTON_B},
    {"button_x", GLFW_GAMEPAD_BUTTON_X},
    {"button_y", GLFW_GAMEPAD_BUTTON_Y},
    {"button_start", GLFW_GAMEPAD_BUTTON_START},
    {"button_back", GLFW_GAMEPAD_BUTTON_BACK},
    {"left_shoulder", GLFW_GAMEPAD_BUTTON_LEFT_BUMPER},
    {"right_shoulder", GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER},
    {"pad_up", GLFW_GAMEPAD_BUTTON_DPAD_UP},
    {"pad_down", GLFW_GAMEPAD_BUTTON_DPAD_DOWN},
    {"pad_left", GLFW_GAMEPAD_BUTTON_DPAD_LEFT},
    {"pad_right", GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"left_stick", GLFW_GAMEPAD_BUTTON_LEFT_THUMB},
    {"right_stick", GLFW_GAMEPAD_BUTTON_RIGHT_THUMB}};
} // namespace

void xiiControllerInputGlfw::RegisterControllerButton(const char* szButton, const char* szName, xiiBitflags<xiiInputSlotFlags> SlotFlags)
{
  xiiStringBuilder s, s2;

  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    s.Format("controller{0}_{1}", i, szButton);
    s2.Format("Cont {0}: {1}", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void xiiControllerInputGlfw::SetDeadZone(const char* szButton)
{
  xiiStringBuilder s;

  for (xiiInt32 i = 0; i < MaxControllers; ++i)
  {
    s.Format("controller{0}_{1}", i, szButton);
    xiiInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
  }
}

void xiiControllerInputGlfw::SetControllerValue(xiiStringBuilder& tmp, xiiUInt8 controllerIndex, const char* inputSlotName, float value)
{
  tmp.Format("controller{0}_{1}", controllerIndex, inputSlotName);
  m_InputSlotValues[tmp] = value;
}

void xiiControllerInputGlfw::InitializeDevice()
{
  // Make a arbitrary call into glfw so that we can check if the library is properly initialized
  glfwJoystickPresent(0);

  // Check for errors during the previous call
  const char* desc;
  int errorCode = glfwGetError(&desc);
  if(errorCode != GLFW_NO_ERROR)
  {
    xiiLog::Warning("glfw joystick and gamepad input not avaiable: {} - {}", errorCode, desc);
    return;
  }
  m_bInitialized = true;
}

void xiiControllerInputGlfw::UpdateInputSlotValues()
{
  if(!m_bInitialized)
  {
    return;
  }

  xiiStringBuilder inputSlotName;

  // update all virtual controllers
  for (xiiUInt8 uiVirtual = 0; uiVirtual < MaxControllers; ++uiVirtual)
  {
    // check from which physical device to take the input data
    const xiiInt8 iPhysical = GetControllerMapping(uiVirtual);

    // if the mapping is negative (which means 'deactivated'), ignore this controller
    if ((iPhysical < 0) || (iPhysical >= MaxControllers))
      continue;

    int glfwId = GLFW_JOYSTICK_1 + iPhysical;
    if (glfwJoystickPresent(glfwId))
    {
      if (glfwJoystickIsGamepad(glfwId))
      {
        GLFWgamepadstate state = {};
        if (glfwGetGamepadState(glfwId, &state))
        {
          for (size_t buttonIndex = 0; buttonIndex < XII_ARRAY_SIZE(g_ControllerButtonMappings); buttonIndex++)
          {
            const ControllerButtonMapping mapping = g_ControllerButtonMappings[buttonIndex];
            SetControllerValue(inputSlotName, uiVirtual, mapping.xiiName, (state.buttons[mapping.glfwIndex] == GLFW_PRESS) ? 1.0f : 0.0f);
          }

          SetControllerValue(inputSlotName, uiVirtual, "left_trigger", state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER]);
          SetControllerValue(inputSlotName, uiVirtual, "right_trigger", state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER]);

          SetControllerValue(inputSlotName, uiVirtual, "leftstick_negx", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] * -1.0f));
          SetControllerValue(inputSlotName, uiVirtual, "leftstick_posx", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]));
          SetControllerValue(inputSlotName, uiVirtual, "leftstick_negy", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]));
          SetControllerValue(inputSlotName, uiVirtual, "leftstick_posy", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] * -1.0f));

          SetControllerValue(inputSlotName, uiVirtual, "rightstick_negx", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] * -1.0f));
          SetControllerValue(inputSlotName, uiVirtual, "rightstick_posx", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]));
          SetControllerValue(inputSlotName, uiVirtual, "rightstick_negy", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]));
          SetControllerValue(inputSlotName, uiVirtual, "rightstick_posy", xiiMath::Max(0.0f, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] * -1.0f));
        }
      }
    }
  }
}

void xiiControllerInputGlfw::ResetInputSlotValues()
{
}

void xiiControllerInputGlfw::RegisterInputSlots()
{
  if(!m_bInitialized)
  {
    return;
  }

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

bool xiiControllerInputGlfw::IsControllerConnected(xiiUInt8 uiPhysical) const
{
  if(!m_bInitialized)
  {
    return false;
  }

  int glfwId = GLFW_JOYSTICK_1 + uiPhysical;
  return glfwJoystickPresent(glfwId) && glfwJoystickIsGamepad(glfwId);
}

void xiiControllerInputGlfw::ApplyVibration(xiiUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  // Unfortunately GLFW does not have vibration support
}
