#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/SemaphorePoolVulkan.h>

xiiGALSemaphorePoolVulkan::xiiGALSemaphorePoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiInitialSize) :
  m_pDeviceVulkan(pDeviceVulkan), m_Semaphores(pDeviceVulkan->GetAllocator()), m_QueuedSemaphores(pDeviceVulkan->GetAllocator())
{
  XII_LOCK(m_PoolMutex);

  for (xiiUInt32 i = 0; i < uiInitialSize; ++i)
  {
    m_QueuedSemaphores.PushBack(CreateVulkanSemaphore());
  }
}

xiiGALSemaphorePoolVulkan::~xiiGALSemaphorePoolVulkan()
{
  XII_LOCK(m_PoolMutex);

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  for (xiiUInt32 i = 0; i < m_Semaphores.GetCount(); ++i)
  {
    vk::Semaphore& vkSemaphore = m_Semaphores[i];

    vkLogicalDevice.destroySemaphore(vkSemaphore, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }

  m_QueuedSemaphores.Clear();
  m_QueuedSemaphores.Compact();

  m_Semaphores.Clear();
  m_Semaphores.Compact();
}

vk::Semaphore xiiGALSemaphorePoolVulkan::RequestSemaphore()
{
  XII_LOCK(m_PoolMutex);

  if (m_QueuedSemaphores.IsEmpty())
  {
    m_QueuedSemaphores.PushBack(CreateVulkanSemaphore());
  }

  vk::Semaphore vkSemaphore = m_QueuedSemaphores.PeekFront();
  m_QueuedSemaphores.PopFront();

  return vkSemaphore;
}

void xiiGALSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore&& vkSemaphore)
{
  XII_LOCK(m_PoolMutex);

  m_QueuedSemaphores.PushBack(vkSemaphore);
}

vk::Semaphore xiiGALSemaphorePoolVulkan::CreateVulkanSemaphore()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
  vkSemaphoreCreateInfo.pNext                   = nullptr;
  vkSemaphoreCreateInfo.flags                   = {};

  vk::Semaphore vkSemaphore = {};
  VK_ASSERT_DEV(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &vkSemaphore, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_Semaphores.PushBack(vkSemaphore);

  return vkSemaphore;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_SemaphorePoolVulkan);
