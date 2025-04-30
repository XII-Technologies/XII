#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/FencePoolVulkan.h>

xiiGALFencePoolVulkan::xiiGALFencePoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiUInt32 uiInitialSize) :
  m_pDeviceVulkan(pDeviceVulkan), m_Fences(pDeviceVulkan->GetAllocator()), m_QueuedFences(pDeviceVulkan->GetAllocator())
{
  XII_LOCK(m_PoolMutex);

  for (xiiUInt32 i = 0; i < uiInitialSize; ++i)
  {
    m_QueuedFences.PushBack(CreateVulkanFence());
  }
}

xiiGALFencePoolVulkan::~xiiGALFencePoolVulkan()
{
  XII_LOCK(m_PoolMutex);

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  for (xiiUInt32 i = 0; i < m_Fences.GetCount(); ++i)
  {
    vk::Fence& vkFence = m_Fences[i];

    vkLogicalDevice.destroyFence(vkFence, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }

  m_QueuedFences.Clear();
  m_QueuedFences.Compact();

  m_Fences.Clear();
  m_Fences.Compact();
}

vk::Fence xiiGALFencePoolVulkan::RequestFence()
{
  XII_LOCK(m_PoolMutex);

  if (m_QueuedFences.IsEmpty())
  {
    m_QueuedFences.PushBack(CreateVulkanFence());
  }

  vk::Fence vkFence = m_QueuedFences.PeekFront();
  m_QueuedFences.PopFront();

  return vkFence;
}

void xiiGALFencePoolVulkan::ReclaimFence(vk::Fence& vkFence)
{
  XII_LOCK(m_PoolMutex);

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  VK_ASSERT_DEV(vkLogicalDevice.resetFences(1U, &vkFence, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_QueuedFences.PushBack(vkFence);
}

vk::Fence xiiGALFencePoolVulkan::CreateVulkanFence()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  vk::FenceCreateInfo vkFenceCreateInfo = {};
  vkFenceCreateInfo.pNext               = nullptr;
  vkFenceCreateInfo.flags               = {};

  vk::Fence vkFence = {};
  VK_ASSERT_DEV(vkLogicalDevice.createFence(&vkFenceCreateInfo, nullptr, &vkFence, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_Fences.PushBack(vkFence);

  return vkFence;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Pools_Implementation_FencePoolVulkan);
