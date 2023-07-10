#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

extern "C"
{
  using android_app = struct android_app;
  using AInputEvent = struct AInputEvent;
}

class XII_CORE_DLL xiiStandardInputDevice : public xiiInputDeviceMouseKeyboard
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStandardInputDevice, xiiInputDeviceMouseKeyboard);

public:
  xiiStandardInputDevice(xiiUInt32 uiWindowNumber);
  ~xiiStandardInputDevice();

  void WindowMessage(android_app* pAndroidApp, xiiInt32 iCommand);
  void InputEventMessage(android_app* pAndroidApp, AInputEvent* pInputEvent);

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
  void OnFocusLost();

  static bool                  s_bMainWindowUsed;
  xiiUInt32                    m_uiWindowNumber               = 0;
  xiiMouseCursorClipMode::Enum m_ClipCursorMode               = xiiMouseCursorClipMode::ClipToWindow;
  bool                         m_bApplyClipRect               = false;
  xiiUInt8                     m_uiMouseButtonReceivedDown[5] = {0, 0, 0, 0, 0};
  xiiUInt8                     m_uiMouseButtonReceivedUp[5]   = {0, 0, 0, 0, 0};
};
