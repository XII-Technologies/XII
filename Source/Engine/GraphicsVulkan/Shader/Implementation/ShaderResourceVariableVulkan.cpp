#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Shader/ShaderResourceVariableVulkan.h>

xiiGALShaderResourceVariableVulkan::xiiGALShaderResourceVariableVulkan() :
  xiiGALShaderResourceVariable()
{
}

xiiGALShaderResourceVariableVulkan::~xiiGALShaderResourceVariableVulkan() = default;

xiiResult xiiGALShaderResourceVariableVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALShaderResourceVariableVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderResourceVariableVulkan);
