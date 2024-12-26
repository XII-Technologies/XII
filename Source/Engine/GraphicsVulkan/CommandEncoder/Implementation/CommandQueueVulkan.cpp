#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FenceVulkan.h>

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription), m_CommandLists(pDeviceVulkan->GetAllocator())
{
  /// \todo GraphicsVulkan: Allocate arrays with xiiGALDeviceVulkan memory allocator.
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan() = default;

void xiiGALCommandQueueVulkan::InitializePlatform(xiiUInt32 uiQueueFamilyIndex, vk::Queue vkQueue)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkDevice           = pDeviceVulkan->GetVulkanLogicalDevice();
  m_uiQueueFamilyIndex = uiQueueFamilyIndex;
  m_vkQueue            = vkQueue;

  vk::CommandPoolCreateInfo commandPoolCreationDescription = {};
  commandPoolCreationDescription.pNext                     = nullptr;
  commandPoolCreationDescription.flags                     = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  commandPoolCreationDescription.queueFamilyIndex          = m_uiQueueFamilyIndex;

  VK_ASSERT_DEV(m_vkDevice.createCommandPool(&commandPoolCreationDescription, nullptr, &m_vkCommandPool, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  m_vkSupportedStageFlags  = pDeviceVulkan->GetVulkanLogicalDeviceSupportedStagesFlags(uiQueueFamilyIndex);
  m_vkSupportedAccessFlags = pDeviceVulkan->GetVulkanLogicalDeviceSupportedAccessFlags(uiQueueFamilyIndex);
}

void xiiGALCommandQueueVulkan::DeInitializePlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  for (xiiUInt32 i = 0; i < m_CommandLists.GetCount(); ++i)
  {
    xiiGALCommandListVulkan* pCommandList = m_CommandLists[i];

    XII_DELETE(pDeviceVulkan->GetAllocator(), pCommandList);
  }
  m_CommandLists.Clear();

  m_vkDevice.destroyCommandPool(m_vkCommandPool, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  m_vkCommandPool = nullptr;

  m_vkDevice = nullptr;
}

xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkQueue.waitIdle(pDeviceVulkan->GetVulkanDynamicDispatchLoader());

  return xiiUInt64();
}

xiiGALCommandList* xiiGALCommandQueueVulkan::BeginCommandList()
{
  XII_ASSERT_DEV(m_vkDevice != nullptr, "");
  XII_ASSERT_DEV(m_vkCommandPool != nullptr, "");

  XII_LOCK(m_QueueMutex);

  xiiGALCommandListVulkan* pCommandListVulkan = nullptr;
  if (!m_CommandLists.IsEmpty() && m_CommandLists.PeekFront()->GetRecordingState() == xiiGALCommandList::RecordingState::Reset)
  {
    pCommandListVulkan = m_CommandLists.PeekFront();
  }

  XII_ASSERT_DEV(!(pCommandListVulkan != nullptr && pCommandListVulkan->GetRecordingState() != xiiGALCommandList::RecordingState::Reset), "The retrieved command list is not reset.");

  if (pCommandListVulkan == nullptr)
  {
    // Allocate a new command list.
    xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
    xiiGALDeviceVulkan*                  pDeviceVulkan          = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
    pCommandListVulkan                                          = XII_NEW(pDeviceVulkan->GetAllocator(), xiiGALCommandListVulkan, pDeviceVulkan, this, commandListDescription);

    m_CommandLists.PushBack(pCommandListVulkan);

    xiiStringBuilder sb;
    sb.SetFormat("Command List {}", m_CommandLists.GetCount());
    pCommandListVulkan->SetDebugName(sb);
  }

  pCommandListVulkan->Begin();

  XII_ASSERT_DEV(pCommandListVulkan != nullptr && pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Recording, "The retrieved command list is not begun.");

  return pCommandListVulkan;
}

void xiiGALCommandQueueVulkan::BeginCommandList(xiiGALCommandListVulkan* pCommandListVulkan)
{
  XII_LOCK(m_QueueMutex);

  XII_VERIFY(m_CommandLists.RemoveAndSwap(pCommandListVulkan), "Invalid command list to reset.");

  XII_ASSERT_DEV(!m_CommandLists.Contains(pCommandListVulkan), "Command list duplication error.");

  m_CommandLists.PushBack(pCommandListVulkan);
}

void xiiGALCommandQueueVulkan::ResetCommandList(xiiGALCommandListVulkan* pCommandListVulkan)
{
  XII_LOCK(m_QueueMutex);

  XII_VERIFY(m_CommandLists.RemoveAndSwap(pCommandListVulkan), "Invalid command list to reset.");

  XII_ASSERT_DEV(!m_CommandLists.Contains(pCommandListVulkan), "Command list duplication error.");

  m_CommandLists.PushFront(pCommandListVulkan);
}

void xiiGALCommandQueueVulkan::Flush()
{
}

xiiUInt64 xiiGALCommandQueueVulkan::SubmitCommandList(xiiGALCommandList* pCommandList, bool bReset)
{
  xiiGALDeviceVulkan*      pDeviceVulkan      = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiGALCommandListVulkan* pCommandListVulkan = static_cast<xiiGALCommandListVulkan*>(pCommandList);

  if (pCommandListVulkan->GetRecordingState() == xiiGALCommandList::RecordingState::Recording)
  {
    pCommandListVulkan->End();
  }

  bool bTimelineSemaphoreInUse = false;
  for (const auto& fenceInfo : pCommandListVulkan->m_SignalFences)
  {
    if (!fenceInfo.m_pFenceVulkan->IsTimelineSemaphore())
      continue;

    bTimelineSemaphoreInUse = true;

    pCommandListVulkan->m_vkSignalSemaphores.PushBack(fenceInfo.m_pFenceVulkan->GetVulkanSemaphore());
    pCommandListVulkan->m_vkSignalSemaphoreValues.PushBack(fenceInfo.m_uiWaitValue);
  }

  vk::CommandBuffer vkCommandBuffer = pCommandListVulkan->GetVulkanCommandBuffer();

  vk::SubmitInfo vkSubmitInformation       = {};
  vkSubmitInformation.pNext                = nullptr;
  vkSubmitInformation.waitSemaphoreCount   = pCommandListVulkan->m_vkWaitSemaphores.GetCount();
  vkSubmitInformation.pWaitSemaphores      = pCommandListVulkan->m_vkWaitSemaphores.GetData();
  vkSubmitInformation.pWaitDstStageMask    = pCommandListVulkan->m_vkWaitDestinationStageFlags.GetData();
  vkSubmitInformation.pCommandBuffers      = &vkCommandBuffer;
  vkSubmitInformation.commandBufferCount   = 1U;
  vkSubmitInformation.signalSemaphoreCount = pCommandListVulkan->m_vkSignalSemaphores.GetCount();
  vkSubmitInformation.pSignalSemaphores    = pCommandListVulkan->m_vkSignalSemaphores.GetData();

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

  VK_ASSERT_DEV(m_vkQueue.submit(1U, &vkSubmitInformation, VK_NULL_HANDLE, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

  if (bReset)
  {
    pCommandListVulkan->Reset();
  }

  return 0U;
}

void xiiGALCommandQueueVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);
  xiiStringBuilder    tmp;

  pDeviceVulkan->SetVulkanObjectDebugName(m_vkCommandPool, sName.GetData(tmp));
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
