#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>

class XII_CORE_DLL xiiStandardInputDevice : public xiiInputDeviceMouseKeyboard
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStandardInputDevice, xiiInputDeviceMouseKeyboard);

public:
  xiiStandardInputDevice(xiiUInt32 uiWindowNumber);
  ~xiiStandardInputDevice();

  /// \brief This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(xiiMinWindows::HWND pWnd, xiiMinWindows::UINT msg, xiiMinWindows::WPARAM wparam, xiiMinWindows::LPARAM lparam);

  /// \brief Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from an English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

  virtual void                         SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) override;
  virtual xiiMouseCursorClipMode::Enum GetClipMouseCursor() const override { return m_ClipCursorMode; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

protected:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void ApplyClipRect(xiiMouseCursorClipMode::Enum mode, xiiMinWindows::HWND hWnd);
  void OnFocusLost(xiiMinWindows::HWND hWnd);

  static bool                  s_bMainWindowUsed;
  xiiUInt32                    m_uiWindowNumber               = 0;
  bool                         m_bShowCursor                  = true;
  xiiMouseCursorClipMode::Enum m_ClipCursorMode               = xiiMouseCursorClipMode::NoClip;
  bool                         m_bApplyClipRect               = false;
  xiiUInt8                     m_uiMouseButtonReceivedDown[5] = {0, 0, 0, 0, 0};
  xiiUInt8                     m_uiMouseButtonReceivedUp[5]   = {0, 0, 0, 0, 0};
};
