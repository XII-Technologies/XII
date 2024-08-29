#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

struct xiiAndroidInputEvent;
struct AInputEvent;

/// \brief Android standard input device.
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
  virtual void ResetInputSlotValues() override;

private:
  void AndroidInputEventHandler(xiiAndroidInputEvent& event);
  void AndroidAppCommandEventHandler(xiiInt32 iCmd);
  bool AndroidHandleInput(AInputEvent* pEvent);

private:
  xiiInt32 m_iResolutionX = 0;
  xiiInt32 m_iResolutionY = 0;
};
