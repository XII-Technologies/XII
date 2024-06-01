#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>

xiiGALShaderVulkan::xiiGALShaderVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceVulkan, creationDescription)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan() = default;

xiiResult xiiGALShaderVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALShaderVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderVulkan);
