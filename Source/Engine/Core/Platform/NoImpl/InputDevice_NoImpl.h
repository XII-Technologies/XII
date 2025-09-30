#pragma once

#if XII_DISABLED(XII_SUPPORTS_SDL)

#  include <Core/Input/DeviceTypes/MouseKeyboard.h>

class XII_CORE_DLL xiiStandardInputDevice : public xiiInputDeviceMouseKeyboard
{
  XII_ADD_DYNAMIC_REFLECTION(xiiStandardInputDevice, xiiInputDeviceMouseKeyboard);

public:
  xiiStandardInputDevice(xiiUInt32 uiWindowNumber);
  ~xiiStandardInputDevice();

  virtual void                         SetShowMouseCursor(bool bShow) override;
  virtual bool                         GetShowMouseCursor() const override;
  virtual void                         SetClipMouseCursor(xiiMouseCursorClipMode::Enum mode) override;
  virtual xiiMouseCursorClipMode::Enum GetClipMouseCursor() const override;

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
};

#endif
