#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInputDeviceMouseKeyboard, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiInputDeviceController, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiInt32 xiiInputDeviceMouseKeyboard::s_iMouseIsOverWindowNumber = -1;

xiiInputDeviceController::xiiInputDeviceController()
{
  m_uiVibrationTrackPos = 0;

  for (xiiInt8 c = 0; c < MaxControllers; ++c)
  {
    m_bVibrationEnabled[c]  = false;
    m_iControllerMapping[c] = c;

    for (xiiInt8 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      m_fVibrationStrength[c][m] = 0.0f;

      for (xiiUInt8 t = 0; t < MaxVibrationSamples; ++t)
        m_fVibrationTracks[c][m][t] = 0.0f;
    }
  }
}

void xiiInputDeviceController::EnableVibration(xiiUInt8 uiVirtual, bool bEnable)
{
  XII_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  m_bVibrationEnabled[uiVirtual] = bEnable;
}

bool xiiInputDeviceController::IsVibrationEnabled(xiiUInt8 uiVirtual) const
{
  XII_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_bVibrationEnabled[uiVirtual];
}

void xiiInputDeviceController::SetVibrationStrength(xiiUInt8 uiVirtual, Motor::Enum motor, float fValue)
{
  XII_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  XII_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  m_fVibrationStrength[uiVirtual][motor] = xiiMath::Clamp(fValue, 0.0f, 1.0f);
}

float xiiInputDeviceController::GetVibrationStrength(xiiUInt8 uiVirtual, Motor::Enum motor)
{
  XII_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  XII_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  return m_fVibrationStrength[uiVirtual][motor];
}

void xiiInputDeviceController::SetControllerMapping(xiiUInt8 uiVirtualController, xiiInt8 iTakeInputFromPhysical)
{
  XII_ASSERT_DEV(
    uiVirtualController < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", uiVirtualController, MaxControllers);
  XII_ASSERT_DEV(
    iTakeInputFromPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", iTakeInputFromPhysical, MaxControllers);

  if (iTakeInputFromPhysical < 0)
  {
    // deactivates this virtual controller
    m_iControllerMapping[uiVirtualController] = -1;
  }
  else
  {
    // if any virtual controller already maps to the given physical controller, let it use the physical controller that
    // uiVirtualController is currently mapped to
    for (xiiInt32 c = 0; c < MaxControllers; ++c)
    {
      if (m_iControllerMapping[c] == iTakeInputFromPhysical)
      {
        m_iControllerMapping[c] = m_iControllerMapping[uiVirtualController];
        break;
      }
    }

    m_iControllerMapping[uiVirtualController] = iTakeInputFromPhysical;
  }
}

xiiInt8 xiiInputDeviceController::GetControllerMapping(xiiUInt8 uiVirtual) const
{
  XII_ASSERT_DEV(uiVirtual < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_iControllerMapping[uiVirtual];
}

void xiiInputDeviceController::AddVibrationTrack(
  xiiUInt8    uiVirtual,
  Motor::Enum motor,
  float*      pVibrationTrackValue,
  xiiUInt32   uiSamples,
  float       fScalingFactor)
{
  uiSamples = xiiMath::Min<xiiUInt32>(uiSamples, MaxVibrationSamples);

  for (xiiUInt32 s = 0; s < uiSamples; ++s)
  {
    float& fVal = m_fVibrationTracks[uiVirtual][motor][(m_uiVibrationTrackPos + 1 + s) % MaxVibrationSamples];

    fVal = xiiMath::Max(fVal, pVibrationTrackValue[s] * fScalingFactor);
    fVal = xiiMath::Clamp(fVal, 0.0f, 1.0f);
  }
}

void xiiInputDeviceController::UpdateVibration(xiiTime tTimeDifference)
{
  static xiiTime tElapsedTime;
  tElapsedTime += tTimeDifference;

  const xiiTime tTimePerSample = xiiTime::Seconds(1.0 / VibrationSamplesPerSecond);

  // advance the vibration track sampling
  while (tElapsedTime >= tTimePerSample)
  {
    tElapsedTime -= tTimePerSample;

    for (xiiUInt32 c = 0; c < MaxControllers; ++c)
    {
      for (xiiUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        m_fVibrationTracks[c][m][m_uiVibrationTrackPos] = 0.0f;
    }

    m_uiVibrationTrackPos = (m_uiVibrationTrackPos + 1) % MaxVibrationSamples;
  }

  // we will temporarily store how much vibration is to be applied on each physical controller
  float fVibrationToApply[MaxControllers][Motor::ENUM_COUNT];

  // Initialize with zero (we might not set all values later)
  for (xiiUInt32 c = 0; c < MaxControllers; ++c)
  {
    for (xiiUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
      fVibrationToApply[c][m] = 0.0f;
  }

  // go through all controllers and motors
  for (xiiUInt8 c = 0; c < MaxControllers; ++c)
  {
    // ignore if vibration is disabled on this controller
    if (!m_bVibrationEnabled[c])
      continue;

    // check which physical controller this virtual controller is attached to
    const xiiInt8 iPhysical = GetControllerMapping(c);

    // if it is attached to any physical controller, store the vibration value
    if (iPhysical >= 0)
    {
      for (xiiUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        fVibrationToApply[(xiiUInt8)iPhysical][m] = xiiMath::Max(m_fVibrationStrength[c][m], m_fVibrationTracks[c][m][m_uiVibrationTrackPos]);
    }
  }

  // now send the back-end all the information about how to vibrate which physical controller
  // this also always resets vibration to zero for controllers that might have been changed to another virtual controller etc.
  for (xiiUInt8 c = 0; c < MaxControllers; ++c)
  {
    for (xiiUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      ApplyVibration(c, (Motor::Enum)m, fVibrationToApply[c][m]);
    }
  }
}

void xiiInputDeviceMouseKeyboard::UpdateInputSlotValues()
{
  const char* slots[3]    = {xiiInputSlot_MouseButton0, xiiInputSlot_MouseButton1, xiiInputSlot_MouseButton2};
  const char* dlbSlots[3] = {xiiInputSlot_MouseDblClick0, xiiInputSlot_MouseDblClick1, xiiInputSlot_MouseDblClick2};

  const xiiTime tNow = xiiTime::Now();

  for (int i = 0; i < 3; ++i)
  {
    m_InputSlotValues[dlbSlots[i]] = 0.0f;

    const bool bDown = m_InputSlotValues[slots[i]] > 0;
    if (bDown)
    {
      if (!m_bMouseDown[i])
      {
        if (tNow - m_LastMouseClick[i] <= m_DoubleClickTime)
        {
          m_InputSlotValues[dlbSlots[i]] = 1.0f;
          m_LastMouseClick[i].SetZero(); // this prevents triple-clicks from appearing as two double clicks
        }
        else
        {
          m_LastMouseClick[i] = tNow;
        }
      }
    }

    m_bMouseDown[i] = bDown;
  }
}

XII_STATICLINK_FILE(Core, Core_Input_DeviceTypes_Implementation_DeviceTypes);
