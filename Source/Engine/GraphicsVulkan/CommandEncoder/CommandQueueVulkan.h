/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

namespace vk
{
  class Fence;
}

class XII_GRAPHICSVULKAN_DLL xiiGALCommandQueueVulkan final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueVulkan, xiiGALCommandQueue);

public:
  /// This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return m_uiNextFenceValue; }

  /// This returns the last completed value of the internal fence.
  virtual xiiUInt64 GetCompletedFenceValue() override final;

  XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetQueueInformation() const { return m_QueueInformation; };

  XII_ALWAYS_INLINE xiiGALCpuWaitOnlyFenceVulkan* GetWaitOnlyFence() const { return m_pQueueFence.Borrow(); }

  virtual xiiUInt64 SubmitPlatform(xiiGALCommandList* pCommandList) override final;

  /// This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListVulkan;

  xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationVulkan& queueInformation);

  virtual ~xiiGALCommandQueueVulkan();

private:
  struct SyncPointData
  {
    xiiUInt64 m_uiValue;
    vk::Fence m_vkFence;
  };

  xiiGALQueueInformationVulkan                                   m_QueueInformation;
  xiiMap<xiiUInt64, xiiUniquePtr<xiiGALCommandBufferPoolVulkan>> m_CommandBufferPool;

  xiiUniquePtr<xiiGALCpuWaitOnlyFenceVulkan> m_pQueueFence;
  xiiAtomicIntegerU64                        m_uiNextFenceValue{1ULL};
  xiiUInt64                                  m_uiLastSyncPointValue{0ULL};
};
