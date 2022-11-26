#include <Core/System/Implementation/glfw/InputDevice_glfw.h>
#include <GLFW/glfw3.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  const char* ConvertGLFWKeyToEngineName(int key)
  {
    switch (key)
    {
      case GLFW_KEY_LEFT:
        return xiiInputSlot_KeyLeft;
      case GLFW_KEY_RIGHT:
        return xiiInputSlot_KeyRight;
      case GLFW_KEY_UP:
        return xiiInputSlot_KeyUp;
      case GLFW_KEY_DOWN:
        return xiiInputSlot_KeyDown;
      case GLFW_KEY_ESCAPE:
        return xiiInputSlot_KeyEscape;
      case GLFW_KEY_SPACE:
        return xiiInputSlot_KeySpace;
      case GLFW_KEY_BACKSPACE:
        return xiiInputSlot_KeyBackspace;
      case GLFW_KEY_ENTER:
        return xiiInputSlot_KeyReturn;
      case GLFW_KEY_TAB:
        return xiiInputSlot_KeyTab;
      case GLFW_KEY_LEFT_SHIFT:
        return xiiInputSlot_KeyLeftShift;
      case GLFW_KEY_RIGHT_SHIFT:
        return xiiInputSlot_KeyRightShift;
      case GLFW_KEY_LEFT_CONTROL:
        return xiiInputSlot_KeyLeftCtrl;
      case GLFW_KEY_RIGHT_CONTROL:
        return xiiInputSlot_KeyRightCtrl;
      case GLFW_KEY_LEFT_ALT:
        return xiiInputSlot_KeyLeftAlt;
      case GLFW_KEY_RIGHT_ALT:
        return xiiInputSlot_KeyRightAlt;
      case GLFW_KEY_LEFT_SUPER:
        return xiiInputSlot_KeyLeftWin;
      case GLFW_KEY_RIGHT_SUPER:
        return xiiInputSlot_KeyRightWin;
      case GLFW_KEY_MENU:
        return xiiInputSlot_KeyApps;
      case GLFW_KEY_LEFT_BRACKET:
        return xiiInputSlot_KeyBracketOpen;
      case GLFW_KEY_RIGHT_BRACKET:
        return xiiInputSlot_KeyBracketClose;
      case GLFW_KEY_SEMICOLON:
        return xiiInputSlot_KeySemicolon;
      case GLFW_KEY_APOSTROPHE:
        return xiiInputSlot_KeyApostrophe;
      case GLFW_KEY_SLASH:
        return xiiInputSlot_KeySlash;
      case GLFW_KEY_EQUAL:
        return xiiInputSlot_KeyEquals;
      case GLFW_KEY_GRAVE_ACCENT:
        return xiiInputSlot_KeyTilde;
      case GLFW_KEY_MINUS:
        return xiiInputSlot_KeyHyphen;
      case GLFW_KEY_COMMA:
        return xiiInputSlot_KeyComma;
      case GLFW_KEY_PERIOD:
        return xiiInputSlot_KeyPeriod;
      case GLFW_KEY_BACKSLASH:
        return xiiInputSlot_KeyBackslash;
      case GLFW_KEY_WORLD_1:
        return xiiInputSlot_KeyPipe;
      case GLFW_KEY_1:
        return xiiInputSlot_Key1;
      case GLFW_KEY_2:
        return xiiInputSlot_Key2;
      case GLFW_KEY_3:
        return xiiInputSlot_Key3;
      case GLFW_KEY_4:
        return xiiInputSlot_Key4;
      case GLFW_KEY_5:
        return xiiInputSlot_Key5;
      case GLFW_KEY_6:
        return xiiInputSlot_Key6;
      case GLFW_KEY_7:
        return xiiInputSlot_Key7;
      case GLFW_KEY_8:
        return xiiInputSlot_Key8;
      case GLFW_KEY_9:
        return xiiInputSlot_Key9;
      case GLFW_KEY_0:
        return xiiInputSlot_Key0;
      case GLFW_KEY_KP_1:
        return xiiInputSlot_KeyNumpad1;
      case GLFW_KEY_KP_2:
        return xiiInputSlot_KeyNumpad2;
      case GLFW_KEY_KP_3:
        return xiiInputSlot_KeyNumpad3;
      case GLFW_KEY_KP_4:
        return xiiInputSlot_KeyNumpad4;
      case GLFW_KEY_KP_5:
        return xiiInputSlot_KeyNumpad5;
      case GLFW_KEY_KP_6:
        return xiiInputSlot_KeyNumpad6;
      case GLFW_KEY_KP_7:
        return xiiInputSlot_KeyNumpad7;
      case GLFW_KEY_KP_8:
        return xiiInputSlot_KeyNumpad8;
      case GLFW_KEY_KP_9:
        return xiiInputSlot_KeyNumpad9;
      case GLFW_KEY_KP_0:
        return xiiInputSlot_KeyNumpad0;
      case GLFW_KEY_A:
        return xiiInputSlot_KeyA;
      case GLFW_KEY_B:
        return xiiInputSlot_KeyB;
      case GLFW_KEY_C:
        return xiiInputSlot_KeyC;
      case GLFW_KEY_D:
        return xiiInputSlot_KeyD;
      case GLFW_KEY_E:
        return xiiInputSlot_KeyE;
      case GLFW_KEY_F:
        return xiiInputSlot_KeyF;
      case GLFW_KEY_G:
        return xiiInputSlot_KeyG;
      case GLFW_KEY_H:
        return xiiInputSlot_KeyH;
      case GLFW_KEY_I:
        return xiiInputSlot_KeyI;
      case GLFW_KEY_J:
        return xiiInputSlot_KeyJ;
      case GLFW_KEY_K:
        return xiiInputSlot_KeyK;
      case GLFW_KEY_L:
        return xiiInputSlot_KeyL;
      case GLFW_KEY_M:
        return xiiInputSlot_KeyM;
      case GLFW_KEY_N:
        return xiiInputSlot_KeyN;
      case GLFW_KEY_O:
        return xiiInputSlot_KeyO;
      case GLFW_KEY_P:
        return xiiInputSlot_KeyP;
      case GLFW_KEY_Q:
        return xiiInputSlot_KeyQ;
      case GLFW_KEY_R:
        return xiiInputSlot_KeyR;
      case GLFW_KEY_S:
        return xiiInputSlot_KeyS;
      case GLFW_KEY_T:
        return xiiInputSlot_KeyT;
      case GLFW_KEY_U:
        return xiiInputSlot_KeyU;
      case GLFW_KEY_V:
        return xiiInputSlot_KeyV;
      case GLFW_KEY_W:
        return xiiInputSlot_KeyW;
      case GLFW_KEY_X:
        return xiiInputSlot_KeyX;
      case GLFW_KEY_Y:
        return xiiInputSlot_KeyY;
      case GLFW_KEY_Z:
        return xiiInputSlot_KeyZ;
      case GLFW_KEY_F1:
        return xiiInputSlot_KeyF1;
      case GLFW_KEY_F2:
        return xiiInputSlot_KeyF2;
      case GLFW_KEY_F3:
        return xiiInputSlot_KeyF3;
      case GLFW_KEY_F4:
        return xiiInputSlot_KeyF4;
      case GLFW_KEY_F5:
        return xiiInputSlot_KeyF5;
      case GLFW_KEY_F6:
        return xiiInputSlot_KeyF6;
      case GLFW_KEY_F7:
        return xiiInputSlot_KeyF7;
      case GLFW_KEY_F8:
        return xiiInputSlot_KeyF8;
      case GLFW_KEY_F9:
        return xiiInputSlot_KeyF9;
      case GLFW_KEY_F10:
        return xiiInputSlot_KeyF10;
      case GLFW_KEY_F11:
        return xiiInputSlot_KeyF11;
      case GLFW_KEY_F12:
        return xiiInputSlot_KeyF12;
      case GLFW_KEY_HOME:
        return xiiInputSlot_KeyHome;
      case GLFW_KEY_END:
        return xiiInputSlot_KeyEnd;
      case GLFW_KEY_DELETE:
        return xiiInputSlot_KeyDelete;
      case GLFW_KEY_INSERT:
        return xiiInputSlot_KeyInsert;
      case GLFW_KEY_PAGE_UP:
        return xiiInputSlot_KeyPageUp;
      case GLFW_KEY_PAGE_DOWN:
        return xiiInputSlot_KeyPageDown;
      case GLFW_KEY_NUM_LOCK:
        return xiiInputSlot_KeyNumLock;
      case GLFW_KEY_KP_ADD:
        return xiiInputSlot_KeyNumpadPlus;
      case GLFW_KEY_KP_SUBTRACT:
        return xiiInputSlot_KeyNumpadMinus;
      case GLFW_KEY_KP_MULTIPLY:
        return xiiInputSlot_KeyNumpadStar;
      case GLFW_KEY_KP_DIVIDE:
        return xiiInputSlot_KeyNumpadSlash;
      case GLFW_KEY_KP_DECIMAL:
        return xiiInputSlot_KeyNumpadPeriod;
      case GLFW_KEY_KP_ENTER:
        return xiiInputSlot_KeyNumpadEnter;
      case GLFW_KEY_CAPS_LOCK:
        return xiiInputSlot_KeyCapsLock;
      case GLFW_KEY_PRINT_SCREEN:
        return xiiInputSlot_KeyPrint;
      case GLFW_KEY_SCROLL_LOCK:
        return xiiInputSlot_KeyScroll;
      case GLFW_KEY_PAUSE:
        return xiiInputSlot_KeyPause;
      // TODO xiiInputSlot_KeyPrevTrack
      // TODO xiiInputSlot_KeyNextTrack
      // TODO xiiInputSlot_KeyPlayPause
      // TODO xiiInputSlot_KeyStop
      // TODO xiiInputSlot_KeyVolumeUp
      // TODO xiiInputSlot_KeyVolumeDown
      // TODO xiiInputSlot_KeyMute
      default:
        return nullptr;
    }
  }
} // namespace

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber, GLFWwindow* windowHandle)
  : m_uiWindowNumber(uiWindowNumber)
  , m_pWindow(windowHandle)
{
}

xiiStandardInputDevice::~xiiStandardInputDevice()
{
}

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  glfwSetInputMode(m_pWindow, GLFW_CURSOR, bShow ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return (glfwGetInputMode(m_pWindow, GLFW_CURSOR) != GLFW_CURSOR_DISABLED);
}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode)
{
}

xiiMouseCursorClipMode::Enum xiiStandardInputDevice::GetClipMouseCursor() const
{
  return xiiMouseCursorClipMode::Default;
}

void xiiStandardInputDevice::InitializeDevice() {}

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
  // TODO RegisterInputSlot(xiiInputSlot_KeyTilde, "~", xiiInputSlotFlags::IsButton);
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

  /* TODO
  RegisterInputSlot(xiiInputSlot_KeyPrevTrack, "Previous Track", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyNextTrack, "Next Track", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyPlayPause, "Play / Pause", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyStop, "Stop", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyVolumeUp, "Volume Up", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyVolumeDown, "Volume Down", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_KeyMute, "Mute", xiiInputSlotFlags::IsButton);
  */

  RegisterInputSlot(xiiInputSlot_MousePositionX, "Mouse Position X", xiiInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(xiiInputSlot_MousePositionY, "Mouse Position Y", xiiInputSlotFlags::IsMouseAxisPosition);

  RegisterInputSlot(xiiInputSlot_MouseMoveNegX, "Mouse Move Left", xiiInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(xiiInputSlot_MouseMovePosX, "Mouse Move Right", xiiInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(xiiInputSlot_MouseMoveNegY, "Mouse Move Down", xiiInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(xiiInputSlot_MouseMovePosY, "Mouse Move Up", xiiInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(xiiInputSlot_MouseButton0, "Mousebutton 0", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton1, "Mousebutton 1", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton2, "Mousebutton 2", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton3, "Mousebutton 3", xiiInputSlotFlags::IsButton);
  RegisterInputSlot(xiiInputSlot_MouseButton4, "Mousebutton 4", xiiInputSlotFlags::IsButton);

  RegisterInputSlot(xiiInputSlot_MouseWheelUp, "Mousewheel Up", xiiInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(xiiInputSlot_MouseWheelDown, "Mousewheel Down", xiiInputSlotFlags::IsMouseWheel);
}

void xiiStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[xiiInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[xiiInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[xiiInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[xiiInputSlot_MouseMovePosY] = 0;
}

void xiiStandardInputDevice::OnKey(int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_uiLastCharacter = 0x00000008;
  }

  const char* szInputSlotName = ConvertGLFWKeyToEngineName(key);
  if (szInputSlotName)
  {
    m_InputSlotValues[szInputSlotName] = (action == GLFW_RELEASE) ? 0.0f : 1.0f;
  }
  else
  {
    xiiLog::Warning("Unhandeled glfw keyboard key {} {}", key, (action == GLFW_RELEASE) ? "released" : "pressed");
  }
}

void xiiStandardInputDevice::OnCharacter(unsigned int codepoint)
{
  m_uiLastCharacter = codepoint;
}

void xiiStandardInputDevice::OnCursorPosition(double xpos, double ypos)
{
  s_iMouseIsOverWindowNumber = m_uiWindowNumber;

  int width;
  int height;
  glfwGetWindowSize(m_pWindow, &width, &height);

  m_InputSlotValues[xiiInputSlot_MousePositionX] = static_cast<float>(xpos / width);
  m_InputSlotValues[xiiInputSlot_MousePositionY] = static_cast<float>(ypos / height);

  if (m_LastPos.x != xiiMath::MaxValue<double>())
  {
    xiiVec2d diff = xiiVec2d(xpos, ypos) - m_LastPos;

    m_InputSlotValues[xiiInputSlot_MouseMoveNegX] += ((diff.x < 0) ? (float)-diff.x : 0.0f) * GetMouseSpeed().x;
    m_InputSlotValues[xiiInputSlot_MouseMovePosX] += ((diff.x > 0) ? (float)diff.x : 0.0f) * GetMouseSpeed().x;
    m_InputSlotValues[xiiInputSlot_MouseMoveNegY] += ((diff.y < 0) ? (float)-diff.y : 0.0f) * GetMouseSpeed().y;
    m_InputSlotValues[xiiInputSlot_MouseMovePosY] += ((diff.y > 0) ? (float)diff.y : 0.0f) * GetMouseSpeed().y;
  }
  m_LastPos = xiiVec2d(xpos, ypos);
}

void xiiStandardInputDevice::OnMouseButton(int button, int action, int mods)
{
  const char* inputSlot = nullptr;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_1:
      inputSlot = xiiInputSlot_MouseButton0;
      break;
    case GLFW_MOUSE_BUTTON_2:
      inputSlot = xiiInputSlot_MouseButton1;
      break;
    case GLFW_MOUSE_BUTTON_3:
      inputSlot = xiiInputSlot_MouseButton2;
      break;
    case GLFW_MOUSE_BUTTON_4:
      inputSlot = xiiInputSlot_MouseButton3;
      break;
    case GLFW_MOUSE_BUTTON_5:
      inputSlot = xiiInputSlot_MouseButton4;
      break;
  }

  if (inputSlot)
  {
    m_InputSlotValues[inputSlot] = (action == GLFW_PRESS) ? 1.0f : 0.0f;
  }
}

void xiiStandardInputDevice::OnScroll(double xoffset, double yoffset)
{
  if (yoffset > 0)
  {
    m_InputSlotValues[xiiInputSlot_MouseWheelUp] = static_cast<float>(yoffset);
  }
  else
  {
    m_InputSlotValues[xiiInputSlot_MouseWheelDown] = static_cast<float>(-yoffset);
  }
}
