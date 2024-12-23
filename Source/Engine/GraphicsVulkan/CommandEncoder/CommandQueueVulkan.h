#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandQueueVulkan final : public xiiGALCommandQueue
{
public:
  struct FenceInfo
  {
    vk::Fence m_vkFence;
    xiiUInt64 m_uiWaitValue = 0U;
  };

  vk::PipelineStageFlags GetSupportedStagesFlags() const { return m_vkSupportedStageFlags; }
  vk::AccessFlags        GetSupportedAccessFlags() const { return m_vkSupportedAccessFlags; }

public:
  void TransitionImageLayout(xiiGALTextureVulkan* pTextureVulkan, vk::ImageLayout imageLayout);

  void AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags pipelineFlags);
  void AddSignalSemaphore(vk::Semaphore semaphore);

  /// \brief This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return 0ULL; }

  /// \brief This returns the last completed value of the internal fence.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetCompletedFenceValue() override final { return 0ULL; }

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;

  void BeginCommandList(xiiGALCommandListVulkan* pCommandListVulkan);
  void ResetCommandList(xiiGALCommandListVulkan* pCommandListVulkan);

  XII_ALWAYS_INLINE xiiUInt32 GetVulkanQueueFamilyIndex() const { return m_uiQueueFamilyIndex; };
  XII_ALWAYS_INLINE vk::Queue GetVulkanQueue() const { return m_vkQueue; };
  XII_ALWAYS_INLINE vk::CommandPool GetVulkanCommandPool() const { return m_vkCommandPool; };

  void Flush();

protected:
  xiiUInt64 SubmitCommandList(xiiGALCommandList* pCommandList, bool bReset);

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListVulkan;

  xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueVulkan();

  void InitializePlatform(xiiUInt32 uiQueueFamilyIndex, vk::Queue vkQueue);

  void DeInitializePlatform();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

protected:
  xiiMutex m_QueueMutex;

  vk::Device m_vkDevice;
  vk::Queue  m_vkQueue;
  xiiUInt32  m_uiQueueFamilyIndex = xiiInvalidIndex;

  vk::CommandPool                    m_vkCommandPool;
  xiiDeque<xiiGALCommandListVulkan*> m_CommandLists;
  vk::PipelineStageFlags             m_vkSupportedStageFlags;
  vk::AccessFlags                    m_vkSupportedAccessFlags;

  xiiDynamicArray<vk::Semaphore>          m_vkWaitSemaphores;
  xiiDynamicArray<vk::Semaphore>          m_vkSignalSemaphores;
  xiiDynamicArray<vk::PipelineStageFlags> m_vkWaitDestinationStageFlags;

  // Can be used only if timeline semaphore extension is enabled.
  xiiDynamicArray<xiiUInt64> m_vkWaitSemaphoreValues;
  xiiDynamicArray<xiiUInt64> m_vkSignalSemaphoreValues;

  // List of fences to signal/wait next time the command queue is flushed.
  xiiDynamicArray<xiiGALCommandQueueVulkan::FenceInfo> m_SignalFences;
  xiiDynamicArray<xiiGALCommandQueueVulkan::FenceInfo> m_WaitFences;
};
