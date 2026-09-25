/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
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

void xiiGALCommandBufferPoolVulkan::ThreadPool::PushInFlight(vk::CommandBuffer vkCommandBuffer, xiiGALCommandListDataVulkan&& commandListData, bool bIsSecondary, xiiUInt64 uiFenceValue)
{
  XII_LOCK(m_Mutex);

  m_InFlightCommandBuffers.PushBack({vkCommandBuffer, bIsSecondary, uiFenceValue, std::move(commandListData)});
}

//////////////////////////////////////////////////////////////////////////
// Constructor / Destructor

xiiGALCommandBufferPoolVulkan::xiiGALCommandBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, vk::CommandPoolCreateFlags poolCreateFlags /*= vk::CommandPoolCreateFlagBits::eResetCommandBuffer*/, xiiUInt32 uiInitialCountPerThread /*= 16U*/) :
  m_pDeviceVulkan(pDeviceVulkan), m_pCommandQueueVulkan(pCommandQueueVulkan), m_vkCommandPoolCreateFlags(poolCreateFlags), m_uiInitialReserveCount(uiInitialCountPerThread)
{
  XII_ASSERT_DEBUG(m_pDeviceVulkan != nullptr, "Invalid Vulkan device implementation.");
  XII_ASSERT_DEBUG(m_pCommandQueueVulkan != nullptr, "Invalid Vulkan command queue provided.");
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

  // Find or create new sub‐pool for this thread.

  bool bExisted;
  auto it = m_CommandBufferPoolsPerThread.FindOrAdd(uiThreadID, &bExisted);

  if (!bExisted)
  {
    ThreadPool&                         threadPool       = it.Value();
    const xiiGALQueueInformationVulkan& queueInformation = m_pCommandQueueVulkan->GetQueueInformation();

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
      xiiTemporaryHybridArray<vk::CommandBuffer, 4U> tmp;
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

      {
        XII_LOCK(threadPool.m_Mutex);

        threadPool.m_SecondaryFreeCommandBuffers.PushBackRange(tmp);
      }

      VK_ASSERT_DEV(vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, tmp.GetData(), m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    }
  }

  return it.Value();
}

xiiGALCommandBufferPoolVulkan::AutoCommandBuffer xiiGALCommandBufferPoolVulkan::AllocatePrimaryCommandBuffer()
{
  XII_LOCK(m_PoolMutex);

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

  VK_ASSERT_DEV(vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, &vkCommandBuffer, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return {&threadPool, vkCommandBuffer, false};
}

xiiGALCommandBufferPoolVulkan::AutoCommandBuffer xiiGALCommandBufferPoolVulkan::AllocateSecondaryCommandBuffer()
{
  XII_LOCK(m_PoolMutex);

  ThreadPool& threadPool = GetOrCreateThreadPool();

  {
    XII_LOCK(threadPool.m_Mutex);

    if (!threadPool.m_SecondaryFreeCommandBuffers.IsEmpty())
    {
      vk::CommandBuffer vkCommandBuffer = threadPool.m_SecondaryFreeCommandBuffers.PeekBack();
      threadPool.m_SecondaryFreeCommandBuffers.PopBack();

      // GPU is done with this command buffer.
      VK_ASSERT_DEV(vkCommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

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

  VK_ASSERT_DEV(vkLogicalDevice.allocateCommandBuffers(&vkCommandBufferAllocationInfo, &vkCommandBuffer, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  return {&threadPool, vkCommandBuffer, true};
}

void xiiGALCommandBufferPoolVulkan::RecycleAfterSubmit(AutoCommandBuffer&& commandBuffer, xiiGALCommandListDataVulkan&& commandListData, xiiUInt64 uiFenceValue)
{
  if (commandBuffer.m_pOwner == nullptr || commandBuffer.m_vkCommandBuffer == VK_NULL_HANDLE)
    return;

  // Relieve control of the command buffer from AutoCommandBuffer.
  ThreadPool*       pOwner          = commandBuffer.m_pOwner;
  vk::CommandBuffer vkCommandBuffer = commandBuffer.m_vkCommandBuffer;
  bool              bIsSecondary    = commandBuffer.m_bIsSecondary;

  commandBuffer.m_pOwner          = nullptr;
  commandBuffer.m_vkCommandBuffer = VK_NULL_HANDLE;

  // Track it until fence signals.
  // Defer recycling until that fence-value is reached:
  pOwner->PushInFlight(vkCommandBuffer, std::move(commandListData), bIsSecondary, uiFenceValue);
}

void xiiGALCommandBufferPoolVulkan::ReclaimCompleted()
{
  XII_LOCK(m_PoolMutex);

  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  for (auto& it : m_CommandBufferPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();

    XII_LOCK(threadPool.m_Mutex);

    for (xiiUInt32 i = 0; i < threadPool.m_InFlightCommandBuffers.GetCount();)
    {
      InFlightCommandBuffer& inFlightCommandBuffer = threadPool.m_InFlightCommandBuffers[i];
      xiiUInt64              uiCompletedFenceValue = xiiMath::MaxValue<xiiUInt64>();

      const xiiBitflags<xiiGALCommandQueueFlags> queueFlags = m_pCommandQueueVulkan->GetDescription().m_QueueFlags;
      m_pDeviceVulkan->LockCommandQueueAndRun(queueFlags, [&](const vk::Queue&) -> void {
        uiCompletedFenceValue = m_pCommandQueueVulkan->GetCompletedFenceValue();
      });

      if (inFlightCommandBuffer.m_uiFenceValue <= uiCompletedFenceValue)
      {
        for (vk::QueryPool vkQueryPool : inFlightCommandBuffer.m_CommandListData.m_TemporaryQueryPools)
        {
          if (vkQueryPool != VK_NULL_HANDLE)
          {
            vkLogicalDevice.destroyQueryPool(vkQueryPool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
          }
        }

        if (inFlightCommandBuffer.m_bIsSecondary)
        {
          threadPool.m_SecondaryFreeCommandBuffers.PushBack(inFlightCommandBuffer.m_vkCommandBuffer);
        }
        else
        {
          threadPool.m_PrimaryFreeCommandBuffers.PushBack(inFlightCommandBuffer.m_vkCommandBuffer);
        }

        threadPool.m_InFlightCommandBuffers.RemoveAtAndSwap(i);
      }
      else
      {
        ++i;
      }
    }
  }
}

void xiiGALCommandBufferPoolVulkan::ResetPools()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  XII_LOCK(m_PoolMutex);

  for (auto& it : m_CommandBufferPoolsPerThread)
  {
    ThreadPool& threadPool = it.Value();

    VK_ASSERT_DEV(vkLogicalDevice.resetCommandPool(threadPool.m_vkPrimaryPool, vk::CommandPoolResetFlagBits::eReleaseResources, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
    VK_ASSERT_DEV(vkLogicalDevice.resetCommandPool(threadPool.m_vkSecondaryPool, vk::CommandPoolResetFlagBits::eReleaseResources, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    // Clear free‐lists so buffers get re‐created on demand.
    {
      XII_LOCK(threadPool.m_Mutex);

      threadPool.m_PrimaryFreeCommandBuffers.Clear();
      threadPool.m_SecondaryFreeCommandBuffers.Clear();
      threadPool.m_InFlightCommandBuffers.Clear();
    }
  }
}
