#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>

xiiGALFenceVulkan::xiiGALFenceVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(pDeviceVulkan, creationDescription)
{
}

xiiGALFenceVulkan::~xiiGALFenceVulkan() = default;

xiiResult xiiGALFenceVulkan::InitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

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

xiiResult xiiGALFenceVulkan::DeInitPlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  if (IsTimelineSemaphore())
  {
    XII_ASSERT_DEV(m_SyncPoints.IsEmpty(), "");

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

  return XII_SUCCESS;
}

void xiiGALFenceVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkTimelineSemaphore, sName.GetData(tmp));
}

void xiiGALFenceVulkan::ReleaseResourcesImmediately()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  pDeviceVulkan->SafeReleaseDeviceObject(std::move(m_vkTimelineSemaphore));

  m_vkTimelineSemaphore = nullptr;
}

xiiUInt64 xiiGALFenceVulkan::GetCompletedValue()
{
  if (IsTimelineSemaphore())
  {
    xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
    vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

    // GetSemaphoreCounter() is thread safe.

    xiiUInt64 uiSemaphoreCounter = xiiInvalidIndex;
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

  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  while (!m_SyncPoints.IsEmpty())
  {
    SyncPointData& syncData = m_SyncPoints.PeekFront();

    vk::Result status = vkLogicalDevice.getFenceStatus(syncData.m_vkFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
    if (status == vk::Result::eSuccess)
    {
      UpdateLastCompletedFenceValue(syncData.m_uiValue);

      m_SyncPoints.PopFront();
    }
    else
    {
      break;
    }
  }

  return m_LastCompletedFenceValue.load();
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

    xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
    vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

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

    xiiUInt64 uiLastCompletedValue = m_LastCompletedFenceValue.load();
    XII_ASSERT_DEV(uiValue >= uiLastCompletedValue, "Resetting fence '{}' to the value ({}) that is smaller than the last completed value ({}).", GetDebugName(), uiValue, uiLastCompletedValue);

    UpdateLastCompletedFenceValue(uiValue);
  }
}

const xiiGALFenceVulkan::SyncPointData& xiiGALFenceVulkan::CreateSyncPoint(const xiiUInt64 uiFenceValue)
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  /// \todo GraphicsVulkan: Use a pool to recycle sync fences.
  vk::FenceCreateInfo vkFenceCreateInfo = {};
  vkFenceCreateInfo.pNext               = nullptr;
  vkFenceCreateInfo.flags               = {};

  vk::Fence vkFence = {};
  VK_ASSERT_DEV(vkLogicalDevice.createFence(&vkFenceCreateInfo, nullptr, &vkFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  xiiGALFenceVulkan::SyncPointData& syncPoint = m_SyncPoints.ExpandAndGetRef();
  syncPoint.m_vkFence                         = vkFence;
  syncPoint.m_uiValue                         = uiFenceValue;

  return syncPoint;
}

void xiiGALFenceVulkan::Wait(xiiUInt64 uiValue)
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

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

      if (syncData.m_uiValue > uiValue)
        break;

      vk::Result status = vkLogicalDevice.getFenceStatus(syncData.m_vkFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      if (status == vk::Result::eNotReady)
      {
        status = vkLogicalDevice.waitForFences(1U, &syncData.m_vkFence, vk::True, xiiMath::MaxValue<xiiUInt64>(), pDeviceVulkan->GetVulkanDynamicDispatchLoader());
      }

      XII_ASSERT_DEV(status == vk::Result::eSuccess, "All pending fences must now be complete!");

      UpdateLastCompletedFenceValue(syncData.m_uiValue);

      pDeviceVulkan->SafeReleaseDeviceObject(std::move(syncData.m_vkFence));

      m_SyncPoints.PopFront();
    }
  }
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FenceVulkan);
