#include <Core/CorePCH.h>

#include <Core/Input/VirtualThumbStick.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVirtualThumbStick, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiInt32 xiiVirtualThumbStick::s_iThumbsticks = 0;

xiiVirtualThumbStick::xiiVirtualThumbStick()
{
  SetAreaFocusMode(xiiInputActionConfig::RequireKeyUp, xiiInputActionConfig::KeepFocus);
  SetTriggerInputSlot(xiiVirtualThumbStick::Input::Touchpoint);
  SetThumbstickOutput(xiiVirtualThumbStick::Output::Controller0_LeftStick);

  SetInputArea(xiiVec2(0.0f), xiiVec2(0.0f), 0.0f, 0.0f);

  xiiStringBuilder s;
  s.Format("Thumbstick_{0}", s_iThumbsticks);
  m_sName = s;

  ++s_iThumbsticks;

  m_bEnabled       = false;
  m_bConfigChanged = false;
  m_bIsActive      = false;
}

xiiVirtualThumbStick::~xiiVirtualThumbStick()
{
  xiiInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
}

void xiiVirtualThumbStick::SetTriggerInputSlot(xiiVirtualThumbStick::Input::Enum Input, const xiiInputActionConfig* pCustomConfig)
{
  for (xiiInt32 i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    m_ActionConfig.m_sFilterByInputSlotX[i] = xiiInputSlot_None;
    m_ActionConfig.m_sFilterByInputSlotY[i] = xiiInputSlot_None;
    m_ActionConfig.m_sInputSlotTrigger[i]   = xiiInputSlot_None;
  }

  switch (Input)
  {
    case xiiVirtualThumbStick::Input::Touchpoint:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = xiiInputSlot_TouchPoint0_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = xiiInputSlot_TouchPoint0_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[0]   = xiiInputSlot_TouchPoint0;

      m_ActionConfig.m_sFilterByInputSlotX[1] = xiiInputSlot_TouchPoint1_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[1] = xiiInputSlot_TouchPoint1_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[1]   = xiiInputSlot_TouchPoint1;

      m_ActionConfig.m_sFilterByInputSlotX[2] = xiiInputSlot_TouchPoint2_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[2] = xiiInputSlot_TouchPoint2_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[2]   = xiiInputSlot_TouchPoint2;
    }
    break;
    case xiiVirtualThumbStick::Input::MousePosition:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = xiiInputSlot_MousePositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = xiiInputSlot_MousePositionY;
      m_ActionConfig.m_sInputSlotTrigger[0]   = xiiInputSlot_MouseButton0;
    }
    break;
    case xiiVirtualThumbStick::Input::Custom:
    {
      XII_ASSERT_DEV(pCustomConfig != nullptr, "Must pass a custom config, if you want to have a custom config.");

      for (xiiInt32 i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        m_ActionConfig.m_sFilterByInputSlotX[i] = pCustomConfig->m_sFilterByInputSlotX[i];
        m_ActionConfig.m_sFilterByInputSlotY[i] = pCustomConfig->m_sFilterByInputSlotY[i];
        m_ActionConfig.m_sInputSlotTrigger[i]   = pCustomConfig->m_sInputSlotTrigger[i];
      }
    }
    break;
  }

  m_bConfigChanged = true;
}

void xiiVirtualThumbStick::SetThumbstickOutput(
  xiiVirtualThumbStick::Output::Enum Output,
  const char*                        szOutputLeft,
  const char*                        szOutputRight,
  const char*                        szOutputUp,
  const char*                        szOutputDown)
{
  switch (Output)
  {
    case xiiVirtualThumbStick::Output::Controller0_LeftStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller0_LeftStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller0_LeftStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller0_LeftStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller0_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller0_RightStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller0_RightStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller0_RightStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller0_RightStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller0_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller1_LeftStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller1_LeftStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller1_LeftStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller1_LeftStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller1_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller1_RightStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller1_RightStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller1_RightStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller1_RightStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller1_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller2_LeftStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller2_LeftStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller2_LeftStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller2_LeftStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller2_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller2_RightStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller2_RightStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller2_RightStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller2_RightStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller2_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller3_LeftStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller3_LeftStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller3_LeftStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller3_LeftStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller3_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller3_RightStick:
    {
      m_szOutputLeft  = xiiInputSlot_Controller3_RightStick_NegX;
      m_szOutputRight = xiiInputSlot_Controller3_RightStick_PosX;
      m_szOutputUp    = xiiInputSlot_Controller3_RightStick_PosY;
      m_szOutputDown  = xiiInputSlot_Controller3_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Custom:
    {
      m_szOutputLeft  = szOutputLeft;
      m_szOutputRight = szOutputRight;
      m_szOutputUp    = szOutputUp;
      m_szOutputDown  = szOutputDown;
    }
    break;
  }

  m_bConfigChanged = true;
}

void xiiVirtualThumbStick::SetAreaFocusMode(xiiInputActionConfig::OnEnterArea OnEnter, xiiInputActionConfig::OnLeaveArea OnLeave)
{
  m_bConfigChanged = true;

  m_ActionConfig.m_OnEnterArea = OnEnter;
  m_ActionConfig.m_OnLeaveArea = OnLeave;
}

void xiiVirtualThumbStick::SetInputArea(
  const xiiVec2&   vLowerLeft,
  const xiiVec2&   vUpperRight,
  float            fThumbstickRadius,
  float            fPriority,
  CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft                       = vLowerLeft;
  m_vUpperRight                      = vUpperRight;
  m_fRadius                          = fThumbstickRadius;
  m_ActionConfig.m_fFilteredPriority = fPriority;
  m_CenterMode                       = center;
}

void xiiVirtualThumbStick::GetInputArea(xiiVec2& out_vLowerLeft, xiiVec2& out_vUpperRight)
{
  out_vLowerLeft  = m_vLowerLeft;
  out_vUpperRight = m_vUpperRight;
}

void xiiVirtualThumbStick::UpdateActionMapping()
{
  if (!m_bConfigChanged)
    return;

  m_ActionConfig.m_fFilterXMinValue = m_vLowerLeft.x;
  m_ActionConfig.m_fFilterXMaxValue = m_vUpperRight.x;
  m_ActionConfig.m_fFilterYMinValue = m_vLowerLeft.y;
  m_ActionConfig.m_fFilterYMaxValue = m_vUpperRight.y;

  xiiInputManager::SetInputActionConfig(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), m_ActionConfig, false);

  m_bConfigChanged = false;
}

void xiiVirtualThumbStick::UpdateInputSlotValues()
{
  m_bIsActive = false;

  m_InputSlotValues[m_szOutputLeft]  = 0.0f;
  m_InputSlotValues[m_szOutputRight] = 0.0f;
  m_InputSlotValues[m_szOutputUp]    = 0.0f;
  m_InputSlotValues[m_szOutputDown]  = 0.0f;

  if (!m_bEnabled)
  {
    xiiInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
    return;
  }

  UpdateActionMapping();

  float   fValue;
  xiiInt8 iTriggerAlt;

  const xiiKeyState::Enum ks = xiiInputManager::GetInputActionState(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), &fValue, &iTriggerAlt);

  if (ks != xiiKeyState::Up)
  {
    m_bIsActive = true;

    xiiVec2 vTouchPos(0.0f);

    xiiInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotX[(xiiUInt32)iTriggerAlt].GetData(), &vTouchPos.x);
    xiiInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotY[(xiiUInt32)iTriggerAlt].GetData(), &vTouchPos.y);

    if (ks == xiiKeyState::Pressed)
    {
      switch (m_CenterMode)
      {
        case CenterMode::InputArea:
          m_vCenter = m_vLowerLeft + (m_vUpperRight - m_vLowerLeft) * 0.5f;
          break;
        case CenterMode::ActivationPoint:
          m_vCenter = vTouchPos;
          break;
      }
    }

    xiiVec2 vDir = vTouchPos - m_vCenter;
    vDir.y *= -1;

    const float fLength = xiiMath::Min(vDir.GetLength(), m_fRadius) / m_fRadius;
    vDir.Normalize();

    m_InputSlotValues[m_szOutputLeft]  = xiiMath::Max(0.0f, -vDir.x) * fLength;
    m_InputSlotValues[m_szOutputRight] = xiiMath::Max(0.0f, vDir.x) * fLength;
    m_InputSlotValues[m_szOutputUp]    = xiiMath::Max(0.0f, vDir.y) * fLength;
    m_InputSlotValues[m_szOutputDown]  = xiiMath::Max(0.0f, -vDir.y) * fLength;
  }
}

void xiiVirtualThumbStick::RegisterInputSlots()
{
  RegisterInputSlot(xiiInputSlot_Controller0_LeftStick_NegX, "Left Stick Left", xiiInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(xiiInputSlot_Controller0_LeftStick_PosX, "Left Stick Right", xiiInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(xiiInputSlot_Controller0_LeftStick_NegY, "Left Stick Down", xiiInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(xiiInputSlot_Controller0_LeftStick_PosY, "Left Stick Up", xiiInputSlotFlags::IsAnalogStick);

  RegisterInputSlot(xiiInputSlot_Controller0_RightStick_NegX, "Right Stick Left", xiiInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(xiiInputSlot_Controller0_RightStick_PosX, "Right Stick Right", xiiInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(xiiInputSlot_Controller0_RightStick_NegY, "Right Stick Down", xiiInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(xiiInputSlot_Controller0_RightStick_PosY, "Right Stick Up", xiiInputSlotFlags::IsAnalogStick);
}


XII_STATICLINK_FILE(Core, Core_Input_Implementation_VirtualThumbStick);
