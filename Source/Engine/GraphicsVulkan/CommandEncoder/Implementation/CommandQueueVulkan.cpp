/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>
#include <GraphicsVulkan/Utilities/CpuWaitOnlyFenceVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationVulkan& queueInformation) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription), m_QueueInformation(queueInformation)
{
  m_pQueueFence = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALCpuWaitOnlyFenceVulkan, pDeviceVulkan);
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan() = default;

xiiUInt64 xiiGALCommandQueueVulkan::GetCompletedFenceValue()
{
  return m_pQueueFence->GetCompletedValue();
}

void xiiGALCommandQueueVulkan::WaitForFenceValue(xiiUInt64 uiFenceValue)
{
  XII_ASSERT_DEV(uiFenceValue < m_uiNextFenceValue, "Cannot wait for queue fence value {} because it has not been submitted.", uiFenceValue);
  m_pQueueFence->Wait(uiFenceValue);
}

xiiUInt64 xiiGALCommandQueueVulkan::SubmitPlatform(xiiGALCommandList* pCommandList)
{
  XII_ASSERT_DEV(m_pQueueFence != nullptr, "The command queue fence must be valid.");

  xiiGALDeviceVulkan*      pDeviceVulkan      = xiiDynamicCast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandListVulkan* pCommandListVulkan = xiiDynamicCast<xiiGALCommandListVulkan*>(pCommandList);

  bool bTimelineSemaphoreInUse = false;

  for (const auto& fenceInfo : pCommandListVulkan->m_SignalFences)
  {
    if (!fenceInfo.m_pFenceVulkan->IsTimelineSemaphore())
      continue;

    bTimelineSemaphoreInUse = true;

    fenceInfo.m_pFenceVulkan->ValidateFenceSignal(fenceInfo.m_uiWaitValue);

    pCommandListVulkan->m_vkSignalSemaphores.PushBack(fenceInfo.m_pFenceVulkan->GetVulkanTimelineSemaphore());
    pCommandListVulkan->m_vkSignalSemaphoreValues.PushBack(fenceInfo.m_uiWaitValue);
  }

  for (const auto& fenceInfo : pCommandListVulkan->m_WaitFences)
  {
    fenceInfo.m_pFenceVulkan->ValidateDeviceWaitForFence(fenceInfo.m_uiWaitValue);

    if (!fenceInfo.m_pFenceVulkan->IsTimelineSemaphore())
      continue;

    bTimelineSemaphoreInUse = true;

    vk::Semaphore vkWaitSemaphore = fenceInfo.m_pFenceVulkan->GetVulkanTimelineSemaphore();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    for (xiiUInt32 i = 0; i < pCommandListVulkan->m_vkWaitSemaphores.GetCount(); ++i)
    {
      XII_ASSERT_DEV(pCommandListVulkan->m_vkWaitSemaphores[i] != vkWaitSemaphore, "Fence '{}' with value ({}) is already added to wait operation with value ({}).", fenceInfo.m_pFenceVulkan->GetDebugName(), fenceInfo.m_uiWaitValue, pCommandListVulkan->m_vkWaitSemaphoreValues[i]);
    }
#endif

    pCommandListVulkan->m_vkWaitSemaphores.PushBack(vkWaitSemaphore);
    pCommandListVulkan->m_vkWaitSemaphoreValues.PushBack(fenceInfo.m_uiWaitValue);
    pCommandListVulkan->m_vkWaitDestinationStageFlags.PushBack(vk::PipelineStageFlagBits::eAllCommands);
  }

  XII_ASSERT_DEV(pCommandListVulkan->m_vkWaitSemaphores.GetCount() == pCommandListVulkan->m_vkWaitSemaphoreValues.GetCount(), "The wait semaphores and wait semaphore values must have the same count.");
  XII_ASSERT_DEV(pCommandListVulkan->m_vkSignalSemaphores.GetCount() == pCommandListVulkan->m_vkSignalSemaphoreValues.GetCount(), "The signal semaphores and signal semaphore values must have the same count.");
  XII_ASSERT_DEV(pCommandListVulkan->m_vkWaitDestinationStageFlags.GetCount() == pCommandListVulkan->m_vkWaitSemaphores.GetCount(), "The wait semaphores and wait destination stage flags must have the same count.");

  vk::CommandBuffer vkCommandBuffer = pCommandListVulkan->GetVulkanCommandBuffer();

  vk::SubmitInfo vkSubmitInformation       = {};
  vkSubmitInformation.pNext                = nullptr;
  vkSubmitInformation.waitSemaphoreCount   = pCommandListVulkan->m_vkWaitSemaphores.GetCount();
  vkSubmitInformation.pWaitSemaphores      = pCommandListVulkan->m_vkWaitSemaphores.IsEmpty() ? nullptr : pCommandListVulkan->m_vkWaitSemaphores.GetData();
  vkSubmitInformation.pWaitDstStageMask    = pCommandListVulkan->m_vkWaitDestinationStageFlags.IsEmpty() ? nullptr : pCommandListVulkan->m_vkWaitDestinationStageFlags.GetData();
  vkSubmitInformation.pCommandBuffers      = &vkCommandBuffer;
  vkSubmitInformation.commandBufferCount   = 1U;
  vkSubmitInformation.signalSemaphoreCount = pCommandListVulkan->m_vkSignalSemaphores.GetCount();
  vkSubmitInformation.pSignalSemaphores    = pCommandListVulkan->m_vkSignalSemaphores.IsEmpty() ? nullptr : pCommandListVulkan->m_vkSignalSemaphores.GetData();

  vk::TimelineSemaphoreSubmitInfo vkTimelineSemaphoreSubmitInfo = {};
  if (bTimelineSemaphoreInUse)
  {
    vkTimelineSemaphoreSubmitInfo.pNext                     = nullptr;
    vkTimelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = vkSubmitInformation.waitSemaphoreCount;
    vkTimelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = vkSubmitInformation.waitSemaphoreCount > 0 ? pCommandListVulkan->m_vkWaitSemaphoreValues.GetData() : nullptr;
    vkTimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = vkSubmitInformation.signalSemaphoreCount;
    vkTimelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = vkSubmitInformation.signalSemaphoreCount > 0 ? pCommandListVulkan->m_vkSignalSemaphoreValues.GetData() : nullptr;

    vkSubmitInformation.pNext = &vkTimelineSemaphoreSubmitInfo;
  }

  xiiUInt64 uiFenceValue = xiiMath::MaxValue<xiiUInt64>();
  pDeviceVulkan->LockCommandQueueAndRun(m_Description.m_QueueFlags, [&](const vk::Queue& vkQueue) -> void {
    uiFenceValue          = m_uiNextFenceValue.PostIncrement();
    const auto& syncPoint = m_pQueueFence->CreateSyncPoint(uiFenceValue);

    VK_ASSERT_DEV(vkQueue.submit(1U, &vkSubmitInformation, syncPoint.m_vkFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_uiLastSyncPointValue = syncPoint.m_uiValue;
  });

  for (const auto& fenceInfo : pCommandListVulkan->m_SignalFences)
  {
    if (fenceInfo.m_pFenceVulkan->IsTimelineSemaphore())
      continue;

    fenceInfo.m_pFenceVulkan->AddPendingSyncPoint(this, fenceInfo.m_uiWaitValue, m_uiLastSyncPointValue);
  }

  return uiFenceValue;
}

xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  // Update last completed fence value to unlock all waiting events.
  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.PostIncrement();

  pDeviceVulkan->LockCommandQueueAndRun(m_Description.m_QueueFlags, [pDeviceVulkan](const vk::Queue& vkQueue) -> void {
    VK_ASSERT_DEV(vkQueue.waitIdle(pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  });

  m_pQueueFence->Wait(xiiMath::MaxValue<xiiUInt64>());
  m_pQueueFence->Reset(uiFenceValue);

  return uiFenceValue;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
