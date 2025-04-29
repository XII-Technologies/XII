#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferViewVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBufferViewVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBufferViewVulkan::xiiGALBufferViewVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(pDeviceVulkan, pBuffer, creationDescription)
{
}

xiiGALBufferViewVulkan::~xiiGALBufferViewVulkan() = default;

xiiResult xiiGALBufferViewVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan     = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALBufferVulkan* pBufferVulkan     = static_cast<xiiGALBufferVulkan*>(pDeviceVulkan->GetBuffer(m_Description.m_hBuffer));
  const auto&         bufferDescription = pBufferVulkan->GetDescription();

  if (bufferDescription.m_Mode == xiiGALBufferMode::Formatted)
  {
    vk::BufferViewCreateInfo vkBufferViewCreateInfo = {};
    vkBufferViewCreateInfo.pNext                    = nullptr;
    vkBufferViewCreateInfo.flags                    = {};
    vkBufferViewCreateInfo.buffer                   = pBufferVulkan->GetVulkanBuffer();
    vkBufferViewCreateInfo.format                   = xiiVulkanTypeConversions::GetFormat(m_Description.m_Format);
    vkBufferViewCreateInfo.offset                   = m_Description.m_uiByteOffset;
    vkBufferViewCreateInfo.range                    = m_Description.m_uiByteWidth;

    vk::Device vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();
    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createBufferView(&vkBufferViewCreateInfo, nullptr, &m_vkBufferView, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }
  else if (bufferDescription.m_Mode == xiiGALBufferMode::Structured || bufferDescription.m_Mode == xiiGALBufferMode::Raw)
  {
    // Structured and raw buffers are mapped to storage buffers.
  }

  m_vkDescriptorBufferInfo.buffer = pBufferVulkan->GetVulkanBuffer();
  m_vkDescriptorBufferInfo.offset = m_Description.m_uiByteOffset;
  m_vkDescriptorBufferInfo.range  = m_Description.m_uiByteWidth;

  return XII_SUCCESS;
}

xiiResult xiiGALBufferViewVulkan::DeInitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(m_vkBufferView);

  m_vkBufferView = VK_NULL_HANDLE;

  return XII_SUCCESS;
}

void xiiGALBufferViewVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkBufferView, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BufferViewVulkan);
