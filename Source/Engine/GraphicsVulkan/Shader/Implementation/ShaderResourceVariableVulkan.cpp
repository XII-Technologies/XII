#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Shader/ShaderResourceVariableVulkan.h>

#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceVariable.h>

xiiGALShaderResourceVariableVulkan::xiiGALShaderResourceVariableVulkan() :
  xiiGALShaderResourceVariable()
{
}

xiiGALShaderResourceVariableVulkan::~xiiGALShaderResourceVariableVulkan() = default;

void xiiGALShaderResourceVariableVulkan::Set(xiiGALResource* pResource, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableVulkan::SetArray(xiiArrayPtr<xiiGALResource* const> ppResources, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableVulkan::SetBufferRange(xiiGALResource* pResource, xiiUInt64 uiOffset, xiiUInt64 uiSize, xiiUInt32 uiArrayIndex, xiiBitflags<xiiGALSetShaderResourceFlags> flags)
{
}

void xiiGALShaderResourceVariableVulkan::SetBufferOffset(xiiUInt32 uiOffset, xiiUInt32 uiArrayIndex)
{
}

xiiEnum<xiiGALShaderResourceVariableType> xiiGALShaderResourceVariableVulkan::GetType() const
{
  return xiiEnum<xiiGALShaderResourceVariableType>();
}

void xiiGALShaderResourceVariableVulkan::GetResourceDescription(xiiGALShaderResourceDescription& resourceDeccription)
{
}

xiiUInt32 xiiGALShaderResourceVariableVulkan::GetIndex() const
{
  return xiiUInt32();
}

xiiArrayPtr<xiiGALResource*> xiiGALShaderResourceVariableVulkan::Get(xiiUInt32 uiIndex) const
{
  return xiiArrayPtr<xiiGALResource*>();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderResourceVariableVulkan);
