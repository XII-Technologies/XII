/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if XII_ENABLED(XII_SUPPORTS_SDL)

#  include <Core/Input/DeviceTypes/MouseKeyboard.h>

extern "C"
{
  using SDL_Window = struct SDL_Window;
}

class XII_CORE_DLL xiiStandardInputDevice : public xiiInputDeviceMouseKeyboard
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStandardInputDevice, xiiInputDeviceMouseKeyboard);

public:
  xiiStandardInputDevice(xiiUInt32 uiWindowNumber, SDL_Window* windowHandle);
  ~xiiStandardInputDevice() override;

  /// This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(void* pMessage);

  void                         SetShowMouseCursor(bool bShow) override;
  bool                         GetShowMouseCursor() const override;
  void                         SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) override;
  xiiMouseCursorClipMode::Enum GetClipMouseCursor() const override;

private:
  void InitializeDevice() override;
  void RegisterInputSlots() override;
  void ResetInputSlotValues() override;
  void UpdateInputSlotValues() override;

private:
  void OnFocusLost();

  static bool                  s_bMainWindowUsed;
  xiiUInt32                    m_uiWindowNumber = 0;
  SDL_Window*                  m_pWindow        = nullptr;
  xiiMouseCursorClipMode::Enum m_ClipCursorMode = xiiMouseCursorClipMode::NoClip;
};
#endif
