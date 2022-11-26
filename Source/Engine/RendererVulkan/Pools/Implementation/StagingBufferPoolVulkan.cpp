#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void xiiStagingBufferPoolVulkan::Initialize(xiiGALDeviceVulkan* pDevice)
{
  m_pDevice = pDevice;
  m_device  = pDevice->GetVulkanDevice();
}

void xiiStagingBufferPoolVulkan::DeInitialize()
{
  m_device = nullptr;
}

xiiStagingBufferVulkan xiiStagingBufferPoolVulkan::AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size)
{
  //#TODO_VULKAN alignment
  xiiStagingBufferVulkan buffer;

  XII_ASSERT_DEBUG(m_device, "xiiStagingBufferPoolVulkan::Initialize not called");
  vk::BufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.size                 = size;
  bufferCreateInfo.usage                = vk::BufferUsageFlagBits::eTransferSrc;

  bufferCreateInfo.pQueueFamilyIndices   = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode           = vk::SharingMode::eExclusive;


  xiiVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = xiiVulkanMemoryUsage::Auto;
  allocInfo.m_flags = xiiVulkanAllocationCreateFlags::HostAccessSequentialWrite;

  VK_ASSERT_DEV(xiiMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, buffer.m_buffer, buffer.m_alloc, &buffer.m_allocInfo));

  return buffer;
}

void xiiStagingBufferPoolVulkan::ReclaimBuffer(xiiStagingBufferVulkan& buffer)
{
  m_pDevice->DeleteLater(buffer.m_buffer, buffer.m_alloc);

  //XII_ASSERT_DEBUG(m_device, "xiiStagingBufferPoolVulkan::Initialize not called");
  //xiiMemoryAllocatorVulkan::DestroyBuffer(buffer.m_buffer, buffer.m_alloc);
}
