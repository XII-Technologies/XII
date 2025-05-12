#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

#include <GraphicsVulkan/Utilities/CpuWaitOnlyFenceVulkan.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandQueueVulkan final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueVulkan, xiiGALCommandQueue);

public:
  vk::PipelineStageFlags GetSupportedStagesFlags() const { return m_vkSupportedStageFlags; }
  vk::AccessFlags        GetSupportedAccessFlags() const { return m_vkSupportedAccessFlags; }

public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return m_uiNextFenceValue; }

  /// \brief This returns the last completed value of the internal fence.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetCompletedFenceValue() override final { return m_pQueueFence->GetCompletedValue(); }

  XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetQueueInformation() const { return m_QueueInformation; };

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiSharedPtr<xiiGALCommandList> BeginCommandList() override final;

  xiiGALCommandBufferPoolVulkan* GetCommandBufferPool();

private:
  xiiUInt64 SubmitCommandList(xiiGALCommandList* pCommandList);

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListVulkan;

  xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationVulkan& queueInformation);

  virtual ~xiiGALCommandQueueVulkan();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  xiiGALQueueInformationVulkan                                   m_QueueInformation;
  xiiMap<xiiUInt64, xiiUniquePtr<xiiGALCommandBufferPoolVulkan>> m_CommandBufferPool;

  vk::PipelineStageFlags m_vkSupportedStageFlags;
  vk::AccessFlags        m_vkSupportedAccessFlags;

  xiiUniquePtr<xiiGALCpuWaitOnlyFenceVulkan>  m_pQueueFence;
  xiiAtomicIntegerU64                         m_uiNextFenceValue{1ULL};
  xiiGALCpuWaitOnlyFenceVulkan::SyncPointData m_LastSyncPoint;
};
