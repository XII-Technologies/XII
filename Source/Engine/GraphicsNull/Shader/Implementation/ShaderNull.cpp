#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Shader/ShaderNull.h>
#include <GraphicsNull/Shader/InputLayoutNull.h>

xiiGALShaderNull::xiiGALShaderNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceNull, creationDescription)
{
}

xiiGALShaderNull::~xiiGALShaderNull() = default;

xiiResult xiiGALShaderNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALInputLayout> xiiGALShaderNull::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceNull>                  pDeviceNull      = m_pDevice.Downcast<xiiGALDeviceNull>();
  xiiInternal::NewInstance<xiiGALInputLayoutNull> pInputLayoutNull = XII_NEW(pDeviceNull->GetAllocator(), xiiGALInputLayoutNull, pDeviceNull, description);

  if (pInputLayoutNull->InitPlatform().Succeeded())
    return pInputLayoutNull;

  XII_DELETE(pDeviceNull->GetAllocator(), pInputLayoutNull.m_pInstance);

  return pInputLayoutNull;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Shader_Implementation_ShaderNull);
