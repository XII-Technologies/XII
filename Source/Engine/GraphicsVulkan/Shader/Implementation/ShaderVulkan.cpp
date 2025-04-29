#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALShaderVulkan::xiiGALShaderVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(pDeviceVulkan, creationDescription)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan() = default;

xiiResult xiiGALShaderVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
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
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(m_vkShaderModule);

  return XII_SUCCESS;
}

void xiiGALShaderVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkShaderModule, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderVulkan);
