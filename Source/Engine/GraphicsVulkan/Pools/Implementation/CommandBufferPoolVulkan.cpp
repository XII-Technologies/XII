#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/CommandBufferPoolVulkan.h>

//////////////////////////////////////////////////////////////////////////
// ThreadPool Push

void xiiGALCommandBufferPoolVulkan::ThreadPool::Push(vk::CommandBuffer vkCommandBuffer, bool bIsSecondary)
{
  XII_LOCK(m_Mutex);

  if (bIsSecondary)
  {
    m_SecondaryFreeCommandBuffers.PushBack(vkCommandBuffer);
  }
  else
  {
    m_PrimaryFreeCommandBuffers.PushBack(vkCommandBuffer);
  }
}

//////////////////////////////////////////////////////////////////////////
// Constructor / Destructor

xiiGALCommandBufferPoolVulkan::xiiGALCommandBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, vk::CommandPoolCreateFlags poolCreateFlags, xiiUInt32 uiInitialCountPerThread) :
  m_pDeviceVulkan(pDeviceVulkan), m_QueueFlags(queueFlags), m_vkCommandPoolCreateFlags(poolCreateFlags), m_uiInitialReserveCount(uiInitialCountPerThread)
{
}

xiiGALCommandBufferPoolVulkan::~xiiGALCommandBufferPoolVulkan()
{
  XII_LOCK(m_PoolMutex);

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  for (auto& it : m_CommandBufferPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();

    if (threadPool.m_vkPrimaryPool != VK_NULL_HANDLE)
    {
      vkLogicalDevice.destroyCommandPool(threadPool.m_vkPrimaryPool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
    if (threadPool.m_vkSecondaryPool != VK_NULL_HANDLE)
    {
      vkLogicalDevice.destroyCommandPool(threadPool.m_vkSecondaryPool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// Command Buffer Pool API

xiiGALCommandBufferPoolVulkan::ThreadPool& xiiGALCommandBufferPoolVulkan::GetOrCreateThreadPool()
{
  xiiThreadID uiThreadID      = xiiThreadUtils::GetCurrentThreadID();
  vk::Device  vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  XII_LOCK(m_PoolMutex);

  // Find existing thread pool.s
  {
    auto it = m_CommandBufferPoolsPerThread.Find(uiThreadID);

    if (it.IsValid())
      return it.Value();
  }

  // Create new sub‐pool for this thread.
  ThreadPool                   threadPool;
  xiiGALQueueInformationVulkan queueInformation = m_pDeviceVulkan->GetCommandQueueInformation(m_QueueFlags);

  vk::CommandPoolCreateInfo vkCommandPoolCreateInfo = {};
  vkCommandPoolCreateInfo.pNext                     = nullptr;
  vkCommandPoolCreateInfo.flags                     = m_vkCommandPoolCreateFlags;
  vkCommandPoolCreateInfo.queueFamilyIndex          = queueInformation.m_uiQueueFamilyIndex;

  VK_ASSERT_DEV(vkLogicalDevice.createCommandPool(&vkCommandPoolCreateInfo, nullptr, &threadPool.m_vkPrimaryPool, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  VK_ASSERT_DEV(vkLogicalDevice.createCommandPool(&vkCommandPoolCreateInfo, nullptr, &threadPool.m_vkSecondaryPool, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  // Preallocate a batch of buffers for speed.
  if (m_uiInitialReserveCount > 0U)
  {
    // Allocate primary buffers.
    xiiHybridArray<vk::CommandBuffer, 16U> tmp;
    tmp.SetCountUninitialized(m_uiInitialReserveCount);

    vk::CommandBufferAllocateInfo vkCommandBufferAllocationInfo = {};
    vkCommandBufferAllocationInfo.pNext                         = nullptr;
    vkCommandBufferAllocationInfo.commandPool                   = threadPool.m_vkPrimaryPool;
    vkCommandBufferAllocationInfo.level                         = vk::CommandBufferLevel::ePrimary;
    vkCommandBufferAllocationInfo.commandBufferCount            = tmp.GetCount();

    VK_ASSERT_DEV(vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, tmp.GetData(), m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    {
      XII_LOCK(threadPool.m_Mutex);

      threadPool.m_PrimaryFreeCommandBuffers.PushBackRange(tmp);
    }

    // Allocate secondary buffers.
    vkCommandBufferAllocationInfo.commandPool = threadPool.m_vkSecondaryPool;
    vkCommandBufferAllocationInfo.level       = vk::CommandBufferLevel::eSecondary;

    VK_ASSERT_DEV(vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, tmp.GetData(), m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }

  auto it = m_CommandBufferPoolsPerThread.Insert(uiThreadID, threadPool);

  return it.Value();
}

xiiGALCommandBufferPoolVulkan::AutoCommandBuffer xiiGALCommandBufferPoolVulkan::AllocatePrimaryCommandBuffer()
{
  ThreadPool& threadPool = GetOrCreateThreadPool();

  {
    XII_LOCK(threadPool.m_Mutex);

    if (!threadPool.m_PrimaryFreeCommandBuffers.IsEmpty())
    {
      vk::CommandBuffer vkCommandBuffer = threadPool.m_PrimaryFreeCommandBuffers.PeekBack();
      threadPool.m_PrimaryFreeCommandBuffers.PopBack();

      return {&threadPool, vkCommandBuffer, false};
    }
  }

  // No free buffers, allocate one.
  vk::CommandBufferAllocateInfo vkCommandBufferAllocationInfo = {};
  vkCommandBufferAllocationInfo.pNext                         = nullptr;
  vkCommandBufferAllocationInfo.commandPool                   = threadPool.m_vkPrimaryPool;
  vkCommandBufferAllocationInfo.level                         = vk::CommandBufferLevel::ePrimary;
  vkCommandBufferAllocationInfo.commandBufferCount            = 1U;

  vk::Device        vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();
  vk::CommandBuffer vkCommandBuffer;

  vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, &vkCommandBuffer, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return {&threadPool, vkCommandBuffer, false};
}

xiiGALCommandBufferPoolVulkan::AutoCommandBuffer xiiGALCommandBufferPoolVulkan::AllocateSecondaryCommandBuffer()
{
  ThreadPool& threadPool = GetOrCreateThreadPool();

  {
    XII_LOCK(threadPool.m_Mutex);

    if (!threadPool.m_SecondaryFreeCommandBuffers.IsEmpty())
    {
      vk::CommandBuffer vkCommandBuffer = threadPool.m_SecondaryFreeCommandBuffers.PeekBack();
      threadPool.m_SecondaryFreeCommandBuffers.PopBack();

      return {&threadPool, vkCommandBuffer, true};
    }
  }

  // No free buffers, allocate one.
  vk::CommandBufferAllocateInfo vkCommandBufferAllocationInfo = {};
  vkCommandBufferAllocationInfo.pNext                         = nullptr;
  vkCommandBufferAllocationInfo.commandPool                   = threadPool.m_vkSecondaryPool;
  vkCommandBufferAllocationInfo.level                         = vk::CommandBufferLevel::eSecondary;
  vkCommandBufferAllocationInfo.commandBufferCount            = 1U;

  vk::Device        vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();
  vk::CommandBuffer vkCommandBuffer;

  vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, &vkCommandBuffer, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return {&threadPool, vkCommandBuffer, true};
}

void xiiGALCommandBufferPoolVulkan::ResetPools()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  XII_LOCK(m_PoolMutex);

  for (auto& it : m_CommandBufferPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();

    vkLogicalDevice.resetCommandPool(threadPool.m_vkPrimaryPool, vk::CommandPoolResetFlagBits::eReleaseResources, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    vkLogicalDevice.resetCommandPool(threadPool.m_vkSecondaryPool, vk::CommandPoolResetFlagBits::eReleaseResources, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());

    // Clear free‐lists so buffers get re‐created on demand.
    {
      XII_LOCK(threadPool.m_Mutex);

      threadPool.m_PrimaryFreeCommandBuffers.Clear();
      threadPool.m_SecondaryFreeCommandBuffers.Clear();
    }
  }
}
