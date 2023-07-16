#include <Core/Input/InputManager.h>
#include <Core/System/Implementation/Android/InputDevice_android.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringConversion.h>

#include <android_native_app_glue.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool xiiStandardInputDevice::s_bMainWindowUsed = false;

namespace
{
  const char* ConvertAndroidKeyCodeToEngineName(xiiInt32 iKeyCode)
  {
    switch (iKeyCode)
    {
      case AKEYCODE_ESCAPE:
        return xiiInputSlot_KeyEscape;
      case AKEYCODE_1:
        return xiiInputSlot_Key1;
      case AKEYCODE_2:
        return xiiInputSlot_Key2;
      case AKEYCODE_3:
        return xiiInputSlot_Key3;
      case AKEYCODE_4:
        return xiiInputSlot_Key4;
      case AKEYCODE_5:
        return xiiInputSlot_Key5;
      case AKEYCODE_6:
        return xiiInputSlot_Key6;
      case AKEYCODE_7:
        return xiiInputSlot_Key7;
      case AKEYCODE_8:
        return xiiInputSlot_Key8;
      case AKEYCODE_9:
        return xiiInputSlot_Key9;
      case AKEYCODE_0:
        return xiiInputSlot_Key0;

      case AKEYCODE_MINUS:
        return xiiInputSlot_KeyHyphen;
      case AKEYCODE_EQUALS:
        return xiiInputSlot_KeyEquals;
      case AKEYCODE_DEL:
        return xiiInputSlot_KeyBackspace;

      case AKEYCODE_TAB:
        return xiiInputSlot_KeyTab;
      case AKEYCODE_Q:
        return xiiInputSlot_KeyQ;
      case AKEYCODE_W:
        return xiiInputSlot_KeyW;
      case AKEYCODE_E:
        return xiiInputSlot_KeyE;
      case AKEYCODE_R:
        return xiiInputSlot_KeyR;
      case AKEYCODE_T:
        return xiiInputSlot_KeyT;
      case AKEYCODE_Y:
        return xiiInputSlot_KeyY;
      case AKEYCODE_U:
        return xiiInputSlot_KeyU;
      case AKEYCODE_I:
        return xiiInputSlot_KeyI;
      case AKEYCODE_O:
        return xiiInputSlot_KeyO;
      case AKEYCODE_P:
        return xiiInputSlot_KeyP;
      case AKEYCODE_LEFT_BRACKET:
        return xiiInputSlot_KeyBracketClose;
      case AKEYCODE_RIGHT_BRACKET:
        return xiiInputSlot_KeyBracketClose;
      case AKEYCODE_ENTER:
        return xiiInputSlot_KeyReturn;

      case AKEYCODE_CTRL_LEFT:
        return xiiInputSlot_KeyLeftCtrl;
      case AKEYCODE_A:
        return xiiInputSlot_KeyA;
      case AKEYCODE_S:
        return xiiInputSlot_KeyS;
      case AKEYCODE_D:
        return xiiInputSlot_KeyD;
      case AKEYCODE_F:
        return xiiInputSlot_KeyF;
      case AKEYCODE_G:
        return xiiInputSlot_KeyG;
      case AKEYCODE_H:
        return xiiInputSlot_KeyH;
      case AKEYCODE_J:
        return xiiInputSlot_KeyJ;
      case AKEYCODE_K:
        return xiiInputSlot_KeyK;
      case AKEYCODE_L:
        return xiiInputSlot_KeyL;
      case AKEYCODE_SEMICOLON:
        return xiiInputSlot_KeySemicolon;
      case AKEYCODE_APOSTROPHE:
        return xiiInputSlot_KeyApostrophe;

      case AKEYCODE_GRAVE:
        return xiiInputSlot_KeyTilde;
      case AKEYCODE_SHIFT_LEFT:
        return xiiInputSlot_KeyLeftShift;
      case AKEYCODE_BACKSLASH:
        return xiiInputSlot_KeyBackslash;

      case AKEYCODE_Z:
        return xiiInputSlot_KeyZ;
      case AKEYCODE_X:
        return xiiInputSlot_KeyX;
      case AKEYCODE_C:
        return xiiInputSlot_KeyC;
      case AKEYCODE_V:
        return xiiInputSlot_KeyV;
      case AKEYCODE_B:
        return xiiInputSlot_KeyB;
      case AKEYCODE_N:
        return xiiInputSlot_KeyN;
      case AKEYCODE_M:
        return xiiInputSlot_KeyM;
      case AKEYCODE_COMMA:
        return xiiInputSlot_KeyComma;
      case AKEYCODE_PERIOD:
        return xiiInputSlot_KeyPeriod;
      case AKEYCODE_SLASH:
        return xiiInputSlot_KeySlash;
      case AKEYCODE_SHIFT_RIGHT:
        return xiiInputSlot_KeyRightShift;

      case AKEYCODE_NUMPAD_MULTIPLY:
        return xiiInputSlot_KeyNumpadStar;

      case AKEYCODE_ALT_LEFT:
        return xiiInputSlot_KeyLeftAlt;
      case AKEYCODE_SPACE:
        return xiiInputSlot_KeySpace;
      case AKEYCODE_CAPS_LOCK:
        return xiiInputSlot_KeyCapsLock;

      case AKEYCODE_F1:
        return xiiInputSlot_KeyF1;
      case AKEYCODE_F2:
        return xiiInputSlot_KeyF2;
      case AKEYCODE_F3:
        return xiiInputSlot_KeyF3;
      case AKEYCODE_F4:
        return xiiInputSlot_KeyF4;
      case AKEYCODE_F5:
        return xiiInputSlot_KeyF5;
      case AKEYCODE_F6:
        return xiiInputSlot_KeyF6;
      case AKEYCODE_F7:
        return xiiInputSlot_KeyF7;
      case AKEYCODE_F8:
        return xiiInputSlot_KeyF8;
      case AKEYCODE_F9:
        return xiiInputSlot_KeyF9;
      case AKEYCODE_F10:
        return xiiInputSlot_KeyF10;
      case AKEYCODE_F11:
        return xiiInputSlot_KeyF11;
      case AKEYCODE_F12:
        return xiiInputSlot_KeyF12;

      case AKEYCODE_NUM_LOCK:
        return xiiInputSlot_KeyNumLock;
      case AKEYCODE_SCROLL_LOCK:
        return xiiInputSlot_KeyScroll;

      case AKEYCODE_NUMPAD_7:
        return xiiInputSlot_KeyNumpad7;
      case AKEYCODE_NUMPAD_8:
        return xiiInputSlot_KeyNumpad8;
      case AKEYCODE_NUMPAD_9:
        return xiiInputSlot_KeyNumpad9;
      case AKEYCODE_NUMPAD_SUBTRACT:
        return xiiInputSlot_KeyNumpadMinus;

      case AKEYCODE_NUMPAD_4:
        return xiiInputSlot_KeyNumpad4;
      case AKEYCODE_NUMPAD_5:
        return xiiInputSlot_KeyNumpad5;
      case AKEYCODE_NUMPAD_6:
        return xiiInputSlot_KeyNumpad6;
      case AKEYCODE_NUMPAD_ADD:
        return xiiInputSlot_KeyNumpadPlus;

      case AKEYCODE_NUMPAD_1:
        return xiiInputSlot_KeyNumpad1;
      case AKEYCODE_NUMPAD_2:
        return xiiInputSlot_KeyNumpad2;
      case AKEYCODE_NUMPAD_3:
        return xiiInputSlot_KeyNumpad3;
      case AKEYCODE_NUMPAD_0:
        return xiiInputSlot_KeyNumpad0;
      case AKEYCODE_NUMPAD_DOT:
        return xiiInputSlot_KeyNumpadPeriod;

      // case AKEYCODE_PIPE: // Meta state shift
      //   return xiiInputSlot_KeyPipe;
      //
      // case AKEYCODE_WIN_LEFT:
      //   return xiiInputSlot_KeyLeftWin;
      // case AKEYCODE_WIN_RIGHT:
      //   return xiiInputSlot_KeyRightWin;
      case AKEYCODE_ALL_APPS:
        return xiiInputSlot_KeyApps;


      case AKEYCODE_NUMPAD_ENTER:
        return xiiInputSlot_KeyNumpadEnter;
      case AKEYCODE_CTRL_RIGHT:
        return xiiInputSlot_KeyRightCtrl;
      case AKEYCODE_NUMPAD_DIVIDE:
        return xiiInputSlot_KeyNumpadSlash;
      // case AKEYCODE_PRINT:
      //   return xiiInputSlot_KeyPrint;
      case AKEYCODE_ALT_RIGHT:
        return xiiInputSlot_KeyRightAlt;
      case AKEYCODE_MEDIA_PAUSE:
        return xiiInputSlot_KeyPause;
      case AKEYCODE_MOVE_HOME:
        return xiiInputSlot_KeyHome;
      case AKEYCODE_DPAD_UP:
        return xiiInputSlot_KeyUp;
      case AKEYCODE_PAGE_UP:
        return xiiInputSlot_KeyPageUp;

      case AKEYCODE_DPAD_LEFT:
        return xiiInputSlot_KeyLeft;
      case AKEYCODE_DPAD_RIGHT:
        return xiiInputSlot_KeyRight;

      case AKEYCODE_MOVE_END:
        return xiiInputSlot_KeyEnd;
      case AKEYCODE_DPAD_DOWN:
        return xiiInputSlot_KeyDown;
      case AKEYCODE_PAGE_DOWN:
        return xiiInputSlot_KeyPageDown;
      case AKEYCODE_INSERT:
        return xiiInputSlot_KeyInsert;
      case AKEYCODE_FORWARD_DEL:
        return xiiInputSlot_KeyDelete;

      default:
        return nullptr;
    }
  }
} // namespace

xiiStandardInputDevice::xiiStandardInputDevice(xiiUInt32 uiWindowNumber)
{
  m_uiWindowNumber = uiWindowNumber;

  if (uiWindowNumber != 0)
  {
    XII_ASSERT_RELEASE(!s_bMainWindowUsed, "You cannot have two devices of Type xiiStandardInputDevice with the window number zero.");
    xiiStandardInputDevice::s_bMainWindowUsed = true;
  }
}

xiiStandardInputDevice::~xiiStandardInputDevice()
{
  if (m_uiWindowNumber == 0)
    xiiStandardInputDevice::s_bMainWindowUsed = false;
}

void xiiStandardInputDevice::InitializeDevice()
{
  if (m_uiWindowNumber == 0)
  {
    xiiLog::Success("Initialized RawInput for Mouse and Keyboard input.");
  }
  else
  {
    xiiLog::Info("Window {0} does not need to initialize Mouse or Keyboard.", m_uiWindowNumber);
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

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode)
{
  // Unsupported
}

void xiiStandardInputDevice::WindowMessage(android_app* pAndroidApp, xiiInt32 iCommand)
{
  switch (iCommand)
  {
    case APP_CMD_LOST_FOCUS:
    {
      OnFocusLost();
    }
    break;

    default:
      break;
  }
}

void xiiStandardInputDevice::InputEventMessage(android_app* pAndroidApp, AInputEvent* pInputEvent)
{
  xiiInt32 iEventType = AInputEvent_getType(pInputEvent);
  switch (iEventType)
  {
    case AINPUT_EVENT_TYPE_KEY:
    {
      const xiiInt32 iKeyCode = AKeyEvent_getKeyCode(pInputEvent);

      switch (iKeyCode)
      {
        case AKEYCODE_DEL:
          m_uiLastCharacter = 0x8;
          break;
        case AKEYCODE_ENTER:
          m_uiLastCharacter = 0xD;
          break;
        case AKEYCODE_ESCAPE:
          m_uiLastCharacter = 0x1B;
          break;
        case AKEYCODE_TAB:
          m_uiLastCharacter = 0x9;
          break;
      }

      const char*    szInputSlotName = ConvertAndroidKeyCodeToEngineName(iKeyCode);
      const xiiInt32 iKeyAction      = AKeyEvent_getAction(pInputEvent);

      if (szInputSlotName)
      {
        switch (iKeyAction)
        {
          case AKEY_EVENT_ACTION_DOWN:
          {
            xiiInt32 iActionMetaState = AKeyEvent_getMetaState(pInputEvent);

            if (iActionMetaState == AMETA_SHIFT_ON && iKeyCode == AKEYCODE_BACKSLASH)
            {
              m_InputSlotValues[xiiInputSlot_KeyPipe] = 1.0f;
            }
            else
            {
              m_InputSlotValues[szInputSlotName] = 1.0f;
            }
          }
          break;
          case AKEY_EVENT_ACTION_UP:
          {
            m_InputSlotValues[szInputSlotName] = 0.0f;
          }
          break;
          case AKEY_EVENT_ACTION_MULTIPLE:
          {
            // Multiple duplicate key events in a row
            const xiiInt32 iRepeatCount = AKeyEvent_getRepeatCount(pInputEvent);

            if (iRepeatCount % 2 == 0)
            {
              m_InputSlotValues[szInputSlotName] = 1.0f;
            }
          }
          break;
        }
      }
      else
      {
        xiiLog::Warning("Unhandeled keyboard key {}.", iKeyCode);
      }

      // Complex string is being delivered.
      if (iKeyAction == AKEY_EVENT_ACTION_MULTIPLE && iKeyCode == AKEYCODE_UNKNOWN)
      {
        // \todo Core: Handle AKEY_EVENT_ACTION_MULTIPLE on Android complex string keycode.
      }

      // \todo Core: Handle retrieval of the last unicode character.
    }
    break;
    case AINPUT_EVENT_TYPE_MOTION:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_FOCUS:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_CAPTURE:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_DRAG:
    {
    }
    break;
    case AINPUT_EVENT_TYPE_TOUCH_MODE:
    {
    }
    break;

    default:
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      xiiLog::Warning("Unhandeled Application input event {0}", iEventType);
#endif
      break;
  }
}

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  // Unsupported
  XII_IGNORE_UNUSED(bShow);
}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return false;
}

void xiiStandardInputDevice::OnFocusLost()
{
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
