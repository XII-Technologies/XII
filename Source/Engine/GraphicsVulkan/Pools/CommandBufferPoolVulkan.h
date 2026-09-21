/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsVulkan/CommandEncoder/CommandListDataVulkan.h>

namespace vk
{
  class CommandPool;
  class CommandBuffer;
  class Fence;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALCommandBufferPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandBufferPoolVulkan);

public:
  struct InFlightCommandBuffer
  {
    vk::CommandBuffer           m_vkCommandBuffer;
    bool                        m_bIsSecondary;
    xiiUInt64                   m_uiFenceValue;
    xiiGALCommandListDataVulkan m_CommandListData;
  };

  struct ThreadPool
  {
    XII_ALWAYS_INLINE ThreadPool() = default;

    XII_ALWAYS_INLINE ThreadPool(ThreadPool&& other) noexcept :
      m_vkPrimaryPool(std::exchange(other.m_vkPrimaryPool, VK_NULL_HANDLE)), m_vkSecondaryPool(std::exchange(other.m_vkSecondaryPool, VK_NULL_HANDLE)), m_PrimaryFreeCommandBuffers(std::move(other.m_PrimaryFreeCommandBuffers)), m_SecondaryFreeCommandBuffers(std::move(other.m_SecondaryFreeCommandBuffers)), m_InFlightCommandBuffers(std::move(other.m_InFlightCommandBuffers))
    {
      // m_Mutex is default-initialized
    }

    ThreadPool& operator=(ThreadPool&& other) noexcept
    {
      if (this != &other)
      {
        // Lock both mutexes to avoid data races.
        XII_LOCK(m_Mutex);
        XII_LOCK(other.m_Mutex);

        m_vkPrimaryPool   = std::exchange(other.m_vkPrimaryPool, VK_NULL_HANDLE);
        m_vkSecondaryPool = std::exchange(other.m_vkSecondaryPool, VK_NULL_HANDLE);

        m_PrimaryFreeCommandBuffers   = std::move(other.m_PrimaryFreeCommandBuffers);
        m_SecondaryFreeCommandBuffers = std::move(other.m_SecondaryFreeCommandBuffers);
        m_InFlightCommandBuffers      = std::move(other.m_InFlightCommandBuffers);

        // m_Mutex remains default-constructed.
      }
      return *this;
    }

    vk::CommandPool                        m_vkPrimaryPool   = VK_NULL_HANDLE;
    vk::CommandPool                        m_vkSecondaryPool = VK_NULL_HANDLE;
    xiiDynamicArray<vk::CommandBuffer>     m_PrimaryFreeCommandBuffers;
    xiiDynamicArray<vk::CommandBuffer>     m_SecondaryFreeCommandBuffers;
    xiiDynamicArray<InFlightCommandBuffer> m_InFlightCommandBuffers;
    xiiMutex                               m_Mutex;

    /// Push a command buffer back into this thread's free list. Immediate return to free-list
    void Push(vk::CommandBuffer vkCommandBuffer, bool bIsSecondary);

    /// Defer recycle: GPU is still using it.
    void PushInFlight(vk::CommandBuffer vkCommandBuffer, xiiGALCommandListDataVulkan&& commandListData, bool bIsSecondary, xiiUInt64 uiFenceValue);
  };

  /// RAII handle for a VkCommandBuffer allocated from this pool.
  /// On destruction: if not submitted, returns it immediately to free-list.
  /// If submitted via xiiGALCommandBufferPoolVulkan::RecycleAfterSubmit, the command buffer is moved out and not auto-recycled.
  struct AutoCommandBuffer
  {
    XII_DISALLOW_COPY_AND_ASSIGN(AutoCommandBuffer);

    XII_ALWAYS_INLINE AutoCommandBuffer() = default;

    XII_ALWAYS_INLINE AutoCommandBuffer(struct ThreadPool* pThreadPool, vk::CommandBuffer vkCommandBuffer, bool bIsSecondary) noexcept :
      m_pOwner(pThreadPool), m_vkCommandBuffer(vkCommandBuffer), m_bIsSecondary(bIsSecondary)
    {
    }

    XII_ALWAYS_INLINE AutoCommandBuffer(AutoCommandBuffer&& rhs) noexcept :
      m_pOwner(rhs.m_pOwner), m_vkCommandBuffer(rhs.m_vkCommandBuffer), m_bIsSecondary(rhs.m_bIsSecondary)
    {
      rhs.m_pOwner          = nullptr;
      rhs.m_vkCommandBuffer = VK_NULL_HANDLE;
    }

    XII_ALWAYS_INLINE AutoCommandBuffer& operator=(AutoCommandBuffer&& rhs) noexcept
    {
      if (m_pOwner != nullptr && m_vkCommandBuffer != VK_NULL_HANDLE)
      {
        m_pOwner->Push(m_vkCommandBuffer, m_bIsSecondary);
      }

      m_pOwner              = rhs.m_pOwner;
      m_vkCommandBuffer     = rhs.m_vkCommandBuffer;
      m_bIsSecondary        = rhs.m_bIsSecondary;
      rhs.m_pOwner          = nullptr;
      rhs.m_vkCommandBuffer = VK_NULL_HANDLE;

      return *this;
    }

    XII_ALWAYS_INLINE ~AutoCommandBuffer()
    {
      // Only immediate recycle (no GPU in-flight).
      if (m_pOwner != nullptr && m_vkCommandBuffer != VK_NULL_HANDLE)
      {
        m_pOwner->Push(m_vkCommandBuffer, m_bIsSecondary);
      }
    }

    /// Implicit conversion so you can pass it directly to vk calls.
    XII_ALWAYS_INLINE operator vk::CommandBuffer() const { return m_vkCommandBuffer; }

    XII_ALWAYS_INLINE vk::CommandBuffer Get() const { return m_vkCommandBuffer; }

  private:
    friend class xiiGALCommandBufferPoolVulkan;

    ThreadPool*       m_pOwner          = nullptr;
    vk::CommandBuffer m_vkCommandBuffer = VK_NULL_HANDLE;
    bool              m_bIsSecondary    = false;
  };

  /// Allocate a primary-level command buffer for the current thread.
  AutoCommandBuffer AllocatePrimaryCommandBuffer();

  /// Allocate a secondary-level command buffer for the current thread
  AutoCommandBuffer AllocateSecondaryCommandBuffer();

  /// Submit wrapper: after vkQueueSubmit(..., fence), call this to defer recycling.
  void RecycleAfterSubmit(AutoCommandBuffer&& commandBuffer, xiiGALCommandListDataVulkan&& commandListData, xiiUInt64 uiFenceValue);

  /// Poll fences and reclaim any completed buffers.
  /// Call at the start of each frame or from a dedicated thread.
  void ReclaimCompleted();

  /// Resets *all* underlying VkCommandPools, invalidating prerecorded buffers.
  /// Freelist is cleared; next allocate will re‐create new buffers.
  /// Hard reset all pools (invalidates *all* buffers, in-flight or free).
  void ResetPools();

private:
  /// Constructs a multithreaded command‐buffer pool.
  /// pDeviceVulkan           – Vulkan device implementation.
  /// pCommandQueueVulkan     - The command queue.
  /// poolCreateFlags         – Flags for vkCreateCommandPool (e.g. RESET_COMMAND_BUFFER_BIT)
  /// uiInitialCountPerThread – How many buffers to preallocate per thread.
  xiiGALCommandBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiGALCommandQueueVulkan* pCommandQueueVulkan, vk::CommandPoolCreateFlags poolCreateFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, xiiUInt32 uiInitialCountPerThread = 0U);

  ~xiiGALCommandBufferPoolVulkan();

  /// Get or create the thread-pool for this thread ID.
  ThreadPool& GetOrCreateThreadPool();

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALCommandQueueVulkan;

  xiiGALDeviceVulkan*        m_pDeviceVulkan;
  xiiGALCommandQueueVulkan*  m_pCommandQueueVulkan;
  vk::CommandPoolCreateFlags m_vkCommandPoolCreateFlags;
  xiiUInt32                  m_uiInitialReserveCount;

  xiiMutex                                                m_PoolMutex;
  xiiMap<xiiThreadID, ThreadPool>                         m_CommandBufferPoolsPerThread;
  xiiMap<xiiThreadID, xiiDynamicArray<vk::CommandBuffer>> m_FreeCommandBuffersPerThread;
};
