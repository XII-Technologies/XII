#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALCommandQueueVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription), m_CommandLists(pDeviceVulkan->GetAllocator()), m_QueuedCommandLists(pDeviceVulkan->GetAllocator()), m_CommandListsToReset(pDeviceVulkan->GetAllocator())
{
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan() = default;

void xiiGALCommandQueueVulkan::InitializePlatform(const xiiGALQueueInformationVulkan& queueInformation)
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  m_QueueInformation = queueInformation;

  vk::CommandPoolCreateInfo commandPoolCreationDescription = {};
  commandPoolCreationDescription.pNext                     = nullptr;
  commandPoolCreationDescription.flags                     = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  commandPoolCreationDescription.queueFamilyIndex          = m_QueueInformation.m_uiQueueFamilyIndex;

  VK_ASSERT_DEV(vkLogicalDevice.createCommandPool(&commandPoolCreationDescription, nullptr, &m_vkCommandPool, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_vkSupportedStageFlags  = pDeviceVulkan->GetVulkanLogicalDeviceSupportedStagesFlags(m_QueueInformation.m_uiQueueFamilyIndex);
  m_vkSupportedAccessFlags = pDeviceVulkan->GetVulkanLogicalDeviceSupportedAccessFlags(m_QueueInformation.m_uiQueueFamilyIndex);

  xiiGALFenceCreationDescription fenceDescription = {.m_Type = xiiGALFenceType::CpuWaitOnly};
  m_pQueueFence                                   = pDeviceVulkan->CreateFenceInternal(fenceDescription);
}

void xiiGALCommandQueueVulkan::DeInitializePlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan   = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  vk::Device          vkLogicalDevice = pDeviceVulkan->GetVulkanLogicalDevice();

  for (xiiUInt32 i = 0; i < m_CommandLists.GetCount(); ++i)
  {
    xiiGALCommandListVulkan* pCommandList = m_CommandLists[i];

    XII_DELETE(pDeviceVulkan->GetAllocator(), pCommandList);
  }

  m_CommandLists.Clear();
  m_QueuedCommandLists.Clear();
  m_CommandListsToReset.Clear();

  pDeviceVulkan->DestroyFenceInternal(m_pQueueFence);

  vkLogicalDevice.destroyCommandPool(m_vkCommandPool, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  m_vkCommandPool = VK_NULL_HANDLE;
}

xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  XII_LOCK(m_QueueMutex);

  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  // Update last completed fence value to unlock all waiting events.
  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.fetch_add(1);

  m_QueueInformation.m_vkQueue.waitIdle(pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  // TODO (VERIFY): For some reason after idling the queue not all fences are signaled.
  m_pQueueFence->Wait(xiiMath::MaxValue<xiiUInt64>());
  m_pQueueFence->Reset(uiFenceValue);

  return uiFenceValue;
}

xiiGALCommandList* xiiGALCommandQueueVulkan::BeginCommandList()
{
  XII_LOCK(m_QueueMutex);

  XII_ASSERT_DEV(m_vkCommandPool != nullptr, "");

  xiiGALCommandListVulkan* pCommandListVulkan = nullptr;
  {

    if (m_QueuedCommandLists.IsEmpty())
    {
      // Allocate a new command list.
      xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
      xiiGALDeviceVulkan*                  pDeviceVulkan          = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
      pCommandListVulkan                                          = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALCommandListVulkan, pDeviceVulkan, this, commandListDescription);

      m_CommandLists.PushBack(pCommandListVulkan);
      m_QueuedCommandLists.PushBack(pCommandListVulkan);

      xiiStringBuilder sb;
      sb.SetFormat("Command List {}", m_CommandLists.GetCount());
      pCommandListVulkan->SetDebugName(sb);
    }

    pCommandListVulkan = m_QueuedCommandLists.PeekFront();

    m_QueuedCommandLists.PopFront();
  }

  XII_ASSERT_DEV(pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Reset, "Command list is not reset.");

  pCommandListVulkan->Begin();

  XII_ASSERT_DEV(pCommandListVulkan != nullptr && pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Recording, "The retrieved command list is not begun.");

  return pCommandListVulkan;
}

void xiiGALCommandQueueVulkan::ResetCommandList(xiiGALCommandListVulkan* pCommandListVulkan)
{
  XII_LOCK(m_QueueMutex);

  XII_ASSERT_DEV(pCommandListVulkan->GetRecordingState() != xiiGALCommandList::RecordingState::Recording, "Command list has not yet been ended.");
  XII_ASSERT_DEV(m_CommandLists.Contains(pCommandListVulkan), "Command list not found in allocated command lists.");

  m_CommandListsToReset.PushBack(CommandListReleaseInfo{.m_pCommandListVulkan = pCommandListVulkan, .m_uiFenceValue = GetNextFenceValue()});
}

void xiiGALCommandQueueVulkan::RecycleCommandLists()
{
  XII_LOCK(m_QueueMutex);

  // Reset completed command lists.
  xiiUInt64 uiCompletedValue = m_pQueueFence->GetCompletedValue();

  while (!m_CommandListsToReset.IsEmpty() && (m_CommandListsToReset.PeekFront().m_uiFenceValue <= uiCompletedValue))
  {
    auto& commandListInfo = m_CommandListsToReset.PeekFront();

    XII_ASSERT_DEV(commandListInfo.m_pCommandListVulkan->GetRecordingState() != xiiGALCommandList::RecordingState::Reset, "Command list is already reset.");

    commandListInfo.m_pCommandListVulkan->ResetInternal();

    XII_ASSERT_DEV(commandListInfo.m_pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Reset, "Command list is not reset.");
    XII_ASSERT_DEV(!m_QueuedCommandLists.Contains(commandListInfo.m_pCommandListVulkan), "Implementation error.");

    m_QueuedCommandLists.PushBack(commandListInfo.m_pCommandListVulkan);

    m_CommandListsToReset.PopFront();
  }
}

xiiUInt64 xiiGALCommandQueueVulkan::SubmitCommandList(xiiGALCommandList* pCommandList)
{
  XII_LOCK(m_QueueMutex);

  xiiGALDeviceVulkan*      pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandListVulkan* pCommandListVulkan = static_cast<xiiGALCommandListVulkan*>(pCommandList);

  pCommandListVulkan->FlushBarriers();

  if (pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Recording)
  {
    pCommandListVulkan->End();
  }

  XII_ASSERT_DEV(!m_pQueueFence->IsTimelineSemaphore(), "The queue fence should be a CPU wait fence only.");

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
    vkSubmitInformation.pNext = &vkTimelineSemaphoreSubmitInfo;

    vkTimelineSemaphoreSubmitInfo.pNext                     = nullptr;
    vkTimelineSemaphoreSubmitInfo.waitSemaphoreValueCount   = vkSubmitInformation.waitSemaphoreCount;
    vkTimelineSemaphoreSubmitInfo.pWaitSemaphoreValues      = vkSubmitInformation.waitSemaphoreCount > 0 ? pCommandListVulkan->m_vkWaitSemaphoreValues.GetData() : nullptr;
    vkTimelineSemaphoreSubmitInfo.signalSemaphoreValueCount = vkSubmitInformation.signalSemaphoreCount;
    vkTimelineSemaphoreSubmitInfo.pSignalSemaphoreValues    = vkSubmitInformation.signalSemaphoreCount > 0 ? pCommandListVulkan->m_vkSignalSemaphoreValues.GetData() : nullptr;
  }

  // Increment the value before submitting the buffer to be overly safe.
  const xiiUInt64 uiFenceValue = m_uiNextFenceValue.fetch_add(1);
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

void xiiGALCommandQueueVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkCommandPool, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
