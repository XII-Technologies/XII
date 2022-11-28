#include <Core/CorePCH.h>

#include <Core/System/ControllerInput.h>
#include <Foundation/System/PlatformFeatures.h>

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

#if XII_ENABLED(XII_SUPPORTS_SDL)
#  include <Core\System\Implementation\SDL\ControllerInput_SDL.inl>
#endif
