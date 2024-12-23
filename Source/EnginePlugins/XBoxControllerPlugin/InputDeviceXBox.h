#pragma once

#include <Core/Input/DeviceTypes/Controller.h>
#include <XBoxControllerPlugin/XBoxControllerDLL.h>

/// \brief An implementation of xiiInputDeviceController that handles XBox 360 controllers.
///
/// Works on all platforms that provide the XINPUT API.
class XII_XBOXCONTROLLER_DLL xiiInputDeviceXBox360 : public xiiInputDeviceController
{
  XII_ADD_DYNAMIC_REFLECTION(xiiInputDeviceXBox360, xiiInputDeviceController);

public:
  xiiInputDeviceXBox360();
  ~xiiInputDeviceXBox360();

  /// \brief Returns a xiiInputDeviceXBox360 device.
  static xiiInputDeviceXBox360* GetDevice();

  /// \brief Destroys all devices of this type. Automatically called at engine shutdown.
  static void DestroyAllDevices();

  virtual bool IsControllerConnected(xiiUInt8 uiPhysical) const override;

private:
  virtual void ApplyVibration(xiiUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;

  bool m_bControllerConnected[xiiInputDeviceController::MaxControllers];

  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void UpdateHardwareState(xiiTime tTimeDifference) override;

  void SetValue(xiiInt32 iController, const char* szButton, float fValue);

  static void RegisterControllerButton(const char* szButton, const char* szName, xiiBitflags<xiiInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);
};
