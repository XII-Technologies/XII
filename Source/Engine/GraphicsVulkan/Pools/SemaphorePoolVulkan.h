/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

namespace vk
{
  class Semaphore;
}

class XII_GRAPHICSVULKAN_DLL xiiGALSemaphorePoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALSemaphorePoolVulkan);

public:
  vk::Semaphore RequestSemaphore();

  void ReclaimSemaphore(vk::Semaphore&& vkSemaphore);

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;

  xiiGALSemaphorePoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiInitialSize);
  ~xiiGALSemaphorePoolVulkan();

  vk::Semaphore CreateVulkanSemaphore();

  xiiGALDeviceVulkan* m_pDeviceVulkan;

  xiiMutex                       m_PoolMutex;
  xiiDynamicArray<vk::Semaphore> m_Semaphores;
  xiiDeque<vk::Semaphore>        m_QueuedSemaphores;
};
