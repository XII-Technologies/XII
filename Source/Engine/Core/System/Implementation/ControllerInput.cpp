#include <Core/CorePCH.h>

#include <Core/System/ControllerInput.h>

namespace
{
  xiiInputDeviceController* g_pInputDeviceController = nullptr;
}

bool xiiControllerInput::HasDevice()
{
  return g_pInputDeviceController != nullptr;
}

xiiInputDeviceController* xiiControllerInput::GetDevice()
{
  return g_pInputDeviceController;
}

void xiiControllerInput::SetDevice(xiiInputDeviceController* pDevice)
{
  g_pInputDeviceController = pDevice;
}

XII_STATICLINK_FILE(Core, Core_System_Implementation_ControllerInput);
