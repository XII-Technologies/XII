#pragma once

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class xiiGALDeviceVulkan;

struct xiiStagingBufferVulkan
{
  vk::Buffer              m_buffer;
  xiiVulkanAllocation     m_alloc;
  xiiVulkanAllocationInfo m_allocInfo;
};

class XII_RENDERERVULKAN_DLL xiiStagingBufferPoolVulkan
{
public:
  void Initialize(xiiGALDeviceVulkan* pDevice);
  void DeInitialize();

  xiiStagingBufferVulkan AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size);
  void                   ReclaimBuffer(xiiStagingBufferVulkan& buffer);

private:
  xiiGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device          m_device;
};
