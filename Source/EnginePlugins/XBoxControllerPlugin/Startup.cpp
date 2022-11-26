#include <Core/System/ControllerInput.h>
#include <Foundation/Configuration/Startup.h>
#include <XBoxControllerPlugin/InputDeviceXBox.h>

static xiiInputDeviceXBox360* g_InputDeviceXBox360 = nullptr;

xiiInputDeviceXBox360* xiiInputDeviceXBox360::GetDevice()
{
  if (g_InputDeviceXBox360 == nullptr)
    g_InputDeviceXBox360 = XII_DEFAULT_NEW(xiiInputDeviceXBox360);

  return g_InputDeviceXBox360;
}

void xiiInputDeviceXBox360::DestroyAllDevices()
{
  XII_DEFAULT_DELETE(g_InputDeviceXBox360);
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(InputDevices, InputDeviceXBox360)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation", 
    "InputManager"
    
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiInputDeviceXBox360::DestroyAllDevices();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiInputDeviceXBox360* pDevice = xiiInputDeviceXBox360::GetDevice();
    if(xiiControllerInput::GetDevice() == nullptr)
    {
      xiiControllerInput::SetDevice(pDevice);
    }
  }
 
  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if(xiiControllerInput::GetDevice() == g_InputDeviceXBox360)
    {
      xiiControllerInput::SetDevice(nullptr);
    }
    xiiInputDeviceXBox360::DestroyAllDevices();
  }
 
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(System, System_XBoxController_Startup);
