#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandBufferPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandBufferPoolVulkan);

public:
  vk::CommandBuffer RequestCommandBuffer(xiiStringView sDebugName = {});

  void ReclaimCommandBuffer(vk::CommandBuffer&& vkCommandBuffer);

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALCommandQueueVulkan;

  xiiGALCommandBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALQueueInformationVulkan& queueInformation, vk::CommandPoolCreateFlags vkCommandPoolCreateFlags);
  ~xiiGALCommandBufferPoolVulkan();

  void SetDebugName(xiiStringView sName);

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiMutex                    m_PoolMutex;
  vk::CommandPool             m_vkCommandPool;
  xiiDeque<vk::CommandBuffer> m_CommandBuffers;

  const vk::PipelineStageFlags m_vkSupportedStageFlags;
  const vk::AccessFlags        m_vkSupportedAccessFlags;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiAtomicIntegerU32 m_BufferCounter = 0;
#endif
};
