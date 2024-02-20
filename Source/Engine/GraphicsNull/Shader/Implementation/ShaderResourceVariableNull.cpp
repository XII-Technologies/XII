#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Shader/ShaderResourceVariableNull.h>

xiiGALShaderResourceVariableNull::xiiGALShaderResourceVariableNull() :
  xiiGALShaderResourceVariable()
{
}

xiiGALShaderResourceVariableNull::~xiiGALShaderResourceVariableNull() = default;

xiiResult xiiGALShaderResourceVariableNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALShaderResourceVariableNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Shader_Implementation_ShaderResourceVariableNull);
