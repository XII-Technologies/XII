#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Shader/InputLayoutNull.h>
#include <GraphicsNull/Shader/ShaderNull.h>

xiiGALInputLayoutNull::xiiGALInputLayoutNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALInputLayoutCreationDescription& creationDescription) :
  xiiGALInputLayout(pDeviceNull, creationDescription)
{
}

xiiGALInputLayoutNull::~xiiGALInputLayoutNull() = default;

xiiResult xiiGALInputLayoutNull::InitPlatform(xiiGALShader* pShader)
{
  XII_IGNORE_UNUSED(pShader);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Shader_Implementation_InputLayoutNull);
