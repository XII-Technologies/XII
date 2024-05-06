#include <GraphicsD3D12/GraphicsD3D12PCH.h>

#include <GraphicsD3D12/Shader/ShaderResourceVariableD3D12.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderResourceVariableD3D12, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALShaderResourceVariableD3D12::xiiGALShaderResourceVariableD3D12() :
  xiiGALShaderResourceVariable()
{
}

xiiGALShaderResourceVariableD3D12::~xiiGALShaderResourceVariableD3D12() = default;

void xiiGALShaderResourceVariableD3D12::Set(xiiGALResource* pResource, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableD3D12::SetArray(xiiArrayPtr<xiiGALResource* const> ppResources, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableD3D12::SetBufferRange(xiiGALResource* pResource, xiiUInt64 uiOffset, xiiUInt64 uiSize, xiiUInt32 uiArrayIndex, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableD3D12::SetBufferOffset(xiiUInt32 uiOffset, xiiUInt32 uiArrayIndex)
{
}

xiiEnum<xiiGALShaderResourceVariableType> xiiGALShaderResourceVariableD3D12::GetType() const
{
  return xiiEnum<xiiGALShaderResourceVariableType>();
}

void xiiGALShaderResourceVariableD3D12::GetResourceDescription(xiiGALShaderResourceDescription& resourceDeccription)
{
}

xiiUInt32 xiiGALShaderResourceVariableD3D12::GetIndex() const
{
  return xiiUInt32();
}

xiiArrayPtr<xiiGALResource*> xiiGALShaderResourceVariableD3D12::Get(xiiUInt32 uiIndex) const
{
  return xiiArrayPtr<xiiGALResource*>();
}

XII_STATICLINK_FILE(GraphicsD3D12, GraphicsD3D12_Shader_Implementation_ShaderResourceVariableD3D12);
