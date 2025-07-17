#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

namespace vk
{
  class CommandPool;
  class CommandBuffer;
} // namespace vk

class XII_GRAPHICSVULKAN_DLL xiiGALCommandBufferPoolVulkan
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALCommandBufferPoolVulkan);

public:
  struct ThreadPool
  {
    vk::CommandPool                    m_vkPrimaryPool   = VK_NULL_HANDLE;
    vk::CommandPool                    m_vkSecondaryPool = VK_NULL_HANDLE;
    xiiDynamicArray<vk::CommandBuffer> m_PrimaryFreeCommandBuffers;
    xiiDynamicArray<vk::CommandBuffer> m_SecondaryFreeCommandBuffers;
    xiiMutex                           m_Mutex;

    /// \brief Push a command buffer back into this thread's free list.
    void Push(vk::CommandBuffer vkCommandBuffer, bool bIsSecondary);
  };

  /// RAII wrapper for a vk::CommandBuffer automatically returns it to its originating pool on destruction.
  struct AutoCommandBuffer
  {
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
      if (m_pOwner != nullptr && m_vkCommandBuffer != VK_NULL_HANDLE)
      {
        m_pOwner->Push(m_vkCommandBuffer, m_bIsSecondary);
      }
    }

    /// \brief Implicit conversion so you can pass it directly to vk calls.
    XII_ALWAYS_INLINE operator vk::CommandBuffer() const { return m_vkCommandBuffer; }

    XII_ALWAYS_INLINE vk::CommandBuffer Get() const { return m_vkCommandBuffer; }

  private:
    ThreadPool*       m_pOwner          = nullptr;
    vk::CommandBuffer m_vkCommandBuffer = VK_NULL_HANDLE;
    bool              m_bIsSecondary    = false;
  };

  /// \brief Allocate a primary-level command buffer for the current thread.
  AutoCommandBuffer AllocatePrimaryCommandBuffer();

  /// \brief Allocate a secondary-level command buffer for the current thread
  AutoCommandBuffer AllocateSecondaryCommandBuffer();

  /// \brief Resets *all* underlying VkCommandPools, invalidating prerecorded buffers.
  /// Freelist is cleared; next allocate will re‐create new buffers.
  void ResetPools();

private:
  /// \brief Constructs a multithreaded command‐buffer pool.
  /// pDeviceVulkan           – Vulkan device implementation.
  /// queueFlags              – The queue flags for all command pools
  /// poolCreateFlags         – Flags for vkCreateCommandPool (e.g. RESET_COMMAND_BUFFER_BIT)
  /// uiInitialCountPerThread – How many buffers to preallocate per thread.
  xiiGALCommandBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, vk::CommandPoolCreateFlags poolCreateFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, xiiUInt32 uiInitialCountPerThread = 32U);

  ~xiiGALCommandBufferPoolVulkan();

  /// \brief Get or create the thread-pool for this thread ID.
  ThreadPool& GetOrCreateThreadPool();

private:
  friend class xiiMemoryUtils;
  friend class xiiGALDeviceVulkan;
  friend class xiiGALCommandQueueVulkan;

  xiiGALDeviceVulkan*                  m_pDeviceVulkan;
  xiiBitflags<xiiGALCommandQueueFlags> m_QueueFlags;
  vk::CommandPoolCreateFlags           m_vkCommandPoolCreateFlags;
  xiiUInt32                            m_uiInitialReserveCount;
  vk::CommandBufferLevel               m_vkDefaultCommandBufferLevel;

  xiiMutex                        m_PoolMutex;
  xiiMap<xiiThreadID, ThreadPool> m_CommandBufferPoolsPerThread;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiAtomicIntegerU32 m_BufferCounter = 0;
#endif
};
