#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Shader/InputLayoutNull.h>
#include <GraphicsNull/Shader/ShaderNull.h>

xiiGALInputLayoutNull::xiiGALInputLayoutNull(const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(creationDescription)
{
}

xiiGALInputLayoutNull::~xiiGALInputLayoutNull() = default;

xiiResult xiiGALInputLayoutNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALInputLayoutNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Shader_Implementation_InputLayoutNull);
