#pragma once

#include <GraphicsVulkan/GraphicsVulkanDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>

class XII_GRAPHICSVULKAN_DLL xiiGALCommandQueueVulkan final : public xiiGALCommandQueue
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGALCommandQueueVulkan, xiiGALCommandQueue);

public:
  vk::PipelineStageFlags GetSupportedStagesFlags() const { return m_vkSupportedStageFlags; }
  vk::AccessFlags        GetSupportedAccessFlags() const { return m_vkSupportedAccessFlags; }

public:
  /// \brief This returns the value of the internal fence that will be signaled the next time.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetNextFenceValue() const override final { return m_uiNextFenceValue.load(); }

  /// \brief This returns the last completed value of the internal fence.
  XII_ALWAYS_INLINE virtual xiiUInt64 GetCompletedFenceValue() override final { return m_pQueueFence->GetCompletedValue(); }

  XII_ALWAYS_INLINE const xiiGALQueueInformationVulkan& GetQueueInformation() const { return m_QueueInformation; };
  XII_ALWAYS_INLINE vk::CommandPool GetVulkanCommandPool() const { return m_vkCommandPool; };

  /// \brief This blocks execution until all pending GPU commands are complete.
  virtual xiiUInt64 WaitForIdle() override final;

  virtual xiiGALCommandList* BeginCommandList() override final;
  void                       ResetCommandList(xiiGALCommandListVulkan* pCommandListVulkan);
  void                       RecycleCommandLists();

private:
  xiiUInt64 SubmitCommandList(xiiGALCommandList* pCommandList);

protected:
  friend class xiiGALDeviceVulkan;
  friend class xiiMemoryUtils;
  friend class xiiGALCommandListVulkan;

  xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription);

  virtual ~xiiGALCommandQueueVulkan();

  void InitializePlatform(const xiiGALQueueInformationVulkan& queueInformation);

  void DeInitializePlatform();

  virtual void SetDebugNamePlatform(xiiStringView sName) override final;

private:
  struct CommandListReleaseInfo
  {
    XII_DECLARE_POD_TYPE();

    xiiGALCommandListVulkan* m_pCommandListVulkan = nullptr;
    xiiUInt64                m_uiFenceValue       = 0U;
  };

  xiiMutex                     m_QueueMutex;
  xiiGALQueueInformationVulkan m_QueueInformation;

  vk::CommandPool                           m_vkCommandPool;
  xiiDynamicArray<xiiGALCommandListVulkan*> m_CommandLists;
  xiiDeque<xiiGALCommandListVulkan*>        m_QueuedCommandLists;
  xiiDeque<CommandListReleaseInfo>          m_CommandListsToReset;
  vk::PipelineStageFlags                    m_vkSupportedStageFlags;
  vk::AccessFlags                           m_vkSupportedAccessFlags;

  xiiSharedPtr<xiiGALFenceVulkan>  m_pQueueFence;
  std::atomic<xiiUInt64>           m_uiNextFenceValue = 1U;
  xiiGALFenceVulkan::SyncPointData m_LastSyncPoint;
};
