#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Shader/ShaderNull.h>

xiiGALShaderNull::xiiGALShaderNull(const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(creationDescription)
{
}

xiiGALShaderNull::~xiiGALShaderNull() = default;

xiiResult xiiGALShaderNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALShaderNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Shader_Implementation_ShaderNull);
