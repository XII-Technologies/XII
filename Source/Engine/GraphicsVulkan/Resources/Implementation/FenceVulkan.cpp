#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Utilities/CpuWaitOnlyFenceVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFenceVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALFenceVulkan::xiiGALFenceVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALFenceVulkan::~xiiGALFenceVulkan()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (IsTimelineSemaphore())
  {
    XII_ASSERT_DEV(m_SyncPoints.IsEmpty(), "Sync points are not permitted with timeline semaphores.");

    pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkTimelineSemaphore));
  }
  else if (!m_SyncPoints.IsEmpty())
  {
    xiiLog::Info("xiiGALFenceVulkan::DeInitPlatform(): Waiting for {} pending Vulkan fence (s).", m_SyncPoints.GetCount());

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

xiiResult xiiGALFenceVulkan::InitPlatform()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  if (m_Description.m_Type == xiiGALFenceType::General && pDeviceVulkan->GetFeatures().m_NativeFence == xiiGALDeviceFeatureState::Enabled)
  {
    vk::SemaphoreTypeCreateInfo vkTimelineCreateInfo = {};
    vkTimelineCreateInfo.semaphoreType               = vk::SemaphoreType::eTimeline;
    vkTimelineCreateInfo.initialValue                = 0U;

    vk::SemaphoreCreateInfo vkSemaphoreCreateInfo = {};
    vkSemaphoreCreateInfo.pNext                   = &vkTimelineCreateInfo;

    VK_SUCCEED_OR_RETURN_XII_FAILURE(vkLogicalDevice.createSemaphore(&vkSemaphoreCreateInfo, nullptr, &m_vkTimelineSemaphore, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }

  return XII_SUCCESS;
}

void xiiGALFenceVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  xiiStringBuilder                 tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkTimelineSemaphore, sName.GetData(tmp));
}

void xiiGALFenceVulkan::ReleaseResourcesImmediately()
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkTimelineSemaphore));
}

xiiUInt64 xiiGALFenceVulkan::GetCompletedValue()
{
  if (IsTimelineSemaphore())
  {
    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

    // GetSemaphoreCounter() is thread safe.

    uint64_t uiSemaphoreCounter = xiiMath::MaxValue<xiiUInt64>();
    VK_ASSERT_DEV(vkLogicalDevice.getSemaphoreCounterValueKHR(m_vkTimelineSemaphore, &uiSemaphoreCounter, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    return uiSemaphoreCounter;
  }
  else
  {
    XII_LOCK(m_SyncPointGuard);

    return InternalGetCompletedValue();
  }
}

xiiUInt64 xiiGALFenceVulkan::InternalGetCompletedValue()
{
  XII_ASSERT_DEV(!IsTimelineSemaphore(), "The fence must have no timeline semaphore.");

  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  while (!m_SyncPoints.IsEmpty())
  {
    SyncPointData&  syncData         = m_SyncPoints.PeekFront();
    const xiiUInt64 uiCompletedValue = syncData.m_pCommandQueueVulkan->GetCompletedFenceValue();

    if (syncData.m_uiFenceValue <= uiCompletedValue)
    {
      UpdateLastCompletedFenceValue(syncData.m_uiWaitValue);

      m_SyncPoints.PopFront();
    }
    else
    {
      break;
    }
  }

  return m_LastCompletedFenceValue;
}

void xiiGALFenceVulkan::Signal(xiiUInt64 uiValue)
{
  XII_ASSERT_DEV(m_Description.m_Type == xiiGALFenceType::General, "Fence must have been created with xiiGALFenceType::General.");

  if (IsTimelineSemaphore())
  {
    ValidateFenceSignal(uiValue);

    // SignalSemaphore() is thread safe.

    vk::SemaphoreSignalInfo vkSignalInformation = {};
    vkSignalInformation.pNext                   = nullptr;
    vkSignalInformation.semaphore               = m_vkTimelineSemaphore;
    vkSignalInformation.value                   = uiValue;

    xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
    vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

    VK_ASSERT_DEV(vkLogicalDevice.signalSemaphoreKHR(&vkSignalInformation, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }
  else
  {
    XII_REPORT_FAILURE("Signal() is supported only with the timeline semaphore, enable NativeFence feature to use it.");
  }
}

void xiiGALFenceVulkan::Reset(xiiUInt64 uiValue)
{
  if (IsTimelineSemaphore())
  {
    XII_REPORT_FAILURE("Reset() is not supported for timeline semaphore.");
  }
  else
  {
    XII_LOCK(m_SyncPointGuard);

    XII_ASSERT_DEV(uiValue >= m_LastCompletedFenceValue, "Resetting fence '{}' to the value ({}) that is smaller than the last completed value ({}).", GetDebugName(), uiValue, m_LastCompletedFenceValue);

    UpdateLastCompletedFenceValue(uiValue);
  }
}

void xiiGALFenceVulkan::AddPendingSyncPoint(xiiGALCommandQueueVulkan* pCommandQueueVulkan, const xiiUInt64 uiWaitValue, const xiiUInt64 uiFenceValue)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan = m_pDevice.Downcast<xiiGALDeviceVulkan>();

  if (IsTimelineSemaphore())
  {
    XII_REPORT_FAILURE("AddPendingSyncPoint() is not supported for timeline semaphore.");
  }

  ValidateFenceSignal(uiFenceValue);

  XII_LOCK(m_SyncPointGuard);

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  {
    const xiiUInt64 uiLastCompletedValue = m_SyncPoints.IsEmpty() ? (const xiiUInt64)m_LastCompletedFenceValue : m_SyncPoints.PeekBack().m_uiWaitValue;

    XII_ASSERT_DEV(uiFenceValue > uiLastCompletedValue, "Creating fence sync point with the value ({}) that is smaller than the last completed value ({}).", uiFenceValue, uiLastCompletedValue);
  }
  if (!m_SyncPoints.IsEmpty())
  {
    XII_ASSERT_DEV(m_SyncPoints.PeekBack().m_pCommandQueueVulkan == pCommandQueueVulkan, "Fence enqueued for signal operation in command queue {}, but previous signal operation was in command queue {}. This may cause data rase or deadlock. Call Wait() to ensure that all pending signal operation have been completed.", pCommandQueueVulkan->GetDebugName(), m_SyncPoints.PeekBack().m_pCommandQueueVulkan->GetDebugName());
  }
#endif

  // If fence is used only for synchronization between queues it will accumulate many more sync points.
  // We need to check VkFence and remove already reached sync points.
  if (m_SyncPoints.GetCount() > s_uiRequiredArraySize)
  {
    InternalGetCompletedValue();
  }

  xiiGALFenceVulkan::SyncPointData& syncPoint = m_SyncPoints.ExpandAndGetRef();
  syncPoint.m_pCommandQueueVulkan             = pCommandQueueVulkan;
  syncPoint.m_uiWaitValue                     = uiWaitValue;
  syncPoint.m_uiFenceValue                    = uiFenceValue;
}

void xiiGALFenceVulkan::Wait(xiiUInt64 uiValue)
{
  xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan   = m_pDevice.Downcast<xiiGALDeviceVulkan>();
  vk::Device                       vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  if (IsTimelineSemaphore())
  {
    vk::SemaphoreWaitInfo vkWaitInformation = {};
    vkWaitInformation.pNext                 = nullptr;
    vkWaitInformation.flags                 = {};
    vkWaitInformation.semaphoreCount        = 1U;
    vkWaitInformation.pSemaphores           = &m_vkTimelineSemaphore;
    vkWaitInformation.pValues               = &uiValue;

    VK_ASSERT_DEV(vkLogicalDevice.waitSemaphoresKHR(&vkWaitInformation, xiiMath::MaxValue<xiiUInt64>(), pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }
  else
  {
    XII_LOCK(m_SyncPointGuard);

    while (!m_SyncPoints.IsEmpty())
    {
      SyncPointData& syncData = m_SyncPoints.PeekFront();

      if (syncData.m_uiWaitValue > uiValue)
        break;

      syncData.m_pCommandQueueVulkan->GetWaitOnlyFence()->Wait(syncData.m_uiFenceValue);

      UpdateLastCompletedFenceValue(syncData.m_uiWaitValue);

      m_SyncPoints.PopFront();
    }
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FenceVulkan);
