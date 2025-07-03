#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/CommandBufferPoolVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription, const xiiGALQueueInformationVulkan& queueInformation) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription), m_QueueInformation(queueInformation), m_vkSupportedStageFlags(pDeviceVulkan->GetVulkanLogicalDeviceSupportedStagesFlags(m_QueueInformation.m_uiQueueFamilyIndex)), m_vkSupportedAccessFlags(pDeviceVulkan->GetVulkanLogicalDeviceSupportedAccessFlags(m_QueueInformation.m_uiQueueFamilyIndex))
{
  m_pQueueFence = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALCpuWaitOnlyFenceVulkan, pDeviceVulkan);
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan()
{
  m_pQueueFence.Clear();

  m_CommandBufferPool.Clear();
}

xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  // Update last completed fence value to unlock all waiting events.
  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.PostIncrement();

  m_QueueInformation.m_vkQueue.waitIdle(pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  // TODO (VERIFY): For some reason after idling the queue not all fences are signaled.
  m_pQueueFence->Wait(xiiMath::MaxValue<xiiUInt64>());
  m_pQueueFence->Reset(uiFenceValue);

  return uiFenceValue;
}

xiiSharedPtr<xiiGALCommandList> xiiGALCommandQueueVulkan::BeginCommandList()
{
  xiiGALDeviceVulkan*                               pDeviceVulkan          = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandBufferPoolVulkan*                    pCommandBufferPool     = GetCommandBufferPool();
  xiiGALCommandListCreationDescription              commandListDescription = {.m_QueueType = m_Description.m_QueueType};
  xiiInternal::NewInstance<xiiGALCommandListVulkan> pCommandListVulkan     = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALCommandListVulkan, xiiSharedPtr<xiiGALDeviceVulkan>(pDeviceVulkan, pDeviceVulkan->GetAllocator()), this, pCommandBufferPool, commandListDescription);

  xiiStringBuilder sb;
  sb.SetFormat("Command List - Thread {}", static_cast<xiiUInt64>(xiiThreadUtils::GetCurrentThreadID()));
  pCommandListVulkan->SetDebugName(sb.GetView());

  XII_ASSERT_DEV(pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Reset, "Command list is not reset.");

  pCommandListVulkan->Begin();

  XII_ASSERT_DEV(pCommandListVulkan != nullptr && pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Recording, "The created command list is not begun.");

  return pCommandListVulkan;
}

xiiGALCommandBufferPoolVulkan* xiiGALCommandQueueVulkan::GetCommandBufferPool()
{
  if (!m_CommandBufferPool.Contains(xiiThreadUtils::GetCurrentThreadID()))
  {
    xiiGALDeviceVulkan* pDeviceVulkan                         = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
    m_CommandBufferPool[xiiThreadUtils::GetCurrentThreadID()] = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALCommandBufferPoolVulkan, pDeviceVulkan, m_QueueInformation, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    xiiStringBuilder sb;
    sb.SetFormat("Command Buffer Pool - Thread {}", static_cast<xiiUInt64>(xiiThreadUtils::GetCurrentThreadID()));
    m_CommandBufferPool[xiiThreadUtils::GetCurrentThreadID()]->SetDebugName(sb.GetView());
  }
  return m_CommandBufferPool[xiiThreadUtils::GetCurrentThreadID()].Borrow();
}

xiiUInt64 xiiGALCommandQueueVulkan::SubmitCommandList(xiiGALCommandList* pCommandList)
{
  xiiGALDeviceVulkan*      pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandListVulkan* pCommandListVulkan = static_cast<xiiGALCommandListVulkan*>(pCommandList);

  pCommandListVulkan->FlushBarriers();

  if (pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Recording)
  {
    pCommandListVulkan->End();
  }

  XII_LOCK(m_QueueMutex);

  bool bTimelineSemaphoreInUse = false;
  for (const auto& fenceInfo : pCommandListVulkan->m_SignalFences)
  {
    if (fenceInfo.m_pFenceVulkan == nullptr || !fenceInfo.m_pFenceVulkan->IsTimelineSemaphore())
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
    vkSubmitInformation.pNext = &vkTimelineSemaphoreSubmitInfo;

    vkTimelineSemaphoreSubmitInfo.pNext                     = nullptr;
    vkTimelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = vkSubmitInformation.waitSemaphoreCount;
    vkTimelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = vkSubmitInformation.waitSemaphoreCount > 0 ? pCommandListVulkan->m_vkWaitSemaphoreValues.GetData() : nullptr;
    vkTimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = vkSubmitInformation.signalSemaphoreCount;
    vkTimelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = vkSubmitInformation.signalSemaphoreCount > 0 ? pCommandListVulkan->m_vkSignalSemaphoreValues.GetData() : nullptr;
  }

  // Increment the value before submitting the buffer to be overly safe.
  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.PostIncrement();
  {
    const auto& syncPoint = m_pQueueFence->CreateSyncPoint(uiFenceValue);

    VK_ASSERT_DEV(m_QueueInformation.m_vkQueue.submit(1U, &vkSubmitInformation, syncPoint.m_vkFence, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_LastSyncPoint = syncPoint;
  }

  for (const auto& fenceInfo : pCommandListVulkan->m_SignalFences)
  {
    if (fenceInfo.m_pFenceVulkan->IsTimelineSemaphore())
      continue;

    const auto& syncPoint = fenceInfo.m_pFenceVulkan->CreateSyncPoint(fenceInfo.m_uiWaitValue);

    XII_IGNORE_UNUSED(syncPoint);
  }

  pCommandListVulkan->ResetPlatform();

  return uiFenceValue;
}

void xiiGALCommandQueueVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
