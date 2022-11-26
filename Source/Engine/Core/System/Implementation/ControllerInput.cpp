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

#if XII_ENABLED(XII_SUPPORTS_GLFW)
#  include <Core/System/Implementation/glfw/ControllerInput_glfw.inl>
#endif
