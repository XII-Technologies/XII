/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Shader/InputLayoutVulkan.h>
#include <GraphicsVulkan/Shader/ShaderVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALShaderVulkan::xiiGALShaderVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALShader(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALShaderVulkan::~xiiGALShaderVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkShaderModule));
}

xiiResult xiiGALShaderVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  vk::ShaderModuleCreateInfo vkShaderModuleCreateInfo = {};
  vkShaderModuleCreateInfo.flags                      = {};
  vkShaderModuleCreateInfo.codeSize                   = m_Description.m_ByteCode->GetSize();
  vkShaderModuleCreateInfo.pCode                      = reinterpret_cast<const xiiUInt32*>(m_Description.m_ByteCode->GetByteCode());
  vkShaderModuleCreateInfo.pNext                      = nullptr;

  XII_ASSERT_DEV(vkShaderModuleCreateInfo.codeSize % 4 == 0, "The SPIRV shader byte code must be a multiple of 4.");

  VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createShaderModule(&vkShaderModuleCreateInfo, nullptr, &m_vkShaderModule, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return XII_SUCCESS;
}

xiiInternal::NewInstance<xiiGALInputLayout> xiiGALShaderVulkan::CreateInputLayoutPlatform(const xiiGALInputLayoutCreationDescription& description)
{
  xiiSharedPtr<xiiGALDeviceVulkan>                  pDeviceVulkan      = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiInternal::NewInstance<xiiGALInputLayoutVulkan> pInputLayoutVulkan = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALInputLayoutVulkan, pDeviceVulkan, description);

  if (pInputLayoutVulkan->InitPlatform(this).Succeeded())
    return pInputLayoutVulkan;

  XII_DELETE(pDeviceVulkan->GetAllocator(), pInputLayoutVulkan.m_pInstance);

  return pInputLayoutVulkan;
}

void xiiGALShaderVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkShaderModule, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Shader_Implementation_ShaderVulkan);
