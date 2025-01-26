#include <Core/CorePCH.h>

#if XII_ENABLED(XII_SUPPORTS_SDL)

#  include <Core/CorePCH.h>
#  include <Core/Input/InputManager.h>
#  include <Core/Platform/SDL/InputDevice_SDL.h>

#  include <SDL3/SDL_events.h>
#  include <SDL3/SDL_keyboard.h>
#  include <SDL3/SDL_keycode.h>
#  include <SDL3/SDL_mouse.h>

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  endif

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool xiiStandardInputDevice::s_bMainWindowUsed = false;

namespace
{
  const char* ConvertSDLKeyToEngineName(SDL_Keycode key)
  {
    switch (key)
    {
      case SDLK_LEFT:
        return xiiInputSlot_KeyLeft;
      case SDLK_RIGHT:
        return xiiInputSlot_KeyRight;
      case SDLK_UP:
        return xiiInputSlot_KeyUp;
      case SDLK_DOWN:
        return xiiInputSlot_KeyDown;
      case SDLK_ESCAPE:
        return xiiInputSlot_KeyEscape;
      case SDLK_SPACE:
        return xiiInputSlot_KeySpace;
      case SDLK_BACKSPACE:
        return xiiInputSlot_KeyBackspace;
      case SDLK_RETURN:
        return xiiInputSlot_KeyReturn;
      case SDLK_TAB:
        return xiiInputSlot_KeyTab;
      case SDLK_LSHIFT:
        return xiiInputSlot_KeyLeftShift;
      case SDLK_RSHIFT:
        return xiiInputSlot_KeyRightShift;
      case SDLK_LCTRL:
        return xiiInputSlot_KeyLeftCtrl;
      case SDLK_RCTRL:
        return xiiInputSlot_KeyRightCtrl;
      case SDLK_LALT:
        return xiiInputSlot_KeyLeftAlt;
      case SDLK_RALT:
        return xiiInputSlot_KeyRightAlt;
      case SDLK_LGUI:
        return xiiInputSlot_KeyLeftWin;
      case SDLK_RGUI:
        return xiiInputSlot_KeyRightWin;
      case SDLK_APPLICATION:
        return xiiInputSlot_KeyApps;
      case SDLK_LEFTBRACKET:
        return xiiInputSlot_KeyBracketOpen;
      case SDLK_RIGHTBRACKET:
        return xiiInputSlot_KeyBracketClose;
      case SDLK_SEMICOLON:
        return xiiInputSlot_KeySemicolon;
      case SDLK_APOSTROPHE:
        return xiiInputSlot_KeyApostrophe;
      case SDLK_SLASH:
        return xiiInputSlot_KeySlash;
      case SDLK_EQUALS:
        return xiiInputSlot_KeyEquals;
      case SDLK_TILDE:
        return xiiInputSlot_KeyTilde;
      case SDLK_MINUS:
        return xiiInputSlot_KeyHyphen;
      case SDLK_COMMA:
        return xiiInputSlot_KeyComma;
      case SDLK_PERIOD:
        return xiiInputSlot_KeyPeriod;
      case SDLK_BACKSLASH:
        return xiiInputSlot_KeyBackslash;
      case SDLK_PIPE:
        return xiiInputSlot_KeyPipe;
      case SDLK_1:
        return xiiInputSlot_Key1;
      case SDLK_2:
        return xiiInputSlot_Key2;
      case SDLK_3:
        return xiiInputSlot_Key3;
      case SDLK_4:
        return xiiInputSlot_Key4;
      case SDLK_5:
        return xiiInputSlot_Key5;
      case SDLK_6:
        return xiiInputSlot_Key6;
      case SDLK_7:
        return xiiInputSlot_Key7;
      case SDLK_8:
        return xiiInputSlot_Key8;
      case SDLK_9:
        return xiiInputSlot_Key9;
      case SDLK_0:
        return xiiInputSlot_Key0;
      case SDLK_KP_1:
        return xiiInputSlot_KeyNumpad1;
      case SDLK_KP_2:
        return xiiInputSlot_KeyNumpad2;
      case SDLK_KP_3:
        return xiiInputSlot_KeyNumpad3;
      case SDLK_KP_4:
        return xiiInputSlot_KeyNumpad4;
      case SDLK_KP_5:
        return xiiInputSlot_KeyNumpad5;
      case SDLK_KP_6:
        return xiiInputSlot_KeyNumpad6;
      case SDLK_KP_7:
        return xiiInputSlot_KeyNumpad7;
      case SDLK_KP_8:
        return xiiInputSlot_KeyNumpad8;
      case SDLK_KP_9:
        return xiiInputSlot_KeyNumpad9;
      case SDLK_KP_0:
        return xiiInputSlot_KeyNumpad0;
      case SDLK_A:
        return xiiInputSlot_KeyA;
      case SDLK_B:
        return xiiInputSlot_KeyB;
      case SDLK_C:
        return xiiInputSlot_KeyC;
      case SDLK_D:
        return xiiInputSlot_KeyD;
      case SDLK_E:
        return xiiInputSlot_KeyE;
      case SDLK_F:
        return xiiInputSlot_KeyF;
      case SDLK_G:
        return xiiInputSlot_KeyG;
      case SDLK_H:
        return xiiInputSlot_KeyH;
      case SDLK_I:
        return xiiInputSlot_KeyI;
      case SDLK_J:
        return xiiInputSlot_KeyJ;
      case SDLK_K:
        return xiiInputSlot_KeyK;
      case SDLK_L:
        return xiiInputSlot_KeyL;
      case SDLK_M:
        return xiiInputSlot_KeyM;
      case SDLK_N:
        return xiiInputSlot_KeyN;
      case SDLK_O:
        return xiiInputSlot_KeyO;
      case SDLK_P:
        return xiiInputSlot_KeyP;
      case SDLK_Q:
        return xiiInputSlot_KeyQ;
      case SDLK_R:
        return xiiInputSlot_KeyR;
      case SDLK_S:
        return xiiInputSlot_KeyS;
      case SDLK_T:
        return xiiInputSlot_KeyT;
      case SDLK_U:
        return xiiInputSlot_KeyU;
      case SDLK_V:
        return xiiInputSlot_KeyV;
      case SDLK_W:
        return xiiInputSlot_KeyW;
      case SDLK_X:
        return xiiInputSlot_KeyX;
      case SDLK_Y:
        return xiiInputSlot_KeyY;
      case SDLK_Z:
        return xiiInputSlot_KeyZ;
      case SDLK_F1:
        return xiiInputSlot_KeyF1;
      case SDLK_F2:
        return xiiInputSlot_KeyF2;
      case SDLK_F3:
        return xiiInputSlot_KeyF3;
      case SDLK_F4:
        return xiiInputSlot_KeyF4;
      case SDLK_F5:
        return xiiInputSlot_KeyF5;
      case SDLK_F6:
        return xiiInputSlot_KeyF6;
      case SDLK_F7:
        return xiiInputSlot_KeyF7;
      case SDLK_F8:
        return xiiInputSlot_KeyF8;
      case SDLK_F9:
        return xiiInputSlot_KeyF9;
      case SDLK_F10:
        return xiiInputSlot_KeyF10;
      case SDLK_F11:
        return xiiInputSlot_KeyF11;
      case SDLK_F12:
        return xiiInputSlot_KeyF12;
      case SDLK_HOME:
        return xiiInputSlot_KeyHome;
      case SDLK_END:
        return xiiInputSlot_KeyEnd;
      case SDLK_DELETE:
        return xiiInputSlot_KeyDelete;
      case SDLK_INSERT:
        return xiiInputSlot_KeyInsert;
      case SDLK_PAGEUP:
        return xiiInputSlot_KeyPageUp;
      case SDLK_PAGEDOWN:
        return xiiInputSlot_KeyPageDown;
      case SDLK_NUMLOCKCLEAR:
        return xiiInputSlot_KeyNumLock;
      case SDLK_KP_PLUS:
        return xiiInputSlot_KeyNumpadPlus;
      case SDLK_KP_MINUS:
        return xiiInputSlot_KeyNumpadMinus;
      case SDLK_KP_MULTIPLY:
        return xiiInputSlot_KeyNumpadStar;
      case SDLK_KP_DIVIDE:
        return xiiInputSlot_KeyNumpadSlash;
      case SDLK_KP_DECIMAL:
        return xiiInputSlot_KeyNumpadPeriod;
      case SDLK_KP_ENTER:
        return xiiInputSlot_KeyNumpadEnter;
      case SDLK_CAPSLOCK:
        return xiiInputSlot_KeyCapsLock;
      case SDLK_PRINTSCREEN:
        return xiiInputSlot_KeyPrint;
      case SDLK_SCROLLLOCK:
        return xiiInputSlot_KeyScroll;
      case SDLK_PAUSE:
        return xiiInputSlot_KeyPause;
      case SDLK_MEDIA_PREVIOUS_TRACK:
        return xiiInputSlot_KeyPrevTrack;
      case SDLK_MEDIA_NEXT_TRACK:
        return xiiInputSlot_KeyNextTrack;
      case SDLK_MEDIA_PLAY_PAUSE:
        return xiiInputSlot_KeyPlayPause;
      case SDLK_MEDIA_STOP:
        return xiiInputSlot_KeyStop;
      case SDLK_VOLUMEUP:
        return xiiInputSlot_KeyVolumeUp;
      case SDLK_VOLUMEDOWN:
        return xiiInputSlot_KeyVolumeDown;
      case SDLK_MUTE:
        return xiiInputSlot_KeyMute;
      default:
        return nullptr;
    }
  }
} // namespace

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber, SDL_Window* windowHandle) :
  m_uiWindowNumber(uiWindowNumber), m_pWindow(windowHandle)
{
  m_uiWindowNumber = uiWindowNumber;

  if (uiWindowNumber == 0)
  {
    XII_ASSERT_RELEASE(!s_bMainWindowUsed, "You cannot have two devices of Type xiiStandardInputDevice with the window number zero.");
    xiiStandardInputDevice::s_bMainWindowUsed = true;
  }

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
  m_DoubleClickTime = xiiTime::MakeFromMilliseconds(GetDoubleClickTime());
#  endif

  SDL_StartTextInput(m_pWindow);
}

xiiStandardInputDevice::~xiiStandardInputDevice()
{
  SetShowMouseCursor(true);

  if (m_uiWindowNumber == 0)
  {
    xiiStandardInputDevice::s_bMainWindowUsed = false;
  }

  SDL_StopTextInput(m_pWindow);
}

void xiiStandardInputDevice::WindowMessage(void* pMessage)
{
  SDL_Event& event = *static_cast<SDL_Event*>(pMessage);

  switch (event.type)
  {
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    {
      OnFocusLost();
    }
    break;
    case SDL_EVENT_MOUSE_WHEEL:
    {
      float fMouseWheelDelta = event.wheel.y;

      if (fMouseWheelDelta > 0.0f)
      {
        m_InputSlotValues[xiiInputSlot_MouseWheelUp] = fMouseWheelDelta;
      }
      else
      {
        m_InputSlotValues[xiiInputSlot_MouseWheelDown] = fMouseWheelDelta;
      }
    }
    break;
    case SDL_EVENT_MOUSE_MOTION:
    {
      s_iMouseIsOverWindowNumber = m_uiWindowNumber;

      const float fPosX    = static_cast<float>(event.motion.x);
      const float fPosY    = static_cast<float>(event.motion.y);
      const float fRelPosX = static_cast<float>(event.motion.xrel);
      const float fRelPosY = static_cast<float>(event.motion.yrel);

      xiiInt32 iWindowWidth;
      xiiInt32 iWindowHeight;
      SDL_GetWindowSize(m_pWindow, &iWindowWidth, &iWindowHeight);

      m_InputSlotValues[xiiInputSlot_MousePositionX] = static_cast<float>(fPosX / iWindowWidth);
      m_InputSlotValues[xiiInputSlot_MousePositionY] = static_cast<float>(fPosY / iWindowHeight);

      m_InputSlotValues[xiiInputSlot_MouseMoveNegX] += ((fRelPosX < 0) ? -fRelPosX : 0.0f) * GetMouseSpeed().x;
      m_InputSlotValues[xiiInputSlot_MouseMovePosX] += ((fRelPosX > 0) ? fRelPosX : 0.0f) * GetMouseSpeed().x;
      m_InputSlotValues[xiiInputSlot_MouseMoveNegY] += ((fRelPosY < 0) ? -fRelPosY : 0.0f) * GetMouseSpeed().y;
      m_InputSlotValues[xiiInputSlot_MouseMovePosY] += ((fRelPosY > 0) ? fRelPosY : 0.0f) * GetMouseSpeed().y;
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
      const char* szInputSlot         = nullptr;
      const char* szDblClickInputSlot = nullptr;

      switch (event.button.button)
      {
        case SDL_BUTTON_LEFT:
          szInputSlot         = xiiInputSlot_MouseButton0;
          szDblClickInputSlot = xiiInputSlot_MouseDblClick0;
          break;
        case SDL_BUTTON_RIGHT:
          szInputSlot         = xiiInputSlot_MouseButton1;
          szDblClickInputSlot = xiiInputSlot_MouseDblClick1;
          break;
        case SDL_BUTTON_MIDDLE:
          szInputSlot         = xiiInputSlot_MouseButton2;
          szDblClickInputSlot = xiiInputSlot_MouseDblClick2;
          break;
        case SDL_BUTTON_X1:
          szInputSlot = xiiInputSlot_MouseButton3;
          break;
        case SDL_BUTTON_X2:
          szInputSlot = xiiInputSlot_MouseButton4;
          break;
      }

      if (szInputSlot)
      {
        m_InputSlotValues[szInputSlot] = 1.0f;
      }
      if (szDblClickInputSlot && event.button.clicks > 1)
      {
        m_InputSlotValues[szInputSlot] = 1.0f;
      }
    }
    break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
      const char* szInputSlot         = nullptr;
      const char* szDblClickInputSlot = nullptr;

      switch (event.button.button)
      {
        case SDL_BUTTON_LEFT:
          szInputSlot         = xiiInputSlot_MouseButton0;
          szDblClickInputSlot = xiiInputSlot_MouseDblClick0;
          break;
        case SDL_BUTTON_RIGHT:
          szInputSlot         = xiiInputSlot_MouseButton1;
          szDblClickInputSlot = xiiInputSlot_MouseDblClick1;
          break;
        case SDL_BUTTON_MIDDLE:
          szInputSlot         = xiiInputSlot_MouseButton2;
          szDblClickInputSlot = xiiInputSlot_MouseDblClick2;
          break;
        case SDL_BUTTON_X1:
          szInputSlot = xiiInputSlot_MouseButton3;
          break;
        case SDL_BUTTON_X2:
          szInputSlot = xiiInputSlot_MouseButton4;
          break;

        default:
          break;
      }

      if (szInputSlot)
      {
        m_InputSlotValues[szInputSlot] = 0.0f;
      }
      if (szDblClickInputSlot && event.button.clicks > 1)
      {
        m_InputSlotValues[szInputSlot] = 1.0f;
      }
    }
    break;
    case SDL_EVENT_TEXT_INPUT:
    {
      m_uiLastCharacter = event.text.text[0];
    }
    break;
    case SDL_EVENT_KEY_DOWN:
    {
      switch (event.key.key)
      {
        case SDLK_BACKSPACE:
          m_uiLastCharacter = 0x00000008;
          break;
        case SDLK_RETURN:
          m_uiLastCharacter = 0xD;
          break;
        case SDLK_ESCAPE:
          m_uiLastCharacter = 0x1B;
          break;
        case SDLK_TAB:
          m_uiLastCharacter = 0x00000009;
          break;

        default:
          break;
      }

      const char* szInputSlotName = ConvertSDLKeyToEngineName(event.key.key);

      if (szInputSlotName)
      {
        m_InputSlotValues[szInputSlotName] = 1.0f;
      }
      else
      {
        xiiLog::Warning("Unhandled SDL key code {} pressed.", event.key.key);
      }
    }
    break;
    case SDL_EVENT_KEY_UP:
    {
      const char* szInputSlotName = ConvertSDLKeyToEngineName(event.key.key);

      if (szInputSlotName)
      {
        m_InputSlotValues[szInputSlotName] = 0.0f;
      }
      else
      {
        xiiLog::Warning("Unhandled SDL key code {} released.", event.key.key);
      }
    }
    break;
  }
}

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (bShow)
  {
    if (!SDL_ShowCursor())
    {
      xiiLog::Error("SDL failed to show mouse cursor with error '{}'.", SDL_GetError());
    }
  }
  else
  {
    if (!SDL_HideCursor())
    {
      xiiLog::Error("SDL failed to hide mouse cursor with error '{}'.", SDL_GetError());
    }
  }
}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return SDL_CursorVisible();
}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode)
{
  if (m_ClipCursorMode == mode)
    return;

  m_ClipCursorMode = mode;

  switch (mode)
  {
    case xiiMouseCursorClipMode::NoClip:
    {
      SDL_SetWindowMouseGrab(m_pWindow, false);
    }
    break;
    case xiiMouseCursorClipMode::ClipToWindow:
    case xiiMouseCursorClipMode::ClipToWindowImmediate:
    {
      SDL_SetWindowMouseGrab(m_pWindow, true);
    }
    break;
    case xiiMouseCursorClipMode::ClipToPosition:
    {
      float                fRelativeMouseX, fRelativeMouseY;
      SDL_MouseButtonFlags mouseButtonFlags = SDL_GetMouseState(&fRelativeMouseX, &fRelativeMouseY);

      XII_IGNORE_UNUSED(mouseButtonFlags);

      SDL_SetWindowMouseGrab(m_pWindow, true);
      SDL_WarpMouseInWindow(m_pWindow, fRelativeMouseX, fRelativeMouseY);
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

xiiMouseCursorClipMode::Enum xiiStandardInputDevice::GetClipMouseCursor() const
{
  return m_ClipCursorMode;
}

void xiiStandardInputDevice::InitializeDevice()
{
  if (m_uiWindowNumber != 0)
  {
    xiiLog::Info("Window {0} does not need to initialize Mouse or Keyboard.", m_uiWindowNumber);
  }
}

void xiiStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[xiiInputSlot_MouseWheelUp]   = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseWheelDown] = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegX]  = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseMovePosX]  = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegY]  = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseMovePosY]  = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseDblClick0] = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseDblClick1] = 0.0f;
  m_InputSlotValues[xiiInputSlot_MouseDblClick2] = 0.0f;
}

void xiiStandardInputDevice::UpdateInputSlotValues()
{
  SUPER::UpdateInputSlotValues();
}

void xiiStandardInputDevice::OnFocusLost()
{
  auto it = m_InputSlotValues.GetIterator();

  while (it.IsValid())
  {
    it.Value() = 0.0f;
    it.Next();
  }
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
}

#endif
