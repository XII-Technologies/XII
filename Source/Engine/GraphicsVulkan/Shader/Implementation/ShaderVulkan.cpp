#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

xiiGALShaderVulkan::xiiGALShaderVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceVulkan, creationDescription)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan() = default;

xiiResult xiiGALShaderVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  vk::ShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
  vkShaderModuleCreateInfo.flags                      = {};
  vkShaderModuleCreateInfo.codeSize                   = m_Description.m_ByteCode->GetSize();
  vkShaderModuleCreateInfo.pCode                      = reinterpret_cast<const xiiUInt32*>(m_Description.m_ByteCode->GetByteCode());
  vkShaderModuleCreateInfo.pNext                      = nullptr;

  XII_ASSERT_DEV(vkShaderModuleCreateInfo.codeSize % 4 == 0, "The SPIRV shader byte code must be a multiple of 4.");

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createShaderModule(&vkShaderModuleCreateInfo, nullptr, &m_vkShaderModule, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

xiiResult xiiGALShaderVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkShaderModule));

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderVulkan);
