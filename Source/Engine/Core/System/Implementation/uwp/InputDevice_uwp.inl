#include <Core/Input/InputManager.h>
#include <Core/System/Implementation/uwp/InputDevice_uwp.h>
#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringConversion.h>
#include <wrl/event.h>

using namespace ABI::Windows::UI::Core;

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStandardInputDevice, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStandardInputDevice::xiiStandardInputDevice(ICoreWindow* coreWindow) :
  m_coreWindow(coreWindow)
{
  // TODO
  m_ClipCursorMode = xiiMouseCursorClipMode::NoClip;
  m_bShowCursor    = true;
}

xiiStandardInputDevice::~xiiStandardInputDevice()
{
  if (m_coreWindow)
  {
    m_coreWindow->remove_KeyDown(m_eventRegistration_keyDown);
    m_coreWindow->remove_KeyUp(m_eventRegistration_keyUp);
    m_coreWindow->remove_CharacterReceived(m_eventRegistration_characterReceived);
    m_coreWindow->remove_PointerMoved(m_eventRegistration_pointerMoved);
    m_coreWindow->remove_PointerEntered(m_eventRegistration_pointerEntered);
    m_coreWindow->remove_PointerExited(m_eventRegistration_pointerExited);
    m_coreWindow->remove_PointerCaptureLost(m_eventRegistration_pointerCaptureLost);
    m_coreWindow->remove_PointerPressed(m_eventRegistration_pointerPressed);
    m_coreWindow->remove_PointerReleased(m_eventRegistration_pointerReleased);
    m_coreWindow->remove_PointerWheelChanged(m_eventRegistration_pointerWheelChanged);
  }

  if (m_mouseDevice)
  {
    m_mouseDevice->remove_MouseMoved(m_eventRegistration_mouseMoved);
  }
}

void xiiStandardInputDevice::InitializeDevice()
{
  using KeyHandler               = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs;
  using CharacterReceivedHandler = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs;
  using PointerHander            = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs;

  // Keyboard
  m_coreWindow->add_KeyDown(Callback<KeyHandler>(this, &xiiStandardInputDevice::OnKeyEvent).Get(), &m_eventRegistration_keyDown);
  m_coreWindow->add_KeyUp(Callback<KeyHandler>(this, &xiiStandardInputDevice::OnKeyEvent).Get(), &m_eventRegistration_keyUp);
  m_coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &xiiStandardInputDevice::OnCharacterReceived).Get(), &m_eventRegistration_characterReceived);

  // Pointer
  // Note that a pointer may be mouse, pen/stylus or touch!
  // We bundle move/press/enter all in a single callback to update all pointer state - all these cases have in common that pen/touch is
  // pressed now.
  m_coreWindow->add_PointerMoved(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerMovePressEnter).Get(), &m_eventRegistration_pointerMoved);
  m_coreWindow->add_PointerEntered(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerMovePressEnter).Get(), &m_eventRegistration_pointerEntered);
  m_coreWindow->add_PointerPressed(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerMovePressEnter).Get(), &m_eventRegistration_pointerPressed);
  // Changes in the pointer wheel:
  m_coreWindow->add_PointerWheelChanged(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerWheelChange).Get(), &m_eventRegistration_pointerWheelChanged);
  // Exit for touch or stylus means that we no longer have a press.
  // However, we presserve mouse button presses.
  m_coreWindow->add_PointerExited(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerReleasedOrExited).Get(), &m_eventRegistration_pointerExited);
  m_coreWindow->add_PointerReleased(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerReleasedOrExited).Get(), &m_eventRegistration_pointerReleased);
  // Capture loss.
  // From documentation "Occurs when a pointer moves to another app. This event is raised after PointerExited and is the final event
  // received by the app for this pointer." If this happens we want to release all mouse buttons as well.
  m_coreWindow->add_PointerCaptureLost(Callback<PointerHander>(this, &xiiStandardInputDevice::OnPointerCaptureLost).Get(), &m_eventRegistration_pointerCaptureLost);

  // Mouse
  // The only thing that we get from the MouseDevice class is mouse moved which gives us unfiltered relative mouse position.
  // Everything else is done by WinRt's "Pointer"
  // https://docs.microsoft.com/uwp/api/windows.devices.input.mousedevice
  // Relevant article for mouse move:
  // https://docs.microsoft.com/windows/uwp/gaming/relative-mouse-movement
  {
    ComPtr<ABI::Windows::Devices::Input::IMouseDeviceStatics> mouseDeviceStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Devices_Input_MouseDevice).Get(), &mouseDeviceStatics)))
    {
      if (SUCCEEDED(mouseDeviceStatics->GetForCurrentView(&m_mouseDevice)))
      {
        using MouseMovedHandler = __FITypedEventHandler_2_Windows__CDevices__CInput__CMouseDevice_Windows__CDevices__CInput__CMouseEventArgs;

        m_mouseDevice->add_MouseMoved(Callback<MouseMovedHandler>(this, &xiiStandardInputDevice::OnMouseMoved).Get(), &m_eventRegistration_mouseMoved);
      }
    }
  }
}

HRESULT xiiStandardInputDevice::OnKeyEvent(ICoreWindow* coreWindow, IKeyEventArgs* args)
{
  // Closely related to the RawInput implementation in Win32/InputDevice_win32.inl

  CorePhysicalKeyStatus keyStatus;
  XII_SUCCEED_OR_RETURN(args->get_KeyStatus(&keyStatus));

  static bool bWasStupidLeftShift = false;

  if (keyStatus.ScanCode == 42 && keyStatus.IsExtendedKey) // 42 has to be special I guess
  {
    bWasStupidLeftShift = true;
    return S_OK;
  }

  xiiStringView sInputSlotName = xiiInputManager::ConvertScanCodeToEngineName(static_cast<xiiUInt8>(keyStatus.ScanCode), keyStatus.IsExtendedKey == TRUE);
  if (sInputSlotName.IsEmpty())
    return S_OK;


  // Don't know yet how to handle this in UWP:

  // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
  // so we need to fix this manually
  // if (raw->data.keyboard.Flags & RI_KEY_E1)
  // {
  //   sInputSlotName = xiiInputSlot_KeyPause;
  //   bIgnoreNext = true;
  // }


  // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
  // we ignore the first stupid shift key entirely and then modify the following Numpad* key
  // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
  // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
  if ((sInputSlotName == xiiInputSlot_KeyNumpadStar) && bWasStupidLeftShift)
    sInputSlotName = xiiInputSlot_KeyPrint;

  bWasStupidLeftShift = false;

  m_InputSlotValues[sInputSlotName] = keyStatus.IsKeyReleased ? 0.0f : 1.0f;

  return S_OK;
}

HRESULT xiiStandardInputDevice::OnCharacterReceived(ICoreWindow* coreWindow, ICharacterReceivedEventArgs* args)
{
  UINT32 keyCode = 0;
  XII_SUCCEED_OR_RETURN(args->get_KeyCode(&keyCode));
  m_uiLastCharacter = keyCode;

  return S_OK;
}

HRESULT xiiStandardInputDevice::OnPointerMovePressEnter(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  XII_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  XII_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));

  // Pointer position.
  // From the documentation: "The position of the pointer in device-independent pixel (DIP)."
  // Note also, that there is "raw position" which may be free of pointer prediction etc.
  ABI::Windows::Foundation::Point pointerPosition;
  XII_SUCCEED_OR_RETURN(pointerPoint->get_Position(&pointerPosition));
  ABI::Windows::Foundation::Rect windowRectangle;
  XII_SUCCEED_OR_RETURN(coreWindow->get_Bounds(&windowRectangle)); // Bounds are in DIP as well!

  float relativePosX = static_cast<float>(pointerPosition.X) / windowRectangle.Width;
  float relativePosY = static_cast<float>(pointerPosition.Y) / windowRectangle.Height;

  if (deviceType == PointerDeviceType_Mouse)
  {
    // TODO
    // RegisterInputSlot(xiiInputSlot_MouseDblClick0, "Left Double Click", xiiInputSlotFlags::IsDoubleClick);
    // RegisterInputSlot(xiiInputSlot_MouseDblClick1, "Right Double Click", xiiInputSlotFlags::IsDoubleClick);
    // RegisterInputSlot(xiiInputSlot_MouseDblClick2, "Middle Double Click", xiiInputSlotFlags::IsDoubleClick);

    s_iMouseIsOverWindowNumber                     = 0;
    m_InputSlotValues[xiiInputSlot_MousePositionX] = relativePosX;
    m_InputSlotValues[xiiInputSlot_MousePositionY] = relativePosY;

    XII_SUCCEED_OR_RETURN(UpdateMouseButtonStates(pointerPoint.Get()));
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    // All callbacks we subscribed this event to imply that a touch occurs right now.
    m_InputSlotValues[xiiInputManager::GetInputSlotTouchPoint(pointerId)]          = 1.0f; // Touch strength?
    m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionX(pointerId)] = relativePosX;
    m_InputSlotValues[xiiInputManager::GetInputSlotTouchPointPositionY(pointerId)] = relativePosY;
  }

  return S_OK;
}

HRESULT xiiStandardInputDevice::OnPointerWheelChange(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  XII_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));

  // Only interested in mouse devices.
  PointerDeviceType deviceType;
  XII_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));
  if (deviceType == PointerDeviceType_Mouse)
  {
    ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> properties;
    XII_SUCCEED_OR_RETURN(pointerPoint->get_Properties(&properties));

    // .. and only vertical wheels.
    boolean isHorizontalWheel;
    XII_SUCCEED_OR_RETURN(properties->get_IsHorizontalMouseWheel(&isHorizontalWheel));
    if (!isHorizontalWheel)
    {
      INT32 delta;
      XII_SUCCEED_OR_RETURN(properties->get_MouseWheelDelta(&delta));

      if (delta > 0)
        m_InputSlotValues[xiiInputSlot_MouseWheelUp] = delta / 120.0f;
      else
        m_InputSlotValues[xiiInputSlot_MouseWheelDown] = -delta / 120.0f;
    }
  }

  return S_OK;
}

HRESULT xiiStandardInputDevice::OnPointerReleasedOrExited(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  XII_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  XII_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));

  if (deviceType == PointerDeviceType_Mouse)
  {
    // Note that the relased event is only fired if the last mouse button is released according to documentation.
    // However, we're also subscribing to exit and depending on the mouse capture this may or may not be a button release.
    XII_SUCCEED_OR_RETURN(UpdateMouseButtonStates(pointerPoint.Get()));
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    m_InputSlotValues[xiiInputManager::GetInputSlotTouchPoint(pointerId)] = 0.0f;
  }

  return S_OK;
}

HRESULT xiiStandardInputDevice::OnPointerCaptureLost(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  XII_SUCCEED_OR_RETURN(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  XII_SUCCEED_OR_RETURN(pointerDevice->get_PointerDeviceType(&deviceType));

  if (deviceType == PointerDeviceType_Mouse)
  {
    m_InputSlotValues[xiiInputSlot_MouseButton0] = 0.0f;
    m_InputSlotValues[xiiInputSlot_MouseButton1] = 0.0f;
    m_InputSlotValues[xiiInputSlot_MouseButton2] = 0.0f;
    m_InputSlotValues[xiiInputSlot_MouseButton3] = 0.0f;
    m_InputSlotValues[xiiInputSlot_MouseButton4] = 0.0f;
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    XII_SUCCEED_OR_RETURN(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    m_InputSlotValues[xiiInputManager::GetInputSlotTouchPoint(pointerId)] = 0.0f;
  }

  return S_OK;
}

HRESULT xiiStandardInputDevice::OnMouseMoved(ABI::Windows::Devices::Input::IMouseDevice* mouseDevice, ABI::Windows::Devices::Input::IMouseEventArgs* args)
{
  ABI::Windows::Devices::Input::MouseDelta mouseDelta;
  XII_SUCCEED_OR_RETURN(args->get_MouseDelta(&mouseDelta));

  m_InputSlotValues[xiiInputSlot_MouseMoveNegX] += ((mouseDelta.X < 0) ? static_cast<float>(-mouseDelta.X) : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[xiiInputSlot_MouseMovePosX] += ((mouseDelta.X > 0) ? static_cast<float>(mouseDelta.X) : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[xiiInputSlot_MouseMoveNegY] += ((mouseDelta.Y < 0) ? static_cast<float>(-mouseDelta.Y) : 0.0f) * GetMouseSpeed().y;
  m_InputSlotValues[xiiInputSlot_MouseMovePosY] += ((mouseDelta.Y > 0) ? static_cast<float>(mouseDelta.Y) : 0.0f) * GetMouseSpeed().y;

  return S_OK;
}

HRESULT xiiStandardInputDevice::UpdateMouseButtonStates(ABI::Windows::UI::Input::IPointerPoint* pointerPoint)
{
  ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> properties;
  XII_SUCCEED_OR_RETURN(pointerPoint->get_Properties(&properties));

  boolean isPressed;
  XII_SUCCEED_OR_RETURN(properties->get_IsLeftButtonPressed(&isPressed));
  m_InputSlotValues[xiiInputSlot_MouseButton0] = isPressed ? 1.0f : 0.0f;
  XII_SUCCEED_OR_RETURN(properties->get_IsRightButtonPressed(&isPressed));
  m_InputSlotValues[xiiInputSlot_MouseButton1] = isPressed ? 1.0f : 0.0f;
  XII_SUCCEED_OR_RETURN(properties->get_IsMiddleButtonPressed(&isPressed));
  m_InputSlotValues[xiiInputSlot_MouseButton2] = isPressed ? 1.0f : 0.0f;
  XII_SUCCEED_OR_RETURN(properties->get_IsXButton1Pressed(&isPressed));
  m_InputSlotValues[xiiInputSlot_MouseButton3] = isPressed ? 1.0f : 0.0f;
  XII_SUCCEED_OR_RETURN(properties->get_IsXButton2Pressed(&isPressed));
  m_InputSlotValues[xiiInputSlot_MouseButton4] = isPressed ? 1.0f : 0.0f;

  return S_OK;
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

  // RegisterInputSlot(xiiInputSlot_KeyPrevTrack, "Previous Track", xiiInputSlotFlags::IsButton);
  // RegisterInputSlot(xiiInputSlot_KeyNextTrack, "Next Track", xiiInputSlotFlags::IsButton);
  // RegisterInputSlot(xiiInputSlot_KeyPlayPause, "Play / Pause", xiiInputSlotFlags::IsButton);
  // RegisterInputSlot(xiiInputSlot_KeyStop, "Stop", xiiInputSlotFlags::IsButton);
  // RegisterInputSlot(xiiInputSlot_KeyVolumeUp, "Volume Up", xiiInputSlotFlags::IsButton);
  // RegisterInputSlot(xiiInputSlot_KeyVolumeDown, "Volume Down", xiiInputSlotFlags::IsButton);
  // RegisterInputSlot(xiiInputSlot_KeyMute, "Mute", xiiInputSlotFlags::IsButton);

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


  // Not yet supported
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

void SetClipRect(bool bClip, HWND hWnd)
{
  // NOT IMPLEMENTED. TODO
}

void xiiStandardInputDevice::SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode)
{
  if (m_ClipCursorMode == mode)
    return;

  if (mode != xiiMouseCursorClipMode::NoClip)
    m_coreWindow->SetPointerCapture();
  else
    m_coreWindow->ReleasePointerCapture();

  m_ClipCursorMode = mode;
}

void xiiStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  // Hide
  if (!bShow)
  {
    // Save cursor to reinstantiate it.
    m_coreWindow->get_PointerCursor(&m_cursorBeforeHide);
    m_coreWindow->put_PointerCursor(nullptr);
  }

  // Show
  else
  {
    XII_ASSERT_DEV(m_cursorBeforeHide, "There should be a ICoreCursor backup that can be put back.");
    m_coreWindow->put_PointerCursor(m_cursorBeforeHide.Get());
  }

  m_bShowCursor = bShow;
}

bool xiiStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}
