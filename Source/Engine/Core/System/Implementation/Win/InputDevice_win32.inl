#include <Core/Input/InputManager.h>
#include <Core/System/Implementation/Win/InputDevice_win32.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringConversion.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool xiiStandardInputDevice::s_bMainWindowUsed = false;

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber)
{
  m_uiWindowNumber = uiWindowNumber;

  if (uiWindowNumber == 0)
  {
    XII_ASSERT_RELEASE(!s_bMainWindowUsed, "You cannot have two devices of Type xiiStandardInputDevice with the window number zero.");
    xiiStandardInputDevice::s_bMainWindowUsed = true;
  }

  m_DoubleClickTime = xiiTime::Milliseconds(GetDoubleClickTime());
}

xiiStandardInputDevice::~xiiStandardInputDevice()
{
  if (!m_bShowCursor)
  {
    ShowCursor(true);
  }

  if (m_uiWindowNumber == 0)
    xiiStandardInputDevice::s_bMainWindowUsed = false;
}

void xiiStandardInputDevice::InitializeDevice()
{
  if (m_uiWindowNumber == 0)
  {
    RAWINPUTDEVICE Rid[2];

    // Keyboard
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage     = 0x06;
    Rid[0].dwFlags     = RIDEV_NOHOTKEYS; // Disables Windows-Key and Application-Key
    Rid[0].hwndTarget  = nullptr;

    // Mouse
    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage     = 0x02;
    Rid[1].dwFlags     = 0;
    Rid[1].hwndTarget  = nullptr;

    if (RegisterRawInputDevices(&Rid[0], (UINT)2, sizeof(RAWINPUTDEVICE)) == FALSE)
    {
      xiiLog::Error("Could not initialize RawInput for Mouse and Keyboard input.");
    }
    else
      xiiLog::Success("Initialized RawInput for Mouse and Keyboard input.");
  }
  else
    xiiLog::Info("Window {0} does not need to initialize Mouse or Keyboard.", m_uiWindowNumber);
}

void xiiStandardInputDevice::RegisterInputSlots()
{
  RegisterInputSlot(xiiInputSlot_KeyLeft, "Left", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyRight, "Right", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyUp, "Up", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyDown, "Down", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyEscape, "Escape", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeySpace, "Space", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyBackspace, "Backspace", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyReturn, "Return", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyTab, "Tab", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyLeftShift, "Left Shift", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyRightShift, "Right Shift", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyLeftCtrl, "Left Ctrl", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyRightCtrl, "Right Ctrl", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyLeftAlt, "Left Alt", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyRightAlt, "Right Alt", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyLeftWin, "Left Win", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyRightWin, "Right Win", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyBracketOpen, "[", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyBracketClose, "]", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeySemicolon, ";", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyApostrophe, "'", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeySlash, "/", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyEquals, "=", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyTilde, "~", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyHyphen, "-", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyComma, ",", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPeriod, ".", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyBackslash, "\\", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPipe, "|", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_Key1, "1", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key2, "2", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key3, "3", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key4, "4", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key5, "5", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key6, "6", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key7, "7", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key8, "8", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key9, "9", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_Key0, "0", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyNumpad1, "Numpad 1", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad2, "Numpad 2", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad3, "Numpad 3", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad4, "Numpad 4", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad5, "Numpad 5", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad6, "Numpad 6", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad7, "Numpad 7", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad8, "Numpad 8", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad9, "Numpad 9", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpad0, "Numpad 0", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyA, "A", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyB, "B", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyC, "C", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyD, "D", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyE, "E", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF, "F", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyG, "G", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyH, "H", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyI, "I", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyJ, "J", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyK, "K", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyL, "L", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyM, "M", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyN, "N", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyO, "O", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyP, "P", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyQ, "Q", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyR, "R", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyS, "S", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyT, "T", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyU, "U", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyV, "V", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyW, "W", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyX, "X", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyY, "Y", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyZ, "Z", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyF1, "F1", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF2, "F2", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF3, "F3", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF4, "F4", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF5, "F5", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF6, "F6", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF7, "F7", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF8, "F8", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF9, "F9", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF10, "F10", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF11, "F11", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyF12, "F12", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyHome, "Home", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyEnd, "End", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyDelete, "Delete", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyInsert, "Insert", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPageUp, "Page Up", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPageDown, "Page Down", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyNumLock, "Numlock", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpadPlus, "Numpad +", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpadMinus, "Numpad -", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpadStar, "Numpad *", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpadSlash, "Numpad /", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpadPeriod, "Numpad .", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNumpadEnter, "Enter", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyCapsLock, "Capslock", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPrint, "Print", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyScroll, "Scroll", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPause, "Pause", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyApps, "Application", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_KeyPrevTrack, "Previous Track", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNextTrack, "Next Track", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPlayPause, "Play / Pause", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyStop, "Stop", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyVolumeUp, "Volume Up", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyVolumeDown, "Volume Down", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyMute, "Mute", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_MouseWheelUp, "Mousewheel Up", xiiInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(xiiInputSlot_MouseWheelDown, "Mousewheel Down", xiiInputSlotFlags::IsMouseWheel);

  RegisterInputSlot(xiiInputSlot_MouseMoveNegX, "Mouse Move Left", xiiInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(xiiInputSlot_MouseMovePosX, "Mouse Move Right", xiiInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(xiiInputSlot_MouseMoveNegY, "Mouse Move Down", xiiInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(xiiInputSlot_MouseMovePosY, "Mouse Move Up", xiiInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(xiiInputSlot_MouseButton0, "Mousebutton 0", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton1, "Mousebutton 1", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton2, "Mousebutton 2", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton3, "Mousebutton 3", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton4, "Mousebutton 4", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_MouseDblClick0, "Left Double Click", xiiInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(xiiInputSlot_MouseDblClick1, "Right Double Click", xiiInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(xiiInputSlot_MouseDblClick2, "Middle Double Click", xiiInputSlotFlags::IsDoubleClick);

  RegisterInputSlot(xiiInputSlot_MousePositionX, "Mouse Position X", xiiInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(xiiInputSlot_MousePositionY, "Mouse Position Y", xiiInputSlotFlags::IsMouseAxisPosition);


  RegisterInputSlot(xiiInputSlot_TouchPoint0, "Touchpoint 1", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint0_PositionX, "Touchpoint 1 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint0_PositionY, "Touchpoint 1 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint1, "Touchpoint 2", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint1_PositionX, "Touchpoint 2 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint1_PositionY, "Touchpoint 2 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint2, "Touchpoint 3", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint2_PositionX, "Touchpoint 3 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint2_PositionY, "Touchpoint 3 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint3, "Touchpoint 4", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint3_PositionX, "Touchpoint 4 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint3_PositionY, "Touchpoint 4 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint4, "Touchpoint 5", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint4_PositionX, "Touchpoint 5 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint4_PositionY, "Touchpoint 5 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint5, "Touchpoint 6", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint5_PositionX, "Touchpoint 6 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint5_PositionY, "Touchpoint 6 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint6, "Touchpoint 7", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint6_PositionX, "Touchpoint 7 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint6_PositionY, "Touchpoint 7 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint7, "Touchpoint 8", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint7_PositionX, "Touchpoint 8 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint7_PositionY, "Touchpoint 8 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint8, "Touchpoint 9", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint8_PositionX, "Touchpoint 9 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint8_PositionY, "Touchpoint 9 Position Y", xiiInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(xiiInputSlot_TouchPoint9, "Touchpoint 10", xiiInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(xiiInputSlot_TouchPoint9_PositionX, "Touchpoint 10 Position X", xiiInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(xiiInputSlot_TouchPoint9_PositionY, "Touchpoint 10 Position Y", xiiInputSlotFlags::IsTouchPosition);
}

void xiiStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[xiiInputSlot_MouseWheelUp]   = 0;
  m_InputSlotValues[xiiInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegX]  = 0;
  m_InputSlotValues[xiiInputSlot_MouseMovePosX]  = 0;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegY]  = 0;
  m_InputSlotValues[xiiInputSlot_MouseMovePosY]  = 0;
  m_InputSlotValues[xiiInputSlot_MouseDblClick0] = 0;
  m_InputSlotValues[xiiInputSlot_MouseDblClick1] = 0;
  m_InputSlotValues[xiiInputSlot_MouseDblClick2] = 0;
}

void xiiStandardInputDevice::UpdateInputSlotValues()
{
  const char* slotDown[5] = {xiiInputSlot_MouseButton0, xiiInputSlot_MouseButton1, xiiInputSlot_MouseButton2, xiiInputSlot_MouseButton3, xiiInputSlot_MouseButton4};

  // Do not read uninitialized values
  if (!m_InputSlotValues.Contains(slotDown[4]))
  {
    for (xiiInt32 i = 0; i < 5; ++i)
    {
      m_InputSlotValues[slotDown[i]] = 0;
    }
  }

  for (xiiInt32 i = 0; i < 5; ++i)
  {
    if (m_InputSlotValues[slotDown[i]] > 0)
    {
      if (m_uiMouseButtonReceivedUp[i] > 0)
      {
        --m_uiMouseButtonReceivedUp[i];
        m_InputSlotValues[slotDown[i]] = 0;
      }
    }
    else
    {
      if (m_uiMouseButtonReceivedDown[i] > 0)
      {
        --m_uiMouseButtonReceivedDown[i];
        m_InputSlotValues[slotDown[i]] = 1.0f;
      }
    }
  }

  SUPER::UpdateInputSlotValues();
}

void xiiStandardInputDevice::ApplyClipRect(xiiMouseCursorClipMode::Enum mode, xiiMinWindows::HWND hWnd)
{
  if (!m_bApplyClipRect)
    return;

  m_bApplyClipRect = false;

  if (mode == xiiMouseCursorClipMode::NoClip)
  {
    ClipCursor(nullptr);
    return;
  }

  RECT r;
  {
    RECT area;
    GetClientRect(xiiMinWindows::ToNative(hWnd), &area);
    POINT p0, p1;
    p0.x = 0;
    p0.y = 0;
    p1.x = area.right;
    p1.y = area.bottom;

    ClientToScreen(xiiMinWindows::ToNative(hWnd), &p0);
    ClientToScreen(xiiMinWindows::ToNative(hWnd), &p1);

    r.top    = p0.y;
    r.left   = p0.x;
    r.right  = p1.x;
    r.bottom = p1.y;
  }

  if (mode == xiiMouseCursorClipMode::ClipToPosition)
  {
    POINT mp;
    if (GetCursorPos(&mp))
    {
      // Ensure the position is inside the window rect
      mp.x = xiiMath::Clamp(mp.x, r.left, r.right);
      mp.y = xiiMath::Clamp(mp.y, r.top, r.bottom);

      r.top    = mp.y;
      r.bottom = mp.y;
      r.left   = mp.x;
      r.right  = mp.x;
    }
  }

  ClipCursor(&r);
}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode)
{
  if (m_ClipCursorMode == mode)
    return;

  m_ClipCursorMode = mode;
  m_bApplyClipRect = m_ClipCursorMode != xiiMouseCursorClipMode::NoClip;

  if (m_ClipCursorMode == xiiMouseCursorClipMode::NoClip)
    ClipCursor(nullptr);
}

// WM_INPUT mouse clicks do not work in some VMs.
// When this is enabled, mouse clicks are retrieved via standard WM_LBUTTONDOWN.
#define XII_MOUSEBUTTON_COMPATIBILTY_MODE XII_ON

void xiiStandardInputDevice::WindowMessage(xiiMinWindows::HWND pWnd, xiiMinWindows::UINT msg, xiiMinWindows::WPARAM wparam, xiiMinWindows::LPARAM lparam)
{
#if XII_ENABLED(XII_MOUSEBUTTON_COMPATIBILTY_MODE)
  static xiiInt32 s_iMouseCaptureCount = 0;
#endif

  switch (msg)
  {
    case WM_MOUSEWHEEL:
    {
      // The mousewheel does not work with rawinput over touchpads (at least not all)
      // So we handle that one individually

      const xiiInt32 iRotated = (xiiInt16)HIWORD(wparam);

      if (iRotated > 0)
        m_InputSlotValues[xiiInputSlot_MouseWheelUp] = iRotated / 120.0f;
      else
        m_InputSlotValues[xiiInputSlot_MouseWheelDown] = iRotated / -120.0f;
    }
    break;

    case WM_MOUSEMOVE:
    {
      RECT area;
      GetClientRect(xiiMinWindows::ToNative(pWnd), &area);

      const xiiUInt32 uiResX = area.right - area.left;
      const xiiUInt32 uiResY = area.bottom - area.top;

      const float fPosX = (float)((xiiInt16)LOWORD(lparam));
      const float fPosY = (float)((xiiInt16)HIWORD(lparam));

      s_iMouseIsOverWindowNumber                     = m_uiWindowNumber;
      m_InputSlotValues[xiiInputSlot_MousePositionX] = (fPosX / uiResX);
      m_InputSlotValues[xiiInputSlot_MousePositionY] = (fPosY / uiResY);

      if (m_ClipCursorMode == xiiMouseCursorClipMode::ClipToPosition || m_ClipCursorMode == xiiMouseCursorClipMode::ClipToWindowImmediate)
      {
        ApplyClipRect(m_ClipCursorMode, pWnd);
      }
    }
    break;

    case WM_SETFOCUS:
    {
      m_bApplyClipRect = true;
      ApplyClipRect(m_ClipCursorMode, pWnd);
    }
    break;

    case WM_KILLFOCUS:
    {
      OnFocusLost(pWnd);
      return;
    }

    case WM_CHAR:
    {
      m_uiLastCharacter = (wchar_t)wparam;
      return;
    }

    // These messages would only arrive, if the window had the flag CS_DBLCLKS
    // see https://docs.microsoft.com/windows/win32/inputdev/wm-lbuttondblclk
    // this would add lag and hide single clicks when the user double clicks
    // therefore it is not used
#if 0
    case WM_LBUTTONDBLCLK:
     {
      m_InputSlotValues[xiiInputSlot_MouseDblClick0] = 1.0f;
      return;
     }

    case WM_RBUTTONDBLCLK:
     {
      m_InputSlotValues[xiiInputSlot_MouseDblClick1] = 1.0f;
      return;
     }

    case WM_MBUTTONDBLCLK:
     {
      m_InputSlotValues[xiiInputSlot_MouseDblClick2] = 1.0f;
      return;
     }
#endif

#if XII_ENABLED(XII_MOUSEBUTTON_COMPATIBILTY_MODE)

    case WM_LBUTTONDOWN:
    {
      m_uiMouseButtonReceivedDown[0]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(xiiMinWindows::ToNative(pWnd));
      ++s_iMouseCaptureCount;

      return;
    }

    case WM_LBUTTONUP:
    {
      m_uiMouseButtonReceivedUp[0]++;
      ApplyClipRect(m_ClipCursorMode, pWnd);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;
    }

    case WM_RBUTTONDOWN:
    {
      m_uiMouseButtonReceivedDown[1]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(xiiMinWindows::ToNative(pWnd));
      ++s_iMouseCaptureCount;

      return;
    }

    case WM_RBUTTONUP:
    {
      m_uiMouseButtonReceivedUp[1]++;
      ApplyClipRect(m_ClipCursorMode, pWnd);

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;
    }

    case WM_MBUTTONDOWN:
    {
      m_uiMouseButtonReceivedDown[2]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(xiiMinWindows::ToNative(pWnd));
      ++s_iMouseCaptureCount;

      return;
    }

    case WM_MBUTTONUP:
    {
      m_uiMouseButtonReceivedUp[2]++;

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;
    }

    case WM_XBUTTONDOWN:
    {
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
        m_uiMouseButtonReceivedDown[3]++;
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
        m_uiMouseButtonReceivedDown[4]++;

      if (s_iMouseCaptureCount == 0)
        SetCapture(xiiMinWindows::ToNative(pWnd));
      ++s_iMouseCaptureCount;

      return;
    }

    case WM_XBUTTONUP:
    {
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
        m_uiMouseButtonReceivedUp[3]++;
      if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
        m_uiMouseButtonReceivedUp[4]++;

      --s_iMouseCaptureCount;
      if (s_iMouseCaptureCount <= 0)
        ReleaseCapture();

      return;
    }

    case WM_CAPTURECHANGED: // Sent to the window that is losing the mouse capture.
    {
      s_iMouseCaptureCount = 0;
      return;
    }

#else

    case WM_LBUTTONUP:
    {
      ApplyClipRect(m_bClipCursor, hWnd);
      return;
    }

#endif

    case WM_INPUT:
    {
      xiiUInt32 uiSize = 0;

      GetRawInputData((HRAWINPUT)lparam, RID_INPUT, nullptr, &uiSize, sizeof(RAWINPUTHEADER));

      if (uiSize == 0)
        return;

      xiiHybridArray<xiiUInt8, sizeof(RAWINPUT)> InputData;
      InputData.SetCountUninitialized(uiSize);

      if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &InputData[0], &uiSize, sizeof(RAWINPUTHEADER)) != uiSize)
        return;

      RAWINPUT* raw = (RAWINPUT*)&InputData[0];

      if (raw->header.dwType == RIM_TYPEKEYBOARD)
      {
        static bool bIgnoreNext = false;

        if (bIgnoreNext)
        {
          bIgnoreNext = false;
          return;
        }

        static bool bWasLeftShift = false;

        const xiiUInt8 uiScanCode  = static_cast<xiiUInt8>(raw->data.keyboard.MakeCode);
        const bool     bIsExtended = (raw->data.keyboard.Flags & RI_KEY_E0) != 0;

        if (uiScanCode == 42 && bIsExtended) // 42 has to be special I guess
        {
          bWasLeftShift = true;
          return;
        }

        xiiStringView sInputSlotName = xiiInputManager::ConvertScanCodeToEngineName(uiScanCode, bIsExtended);

        // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
        // so we need to fix this manually
        if (raw->data.keyboard.Flags & RI_KEY_E1)
        {
          sInputSlotName = xiiInputSlot_KeyPause;
          bIgnoreNext    = true;
        }

        // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
        // we ignore the first shift key entirely and then modify the following Numpad* key.
        // Note that the 'shift' is sent along with several other keys as well (e.g. left/right/up/down arrows).
        // In these cases we can ignore them entirely, as the following key will have an unambiguous key code.
        if ((sInputSlotName == xiiInputSlot_KeyNumpadStar) && bWasLeftShift)
          sInputSlotName = xiiInputSlot_KeyPrint;

        bWasLeftShift = false;

        xiiInt32 iRequest = raw->data.keyboard.MakeCode << 16;

        if (raw->data.keyboard.Flags & RI_KEY_E0)
          iRequest |= 1 << 24;

        const bool bPressed = !(raw->data.keyboard.Flags & 0x01);

        m_InputSlotValues[sInputSlotName] = bPressed ? 1.0f : 0.0f;

        if ((m_InputSlotValues[xiiInputSlot_KeyLeftCtrl] > 0.1f) && (m_InputSlotValues[xiiInputSlot_KeyLeftAlt] > 0.1f) &&
            (m_InputSlotValues[xiiInputSlot_KeyNumpadEnter] > 0.1f))
        {
          switch (GetClipMouseCursor())
          {
            case xiiMouseCursorClipMode::NoClip:
              SetClipMouseCursor(xiiMouseCursorClipMode::ClipToWindow);
              break;

            default:
              SetClipMouseCursor(xiiMouseCursorClipMode::NoClip);
              break;
          }
        }
      }
      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        const xiiUInt32 uiButtons = raw->data.mouse.usButtonFlags;

        // The "absolute" positions are only reported by devices such as Pens.
        // If at all, we should handle them as touch points, not as mouse positions.
        if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
          m_InputSlotValues[xiiInputSlot_MouseMoveNegX] +=
            ((raw->data.mouse.lLastX < 0) ? (float)-raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x;
          m_InputSlotValues[xiiInputSlot_MouseMovePosX] +=
            ((raw->data.mouse.lLastX > 0) ? (float)raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x;
          m_InputSlotValues[xiiInputSlot_MouseMoveNegY] +=
            ((raw->data.mouse.lLastY < 0) ? (float)-raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y;
          m_InputSlotValues[xiiInputSlot_MouseMovePosY] +=
            ((raw->data.mouse.lLastY > 0) ? (float)raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y;

          // Mouse input does not always work via WM_INPUT
          // e.g. some VMs don't send mouse click input via WM_INPUT when the mouse cursor is visible
          // therefore in 'compatibility mode' it is just queried via standard WM_LBUTTONDOWN etc.
          // to get 'high performance' mouse clicks, this code would work fine though
          // but I doubt it makes much difference in latency
#if XII_DISABLED(XII_MOUSEBUTTON_COMPATIBILTY_MODE)
          for (xiiInt32 mb = 0; mb < 5; ++mb)
          {
            char szTemp[32];
            xiiStringUtils::snprintf(szTemp, 32, "mouse_button_%i", mb);

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2))) != 0)
              m_InputSlotValues[szTemp] = 1.0f;

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2 + 1))) != 0)
              m_InputSlotValues[szTemp] = 0.0f;
          }
#endif
        }
        else if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        {
          if ((raw->data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0)
          {
            // If this flag is set, we are getting mouse input through a remote desktop session
            // and that means we will not get any relative mouse move events, so we need to emulate them

            static const xiiInt32 iVirtualDesktopW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            static const xiiInt32 iVirtualDesktopH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

            static xiiVec2 vLastPos(xiiMath::MaxValue<float>());
            const xiiVec2  vNewPos(
              (raw->data.mouse.lLastX / 65535.0f) * iVirtualDesktopW, (raw->data.mouse.lLastY / 65535.0f) * iVirtualDesktopH);

            if (vLastPos.x != xiiMath::MaxValue<float>())
            {
              const xiiVec2 vDiff = vNewPos - vLastPos;

              m_InputSlotValues[xiiInputSlot_MouseMoveNegX] += ((vDiff.x < 0) ? (float)-vDiff.x : 0.0f) * GetMouseSpeed().x;
              m_InputSlotValues[xiiInputSlot_MouseMovePosX] += ((vDiff.x > 0) ? (float)vDiff.x : 0.0f) * GetMouseSpeed().x;
              m_InputSlotValues[xiiInputSlot_MouseMoveNegY] += ((vDiff.y < 0) ? (float)-vDiff.y : 0.0f) * GetMouseSpeed().y;
              m_InputSlotValues[xiiInputSlot_MouseMovePosY] += ((vDiff.y > 0) ? (float)vDiff.y : 0.0f) * GetMouseSpeed().y;
            }

            vLastPos = vNewPos;
          }
          else
          {
            static xiiInt32 iTouchPoint     = 0;
            static bool     bTouchPointDown = false;

            xiiStringView sSlot  = xiiInputManager::GetInputSlotTouchPoint(iTouchPoint);
            xiiStringView sSlotX = xiiInputManager::GetInputSlotTouchPointPositionX(iTouchPoint);
            xiiStringView sSlotY = xiiInputManager::GetInputSlotTouchPointPositionY(iTouchPoint);

            m_InputSlotValues[sSlotX] = (raw->data.mouse.lLastX / 65535.0f) + m_uiWindowNumber;
            m_InputSlotValues[sSlotY] = (raw->data.mouse.lLastY / 65535.0f);

            if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN)) != 0)
            {
              bTouchPointDown          = true;
              m_InputSlotValues[sSlot] = 1.0f;
            }

            if ((uiButtons & (RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_UP)) != 0)
            {
              bTouchPointDown          = false;
              m_InputSlotValues[sSlot] = 0.0f;
            }
          }
        }
        else
        {
          xiiLog::Info("Unknown Mouse Move: {0} | {1}, Flags = {2}", xiiArgF(raw->data.mouse.lLastX, 1), xiiArgF(raw->data.mouse.lLastY, 1),
                       (xiiUInt32)raw->data.mouse.usFlags);
        }
      }
    }
  }
}


static void SetKeyNameForScanCode(int iScanCode, bool bExtended, const char* szInputSlot)
{
  const xiiUInt32 uiKeyCode = (iScanCode << 16) | (bExtended ? (1 << 24) : 0);

  wchar_t szKeyName[32] = {0};
  GetKeyNameTextW(uiKeyCode, szKeyName, 30);

  xiiStringUtf8 sName(szKeyName);

  xiiLog::Dev("Translated '{0}' to '{1}'", xiiInputManager::GetInputSlotDisplayName(szInputSlot), sName.GetData());

  xiiInputManager::SetInputSlotDisplayName(szInputSlot, sName.GetData());
}

void xiiStandardInputDevice::LocalizeButtonDisplayNames()
{
  XII_LOG_BLOCK("xiiStandardInputDevice::LocalizeButtonDisplayNames");

  SetKeyNameForScanCode(1, false, xiiInputSlot_KeyEscape);
  SetKeyNameForScanCode(2, false, xiiInputSlot_Key1);
  SetKeyNameForScanCode(3, false, xiiInputSlot_Key2);
  SetKeyNameForScanCode(4, false, xiiInputSlot_Key3);
  SetKeyNameForScanCode(5, false, xiiInputSlot_Key4);
  SetKeyNameForScanCode(6, false, xiiInputSlot_Key5);
  SetKeyNameForScanCode(7, false, xiiInputSlot_Key6);
  SetKeyNameForScanCode(8, false, xiiInputSlot_Key7);
  SetKeyNameForScanCode(9, false, xiiInputSlot_Key8);
  SetKeyNameForScanCode(10, false, xiiInputSlot_Key9);
  SetKeyNameForScanCode(11, false, xiiInputSlot_Key0);

  SetKeyNameForScanCode(12, false, xiiInputSlot_KeyHyphen);
  SetKeyNameForScanCode(13, false, xiiInputSlot_KeyEquals);
  SetKeyNameForScanCode(14, false, xiiInputSlot_KeyBackspace);

  SetKeyNameForScanCode(15, false, xiiInputSlot_KeyTab);
  SetKeyNameForScanCode(16, false, xiiInputSlot_KeyQ);
  SetKeyNameForScanCode(17, false, xiiInputSlot_KeyW);
  SetKeyNameForScanCode(18, false, xiiInputSlot_KeyE);
  SetKeyNameForScanCode(19, false, xiiInputSlot_KeyR);
  SetKeyNameForScanCode(20, false, xiiInputSlot_KeyT);
  SetKeyNameForScanCode(21, false, xiiInputSlot_KeyY);
  SetKeyNameForScanCode(22, false, xiiInputSlot_KeyU);
  SetKeyNameForScanCode(23, false, xiiInputSlot_KeyI);
  SetKeyNameForScanCode(24, false, xiiInputSlot_KeyO);
  SetKeyNameForScanCode(25, false, xiiInputSlot_KeyP);
  SetKeyNameForScanCode(26, false, xiiInputSlot_KeyBracketOpen);
  SetKeyNameForScanCode(27, false, xiiInputSlot_KeyBracketClose);
  SetKeyNameForScanCode(28, false, xiiInputSlot_KeyReturn);

  SetKeyNameForScanCode(29, false, xiiInputSlot_KeyLeftCtrl);
  SetKeyNameForScanCode(30, false, xiiInputSlot_KeyA);
  SetKeyNameForScanCode(31, false, xiiInputSlot_KeyS);
  SetKeyNameForScanCode(32, false, xiiInputSlot_KeyD);
  SetKeyNameForScanCode(33, false, xiiInputSlot_KeyF);
  SetKeyNameForScanCode(34, false, xiiInputSlot_KeyG);
  SetKeyNameForScanCode(35, false, xiiInputSlot_KeyH);
  SetKeyNameForScanCode(36, false, xiiInputSlot_KeyJ);
  SetKeyNameForScanCode(37, false, xiiInputSlot_KeyK);
  SetKeyNameForScanCode(38, false, xiiInputSlot_KeyL);
  SetKeyNameForScanCode(39, false, xiiInputSlot_KeySemicolon);
  SetKeyNameForScanCode(40, false, xiiInputSlot_KeyApostrophe);

  SetKeyNameForScanCode(41, false, xiiInputSlot_KeyTilde);
  SetKeyNameForScanCode(42, false, xiiInputSlot_KeyLeftShift);
  SetKeyNameForScanCode(43, false, xiiInputSlot_KeyBackslash);

  SetKeyNameForScanCode(44, false, xiiInputSlot_KeyZ);
  SetKeyNameForScanCode(45, false, xiiInputSlot_KeyX);
  SetKeyNameForScanCode(46, false, xiiInputSlot_KeyC);
  SetKeyNameForScanCode(47, false, xiiInputSlot_KeyV);
  SetKeyNameForScanCode(48, false, xiiInputSlot_KeyB);
  SetKeyNameForScanCode(49, false, xiiInputSlot_KeyN);
  SetKeyNameForScanCode(50, false, xiiInputSlot_KeyM);
  SetKeyNameForScanCode(51, false, xiiInputSlot_KeyComma);
  SetKeyNameForScanCode(52, false, xiiInputSlot_KeyPeriod);
  SetKeyNameForScanCode(53, false, xiiInputSlot_KeySlash);
  SetKeyNameForScanCode(54, false, xiiInputSlot_KeyRightShift);

  SetKeyNameForScanCode(55, false, xiiInputSlot_KeyNumpadStar); // Overlaps with Print

  SetKeyNameForScanCode(56, false, xiiInputSlot_KeyLeftAlt);
  SetKeyNameForScanCode(57, false, xiiInputSlot_KeySpace);
  SetKeyNameForScanCode(58, false, xiiInputSlot_KeyCapsLock);

  SetKeyNameForScanCode(59, false, xiiInputSlot_KeyF1);
  SetKeyNameForScanCode(60, false, xiiInputSlot_KeyF2);
  SetKeyNameForScanCode(61, false, xiiInputSlot_KeyF3);
  SetKeyNameForScanCode(62, false, xiiInputSlot_KeyF4);
  SetKeyNameForScanCode(63, false, xiiInputSlot_KeyF5);
  SetKeyNameForScanCode(64, false, xiiInputSlot_KeyF6);
  SetKeyNameForScanCode(65, false, xiiInputSlot_KeyF7);
  SetKeyNameForScanCode(66, false, xiiInputSlot_KeyF8);
  SetKeyNameForScanCode(67, false, xiiInputSlot_KeyF9);
  SetKeyNameForScanCode(68, false, xiiInputSlot_KeyF10);

  SetKeyNameForScanCode(69, true, xiiInputSlot_KeyNumLock); // Prints 'Pause' if it is not 'extended'
  SetKeyNameForScanCode(70, false, xiiInputSlot_KeyScroll); // This overlaps with Pause

  SetKeyNameForScanCode(71, false, xiiInputSlot_KeyNumpad7); // This overlaps with Home
  SetKeyNameForScanCode(72, false, xiiInputSlot_KeyNumpad8); // This overlaps with Arrow Up
  SetKeyNameForScanCode(73, false, xiiInputSlot_KeyNumpad9); // This overlaps with Page Up
  SetKeyNameForScanCode(74, false, xiiInputSlot_KeyNumpadMinus);

  SetKeyNameForScanCode(75, false, xiiInputSlot_KeyNumpad4); // This overlaps with Arrow Left
  SetKeyNameForScanCode(76, false, xiiInputSlot_KeyNumpad5);
  SetKeyNameForScanCode(77, false, xiiInputSlot_KeyNumpad6); // This overlaps with Arrow Right
  SetKeyNameForScanCode(78, false, xiiInputSlot_KeyNumpadPlus);

  SetKeyNameForScanCode(79, false, xiiInputSlot_KeyNumpad1);      // This overlaps with End
  SetKeyNameForScanCode(80, false, xiiInputSlot_KeyNumpad2);      // This overlaps with Arrow Down
  SetKeyNameForScanCode(81, false, xiiInputSlot_KeyNumpad3);      // This overlaps with Page Down
  SetKeyNameForScanCode(82, false, xiiInputSlot_KeyNumpad0);      // This overlaps with Insert
  SetKeyNameForScanCode(83, false, xiiInputSlot_KeyNumpadPeriod); // This overlaps with Insert

  SetKeyNameForScanCode(86, false, xiiInputSlot_KeyPipe);

  SetKeyNameForScanCode(87, false, xiiInputSlot_KeyF11);
  SetKeyNameForScanCode(88, false, xiiInputSlot_KeyF12);

  SetKeyNameForScanCode(91, true, xiiInputSlot_KeyLeftWin);  // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(92, true, xiiInputSlot_KeyRightWin); // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(93, true, xiiInputSlot_KeyApps);     // Prints '' if it is not 'extended'

  // 'Extended' keys
  SetKeyNameForScanCode(28, true, xiiInputSlot_KeyNumpadEnter);
  SetKeyNameForScanCode(29, true, xiiInputSlot_KeyRightCtrl);
  SetKeyNameForScanCode(53, true, xiiInputSlot_KeyNumpadSlash);
  SetKeyNameForScanCode(55, true, xiiInputSlot_KeyPrint);
  SetKeyNameForScanCode(56, true, xiiInputSlot_KeyRightAlt);
  SetKeyNameForScanCode(70, true, xiiInputSlot_KeyPause);
  SetKeyNameForScanCode(71, true, xiiInputSlot_KeyHome);
  SetKeyNameForScanCode(72, true, xiiInputSlot_KeyUp);
  SetKeyNameForScanCode(73, true, xiiInputSlot_KeyPageUp);

  SetKeyNameForScanCode(75, true, xiiInputSlot_KeyLeft);
  SetKeyNameForScanCode(77, true, xiiInputSlot_KeyRight);

  SetKeyNameForScanCode(79, true, xiiInputSlot_KeyEnd);
  SetKeyNameForScanCode(80, true, xiiInputSlot_KeyDown);
  SetKeyNameForScanCode(81, true, xiiInputSlot_KeyPageDown);
  SetKeyNameForScanCode(82, true, xiiInputSlot_KeyInsert);
  SetKeyNameForScanCode(83, true, xiiInputSlot_KeyDelete);
}

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  m_bShowCursor = bShow;
  ShowCursor(m_bShowCursor);
}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}

void xiiStandardInputDevice::OnFocusLost(xiiMinWindows::HWND hWnd)
{
  m_bApplyClipRect = true;
  ApplyClipRect(xiiMouseCursorClipMode::NoClip, hWnd);

  auto it = m_InputSlotValues.GetIterator();

  while (it.IsValid())
  {
    it.Value() = 0.0f;
    it.Next();
  }

  const char* slotDown[5] = {xiiInputSlot_MouseButton0, xiiInputSlot_MouseButton1, xiiInputSlot_MouseButton2, xiiInputSlot_MouseButton3, xiiInputSlot_MouseButton4};

  static_assert(XII_ARRAY_SIZE(m_uiMouseButtonReceivedDown) == XII_ARRAY_SIZE(slotDown));

  for (xiiInt32 i = 0; i < XII_ARRAY_SIZE(m_uiMouseButtonReceivedDown); ++i)
  {
    m_uiMouseButtonReceivedDown[i] = 0;
    m_uiMouseButtonReceivedUp[i]   = 0;

    m_InputSlotValues[slotDown[i]] = 0;
  }
}
