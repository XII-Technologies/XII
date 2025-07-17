#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

namespace vk
{
  class Fence;
}

class XII_GRAPHICSVULKAN_DLL xiiGALFencePoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALFencePoolVulkan);

public:
  vk::Fence RequestFence();

  void ReclaimFence(vk::Fence&& vkFence);

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  xiiGALFencePoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiInitialSize);
  ~xiiGALFencePoolVulkan();

  vk::Fence CreateVulkanFence();

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiMutex                   m_PoolMutex;
  xiiDynamicArray<vk::Fence> m_Fences;
  xiiDeque<vk::Fence>        m_QueuedFences;
};
