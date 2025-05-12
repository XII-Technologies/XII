#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Utilities/CpuWaitOnlyFenceVulkan.h>

xiiGALCpuWaitOnlyFenceVulkan::xiiGALCpuWaitOnlyFenceVulkan(xiiGALDeviceVulkan* pDeviceVulkan) :
  m_pDeviceVulkan(pDeviceVulkan)
{
}

xiiGALCpuWaitOnlyFenceVulkan::~xiiGALCpuWaitOnlyFenceVulkan()
{
  if (!m_SyncPoints.IsEmpty())
  {
    xiiLog::Info("xiiGALCpuWaitOnlyFenceVulkan::DeInitPlatform(): Waiting for {} pending Vulkan fence (s).", m_SyncPoints.GetCount());

    // Vulkan spec states that all queue submission commands that refer to a fence must have completed execution before the fence is destroyed.
    // (https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VUID-vkDestroyFence-fence-01120)
    Wait(xiiMath::MaxValue<xiiUInt64>());
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  if (m_uiMaxSyncPoints > s_uiRequiredArraySize * 2)
  {
    xiiLog::Warning("Max queue size of pending fences is too large. This may indicate that none of the GetCompletedValue(), Wait(), or ExtractSignalSemaphore() methods have been used.");
  }
#endif
}

xiiUInt64 xiiGALCpuWaitOnlyFenceVulkan::GetCompletedValue()
{
  XII_LOCK(m_SyncPointGuard);

  return InternalGetCompletedValue();
}

xiiUInt64 xiiGALCpuWaitOnlyFenceVulkan::InternalGetCompletedValue()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  while (!m_SyncPoints.IsEmpty())
  {
    SyncPointData& syncData = m_SyncPoints.PeekFront();

    vk::Result status = vkLogicalDevice.getFenceStatus(syncData.m_vkFence, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    if (status == vk::Result::eSuccess)
    {
      UpdateLastCompletedFenceValue(syncData.m_uiValue);

      m_pDeviceVulkan->GetVulkanFencePool()->ReclaimFence(std::move(syncData.m_vkFence));

      m_SyncPoints.PopFront();
    }
    else
    {
      break;
    }
  }

  return m_LastCompletedFenceValue;
}

void xiiGALCpuWaitOnlyFenceVulkan::UpdateLastCompletedFenceValue(xiiUInt64 uiValue)
{
  m_LastCompletedFenceValue.Max(uiValue);
}

void xiiGALCpuWaitOnlyFenceVulkan::Reset(xiiUInt64 uiValue)
{
  XII_LOCK(m_SyncPointGuard);

  XII_ASSERT_DEV(uiValue >= m_LastCompletedFenceValue, "Resetting cpu wait only fence to the value ({}) that is smaller than the last completed value ({}).", uiValue, m_LastCompletedFenceValue);

  UpdateLastCompletedFenceValue(uiValue);
}

const xiiGALCpuWaitOnlyFenceVulkan::SyncPointData& xiiGALCpuWaitOnlyFenceVulkan::CreateSyncPoint(const xiiUInt64 uiFenceValue)
{
  XII_LOCK(m_SyncPointGuard);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    const xiiUInt64 uiLastCompletedValue = m_SyncPoints.IsEmpty() ? (const xiiUInt64)m_LastCompletedFenceValue : m_SyncPoints.PeekBack().m_uiValue;

    XII_ASSERT_DEV(uiFenceValue > uiLastCompletedValue, "Creating fence sync point with the value ({}) that is smaller than the last completed value ({}).", uiFenceValue, uiLastCompletedValue);
  }
#endif

  // If fence is used only for synchronization between queues it will accumulate many more sync points.
  // We need to check VkFence and remove already reached sync points.
  if (m_SyncPoints.GetCount() > s_uiRequiredArraySize)
  {
    InternalGetCompletedValue();
  }

  xiiGALCpuWaitOnlyFenceVulkan::SyncPointData& syncPoint = m_SyncPoints.ExpandAndGetRef();
  syncPoint.m_vkFence                                    = m_pDeviceVulkan->GetVulkanFencePool()->RequestFence();
  syncPoint.m_uiValue                                    = uiFenceValue;

  return syncPoint;
}

void xiiGALCpuWaitOnlyFenceVulkan::Wait(xiiUInt64 uiValue)
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  XII_LOCK(m_SyncPointGuard);

  while (!m_SyncPoints.IsEmpty())
  {
    SyncPointData& syncData = m_SyncPoints.PeekFront();

    if (syncData.m_uiValue > uiValue)
      break;

    vk::Result vkFenceStatus = vkLogicalDevice.getFenceStatus(syncData.m_vkFence, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    if (vkFenceStatus == vk::Result::eNotReady)
    {
      vkFenceStatus = vkLogicalDevice.waitForFences(1U, &syncData.m_vkFence, vk::True, xiiMath::MaxValue<xiiUInt64>(), m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    }

    XII_ASSERT_DEV(vkFenceStatus == vk::Result::eSuccess, "All pending fences must now be complete!");

    UpdateLastCompletedFenceValue(syncData.m_uiValue);

    m_pDeviceVulkan->GetVulkanFencePool()->ReclaimFence(std::move(syncData.m_vkFence));

    m_SyncPoints.PopFront();
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Utilities_Implementation_CpuWaitOnlyFenceVulkan);
