/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Input/VirtualThumbStick.h>
#include <Foundation/Time/Clock.h>

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
  s.SetFormat("Thumbstick_{0}", s_iThumbsticks);
  m_sName = s;

  ++s_iThumbsticks;
}

xiiVirtualThumbStick::~xiiVirtualThumbStick()
{
  xiiInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
}

void xiiVirtualThumbStick::SetTriggerInputSlot(xiiVirtualThumbStick::Input::Enum input, const xiiInputActionConfig* pCustomConfig)
{
  for (xiiInt32 i = 0; i < xiiInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    m_ActionConfig.m_sFilterByInputSlotX[i] = xiiInputSlot_None;
    m_ActionConfig.m_sFilterByInputSlotY[i] = xiiInputSlot_None;
    m_ActionConfig.m_sInputSlotTrigger[i]   = xiiInputSlot_None;
  }

  switch (input)
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

void xiiVirtualThumbStick::SetThumbstickOutput(xiiVirtualThumbStick::Output::Enum output, xiiStringView sOutputLeft, xiiStringView sOutputRight, xiiStringView sOutputUp, xiiStringView sOutputDown)
{
  switch (output)
  {
    case xiiVirtualThumbStick::Output::Controller0_LeftStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller0_LeftStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller0_LeftStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller0_LeftStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller0_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller0_RightStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller0_RightStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller0_RightStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller0_RightStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller0_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller1_LeftStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller1_LeftStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller1_LeftStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller1_LeftStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller1_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller1_RightStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller1_RightStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller1_RightStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller1_RightStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller1_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller2_LeftStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller2_LeftStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller2_LeftStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller2_LeftStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller2_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller2_RightStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller2_RightStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller2_RightStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller2_RightStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller2_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller3_LeftStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller3_LeftStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller3_LeftStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller3_LeftStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller3_LeftStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Controller3_RightStick:
    {
      m_sOutputLeft  = xiiInputSlot_Controller3_RightStick_NegX;
      m_sOutputRight = xiiInputSlot_Controller3_RightStick_PosX;
      m_sOutputUp    = xiiInputSlot_Controller3_RightStick_PosY;
      m_sOutputDown  = xiiInputSlot_Controller3_RightStick_NegY;
    }
    break;
    case xiiVirtualThumbStick::Output::Custom:
    {
      m_sOutputLeft  = sOutputLeft;
      m_sOutputRight = sOutputRight;
      m_sOutputUp    = sOutputUp;
      m_sOutputDown  = sOutputDown;
    }
    break;
  }

  m_bConfigChanged = true;
}

void xiiVirtualThumbStick::SetAreaFocusMode(xiiInputActionConfig::OnEnterArea onEnter, xiiInputActionConfig::OnLeaveArea onLeave)
{
  m_bConfigChanged = true;

  m_ActionConfig.m_OnEnterArea = onEnter;
  m_ActionConfig.m_OnLeaveArea = onLeave;
}

void xiiVirtualThumbStick::SetInputArea(const xiiVec2& vLowerLeft, const xiiVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft                       = vLowerLeft;
  m_vUpperRight                      = vUpperRight;
  m_fRadius                          = fThumbstickRadius;
  m_ActionConfig.m_fFilteredPriority = fPriority;
  m_CenterMode                       = center;
}

void xiiVirtualThumbStick::SetFlags(xiiBitflags<Flags> flags)
{
  m_Flags = flags;
}

void xiiVirtualThumbStick::SetInputCoordinateAspectRatio(float fWidthDivHeight)
{
  m_fAspectRatio = fWidthDivHeight;
}

void xiiVirtualThumbStick::GetInputArea(xiiVec2& out_vLowerLeft, xiiVec2& out_vUpperRight) const
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

  m_InputSlotValues[m_sOutputLeft]  = 0.0f;
  m_InputSlotValues[m_sOutputRight] = 0.0f;
  m_InputSlotValues[m_sOutputUp]    = 0.0f;
  m_InputSlotValues[m_sOutputDown]  = 0.0f;

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

    if (m_CenterMode == CenterMode::Swipe)
    {
      const xiiTime tDiff = xiiClock::GetGlobalClock()->GetTimeDiff();

      m_vCenter = xiiMath::Lerp(m_vCenter, m_vTouchPos, xiiMath::Min(1.0f, tDiff.AsFloatInSeconds() * 4.0f));
    }

    m_vTouchPos.Set(0.0f);

    xiiInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotX[(xiiUInt32)iTriggerAlt].GetData(), &m_vTouchPos.x);
    xiiInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotY[(xiiUInt32)iTriggerAlt].GetData(), &m_vTouchPos.y);

    if (ks == xiiKeyState::Pressed)
    {
      switch (m_CenterMode)
      {
        case CenterMode::InputArea:
          m_vCenter = m_vLowerLeft + (m_vUpperRight - m_vLowerLeft) * 0.5f;
          break;
        case CenterMode::ActivationPoint:
        case CenterMode::Swipe:
          m_vCenter = m_vTouchPos;
          break;
      }
    }

    m_vInputDirection = m_vTouchPos - m_vCenter;

    m_vInputDirection.y /= m_fAspectRatio;

    m_fInputStrength = xiiMath::Min(m_vInputDirection.GetLength(), m_fRadius) / m_fRadius;
    m_vInputDirection.NormalizeIfNotZero(xiiVec2::MakeZero()).IgnoreResult();

    const float fThreshold = 0.1f;

    float& l = m_InputSlotValues[m_sOutputLeft];
    float& r = m_InputSlotValues[m_sOutputRight];
    float& u = m_InputSlotValues[m_sOutputUp];
    float& d = m_InputSlotValues[m_sOutputDown];

    if (m_Flags.IsSet(Flags::OnlyMaxAxis))
    {
      const float maxVal = xiiMath::Max(m_vInputDirection.x, -m_vInputDirection.x, m_vInputDirection.y, -m_vInputDirection.y);

      // only activate the output axis that has the strongest (absolute) value
      if (m_vInputDirection.x == maxVal)
      {
        r = maxVal * m_fInputStrength;
      }
      else if (-m_vInputDirection.x == maxVal)
      {
        l = maxVal * m_fInputStrength;
      }
      else if (m_vInputDirection.y == maxVal)
      {
        d = maxVal * m_fInputStrength;
      }
      else if (-m_vInputDirection.y == maxVal)
      {
        u = maxVal * m_fInputStrength;
      }
    }
    else
    {
      l = xiiMath::Max(0.0f, -m_vInputDirection.x) * m_fInputStrength;
      r = xiiMath::Max(0.0f, m_vInputDirection.x) * m_fInputStrength;
      u = xiiMath::Max(0.0f, -m_vInputDirection.y) * m_fInputStrength;
      d = xiiMath::Max(0.0f, m_vInputDirection.y) * m_fInputStrength;
    }

    if (l < fThreshold)
      l = 0.0f;
    if (r < fThreshold)
      r = 0.0f;
    if (u < fThreshold)
      u = 0.0f;
    if (d < fThreshold)
      d = 0.0f;
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
