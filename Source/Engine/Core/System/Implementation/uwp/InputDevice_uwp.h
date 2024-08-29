#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

#include <windows.applicationmodel.core.h>
#include <wrl/client.h>

class XII_CORE_DLL xiiStandardInputDevice : public xiiInputDeviceMouseKeyboard
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStandardInputDevice, xiiInputDeviceMouseKeyboard);

public:
  xiiStandardInputDevice(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow);
  ~xiiStandardInputDevice();

  virtual void                         SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) override;
  virtual xiiMouseCursorClipMode::Enum GetClipMouseCursor() const override { return m_ClipCursorMode; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

private:
  HRESULT OnKeyEvent(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow, ABI::Windows::UI::Core::IKeyEventArgs* args);
  HRESULT OnCharacterReceived(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow, ABI::Windows::UI::Core::ICharacterReceivedEventArgs* args);
  HRESULT OnPointerMovePressEnter(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnPointerWheelChange(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnPointerReleasedOrExited(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnPointerCaptureLost(ABI::Windows::UI::Core::ICoreWindow* pCoreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnMouseMoved(ABI::Windows::Devices::Input::IMouseDevice* mouseDevice, ABI::Windows::Devices::Input::IMouseEventArgs* args);

  HRESULT UpdateMouseButtonStates(ABI::Windows::UI::Input::IPointerPoint* pointerPoint);

  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

  bool                         m_bShowCursor    = true;
  xiiMouseCursorClipMode::Enum m_ClipCursorMode = xiiMouseCursorClipMode::NoClip;


  Microsoft::WRL::ComPtr<ABI::Windows::UI::Core::ICoreWindow>        m_pCoreWindow;
  Microsoft::WRL::ComPtr<ABI::Windows::UI::Core::ICoreCursor>        m_cursorBeforeHide;
  Microsoft::WRL::ComPtr<ABI::Windows::Devices::Input::IMouseDevice> m_mouseDevice;

  EventRegistrationToken m_eventRegistration_keyDown;
  EventRegistrationToken m_eventRegistration_keyUp;
  EventRegistrationToken m_eventRegistration_characterReceived;
  EventRegistrationToken m_eventRegistration_pointerMoved;
  EventRegistrationToken m_eventRegistration_pointerEntered;
  EventRegistrationToken m_eventRegistration_pointerExited;
  EventRegistrationToken m_eventRegistration_pointerCaptureLost;
  EventRegistrationToken m_eventRegistration_pointerPressed;
  EventRegistrationToken m_eventRegistration_pointerReleased;
  EventRegistrationToken m_eventRegistration_pointerWheelChanged;
  EventRegistrationToken m_eventRegistration_mouseMoved;
};
