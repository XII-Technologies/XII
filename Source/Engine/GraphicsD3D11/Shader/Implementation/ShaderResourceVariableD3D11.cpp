#include <GraphicsD3D11/GraphicsD3D11PCH.h>

#include <GraphicsD3D11/Shader/ShaderResourceVariableD3D11.h>

#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceVariable.h>

xiiGALShaderResourceVariableD3D11::xiiGALShaderResourceVariableD3D11() :
  xiiGALShaderResourceVariable()
{
}

xiiGALShaderResourceVariableD3D11::~xiiGALShaderResourceVariableD3D11() = default;

void xiiGALShaderResourceVariableD3D11::Set(xiiGALResource* pResource, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableD3D11::SetArray(xiiArrayPtr<xiiGALResource* const> ppResources, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableD3D11::SetBufferRange(xiiGALResource* pResource, xiiUInt64 uiOffset, xiiUInt64 uiSize, xiiUInt32 uiArrayIndex, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableD3D11::SetBufferOffset(xiiUInt32 uiOffset, xiiUInt32 uiArrayIndex)
{
}

xiiEnum<xiiGALShaderResourceVariableType> xiiGALShaderResourceVariableD3D11::GetType() const
{
  return xiiEnum<xiiGALShaderResourceVariableType>();
}

void xiiGALShaderResourceVariableD3D11::GetResourceDescription(xiiGALShaderResourceDescription& resourceDeccription)
{
}

xiiUInt32 xiiGALShaderResourceVariableD3D11::GetIndex() const
{
  return xiiUInt32();
}

xiiArrayPtr<xiiGALResource*> xiiGALShaderResourceVariableD3D11::Get(xiiUInt32 uiIndex) const
{
  return xiiArrayPtr<xiiGALResource*>();
}

XII_STATICLINK_FILE(GraphicsD3D11, GraphicsD3D11_Shader_Implementation_ShaderResourceVariableD3D11);
