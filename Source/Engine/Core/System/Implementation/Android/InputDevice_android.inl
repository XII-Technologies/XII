#include <Core/System/Implementation/android/InputDevice_android.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#include <android/log.h>
#include <android_native_app_glue.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// Comment in to get verbose output on android input
// #  define DEBUG_ANDROID_INPUT

#ifdef DEBUG_ANDROID_INPUT
#  define DEBUG_LOG(...) xiiLog::Debug(__VA_ARGS__)
#else
#  define DEBUG_LOG(...)
#endif

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber)
{
  xiiAndroidUtils::s_InputEvent.AddEventHandler(xiiMakeDelegate(&xiiStandardInputDevice::AndroidInputEventHandler, this));
  xiiAndroidUtils::s_AppCommandEvent.AddEventHandler(xiiMakeDelegate(&xiiStandardInputDevice::AndroidAppCommandEventHandler, this));
}

xiiStandardInputDevice::~xiiStandardInputDevice()
{
  xiiAndroidUtils::s_AppCommandEvent.RemoveEventHandler(xiiMakeDelegate(&xiiStandardInputDevice::AndroidAppCommandEventHandler, this));
  xiiAndroidUtils::s_InputEvent.RemoveEventHandler(xiiMakeDelegate(&xiiStandardInputDevice::AndroidInputEventHandler, this));
}

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow) {}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return false;
}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) {}

xiiMouseCursorClipMode::Enum xiiStandardInputDevice::GetClipMouseCursor() const
{
  return xiiMouseCursorClipMode::Default;
}

void xiiStandardInputDevice::InitializeDevice()
{
  xiiHybridArray<xiiScreenInfo, 2> screens;
  if (xiiScreen::EnumerateScreens(screens).Succeeded())
  {
    m_iResolutionX = screens[0].m_iResolutionX;
    m_iResolutionY = screens[0].m_iResolutionY;
  }
}

void xiiStandardInputDevice::RegisterInputSlots()
{
  RegisterInputSlot(xiiInputSlot_TouchPoint0, "Touchpoint 0", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint0_PositionX, "Touchpoint 0 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint0_PositionY, "Touchpoint 0 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint1, "Touchpoint 1", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint1_PositionX, "Touchpoint 1 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint1_PositionY, "Touchpoint 1 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint2, "Touchpoint 2", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint2_PositionX, "Touchpoint 2 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint2_PositionY, "Touchpoint 2 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint3, "Touchpoint 3", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint3_PositionX, "Touchpoint 3 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint3_PositionY, "Touchpoint 3 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint4, "Touchpoint 4", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint4_PositionX, "Touchpoint 4 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint4_PositionY, "Touchpoint 4 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint5, "Touchpoint 5", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint5_PositionX, "Touchpoint 5 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint5_PositionY, "Touchpoint 5 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint6, "Touchpoint 6", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint6_PositionX, "Touchpoint 6 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint6_PositionY, "Touchpoint 6 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint7, "Touchpoint 7", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint7_PositionX, "Touchpoint 7 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint7_PositionY, "Touchpoint 7 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint8, "Touchpoint 8", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint8_PositionX, "Touchpoint 8 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint8_PositionY, "Touchpoint 8 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint9, "Touchpoint 9", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint9_PositionX, "Touchpoint 9 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint9_PositionY, "Touchpoint 9 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_MouseWheelUp, "Mousewheel Up", xiiInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(xiiInputSlot_MouseWheelDown, "Mousewheel Down", xiiInputSlotFlags::IsMouseWheel);
}

void xiiStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[xiiInputSlot_MouseWheelUp]   = 0;
  m_InputSlotValues[xiiInputSlot_MouseWheelDown] = 0;
  for (int id = 0; id < 10; ++id)
  {
    // We can't reset the position inside AndroidHandleInput as we want the position to be valid when lifting a finger. Thus, we clear the position here after the update has been performed.
    if (m_InputSlotValues[xiiInputManager::GetInputSlotTouchPoint(id)] == 0)
    {
      m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionX(id)] = 0;
      m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionY(id)] = 0;
    }
  }
}

void xiiStandardInputDevice::AndroidInputEventHandler(xiiAndroidInputEvent& event)
{
  event.m_bHandled = AndroidHandleInput(event.m_pEvent);
  SUPER::UpdateInputSlotValues();
}

void xiiStandardInputDevice::AndroidAppCommandEventHandler(xiiInt32 iCmd)
{
  if (iCmd == APP_CMD_WINDOW_RESIZED)
  {
    xiiHybridArray<xiiScreenInfo, 2> screens;
    if (xiiScreen::EnumerateScreens(screens).Succeeded())
    {
      m_iResolutionX = screens[0].m_iResolutionX;
      m_iResolutionY = screens[0].m_iResolutionY;
    }
  }
}

bool xiiStandardInputDevice::AndroidHandleInput(AInputEvent* pEvent)
{
  // #TODO_ANDROID Only touchscreen input is implemented right now.
  const xiiInt32  iEventType   = AInputEvent_getType(pEvent);
  const xiiInt32  iEventSource = AInputEvent_getSource(pEvent);
  const xiiUInt32 uiAction     = (xiiUInt32)AMotionEvent_getAction(pEvent);
  const xiiInt32  iKeyCode     = AKeyEvent_getKeyCode(pEvent);
  const xiiInt32  iButtonState = AMotionEvent_getButtonState(pEvent);
  XII_IGNORE_UNUSED(iKeyCode);
  XII_IGNORE_UNUSED(iButtonState);
  DEBUG_LOG("Android INPUT: iEventType: {}, iEventSource: {}, uiAction: {}, iKeyCode: {}, iButtonState: {}", iEventType,
            iEventSource, uiAction, iKeyCode, iButtonState);

  if (m_iResolutionX == 0 || m_iResolutionY == 0)
    return false;

  // I.e. fingers have touched the touchscreen.
  if (iEventType == AINPUT_EVENT_TYPE_MOTION && (iEventSource & AINPUT_SOURCE_TOUCHSCREEN) != 0)
  {
    // Update pointer positions
    const xiiUInt64 uiPointerCount = AMotionEvent_getPointerCount(pEvent);
    for (xiiUInt32 uiPointerIndex = 0; uiPointerIndex < uiPointerCount; uiPointerIndex++)
    {
      const float    fPixelX = AMotionEvent_getX(pEvent, uiPointerIndex);
      const float    fPixelY = AMotionEvent_getY(pEvent, uiPointerIndex);
      const xiiInt32 id      = AMotionEvent_getPointerId(pEvent, uiPointerIndex);
      if (id < 10)
      {
        m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(m_iResolutionX));
        m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(m_iResolutionY));
        DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionY(id)]);
      }
    }

    // Update pointer state
    const xiiUInt32 uiActionEvent        = uiAction & AMOTION_EVENT_ACTION_MASK;
    const xiiUInt32 uiActionPointerIndex = (uiAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    const xiiInt32 id = AMotionEvent_getPointerId(pEvent, uiActionPointerIndex);
    // We only support up to 10 touch points at the same time.
    if (id >= 10)
      return false;

    {
      // Not sure if the action finger is always present in the upper loop of uiPointerCount, so we update it here for good measure.
      const float fPixelX                                                     = AMotionEvent_getX(pEvent, uiActionPointerIndex);
      const float fPixelY                                                     = AMotionEvent_getY(pEvent, uiActionPointerIndex);
      m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionX(id)] = static_cast<float>(fPixelX / static_cast<float>(m_iResolutionX));
      m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionY(id)] = static_cast<float>(fPixelY / static_cast<float>(m_iResolutionY));
      DEBUG_LOG("Finger MOVE: {} = {} x {}", id, m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionX(id)], m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionY(id)]);
    }

    switch (uiActionEvent)
    {
      case AMOTION_EVENT_ACTION_DOWN:
      case AMOTION_EVENT_ACTION_POINTER_DOWN:
        m_InputSlotValues[xiiInputManager::GetInputSlotTouchPoint(id)] = 1;
        DEBUG_LOG("Finger DOWN: {}", id);
        return true;
      case AMOTION_EVENT_ACTION_MOVE:
        // Finger moved (we always update that at the top).
        return true;
      case AMOTION_EVENT_ACTION_UP:
      case AMOTION_EVENT_ACTION_POINTER_UP:
      case AMOTION_EVENT_ACTION_CANCEL:
      case AMOTION_EVENT_ACTION_OUTSIDE:
        m_InputSlotValues[xiiInputManager::GetInputSlotTouchPoint(id)] = 0;
        DEBUG_LOG("Finger UP: {}", id);
        return true;
      case AMOTION_EVENT_ACTION_SCROLL:
      {
        float fRotated = AMotionEvent_getAxisValue(pEvent, AMOTION_EVENT_AXIS_VSCROLL, 0);
        if (fRotated > 0)
          m_InputSlotValues[xiiInputSlot_MouseWheelUp] = fRotated;
        else
          m_InputSlotValues[xiiInputSlot_MouseWheelDown] = fRotated;
        return true;
      }
      case AMOTION_EVENT_ACTION_HOVER_ENTER:
      case AMOTION_EVENT_ACTION_HOVER_MOVE:
      case AMOTION_EVENT_ACTION_HOVER_EXIT:
        return false;
      default:
        DEBUG_LOG("Unknown AMOTION_EVENT_ACTION: {}", uiActionEvent);
        return false;
    }
  }
  return false;
}
