#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

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

void xiiGALCommandQueueVulkan::TransitionImageLayout(xiiGALTextureVulkan* pTextureVulkan, vk::ImageLayout imageLayout)
{
}

void xiiGALCommandQueueVulkan::AddWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlags pipelineFlags)
{
  XII_ASSERT_DEV(semaphore != VK_NULL_HANDLE, "");

  m_vkWaitSemaphores.PushBack(semaphore);
  m_vkWaitDestinationStageFlags.PushBack(pipelineFlags);
  m_vkWaitSemaphoreValues.PushBack(0); // Ignored for binary semaphore.
}

void xiiGALCommandQueueVulkan::AddSignalSemaphore(vk::Semaphore semaphore)
{
  XII_ASSERT_DEV(semaphore != VK_NULL_HANDLE, "");

  m_vkSignalSemaphores.PushBack(semaphore);
  m_vkSignalSemaphoreValues.PushBack(0); // Ignored for binary semaphore.
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

  vk::CommandBuffer vkCommandBuffer = pCommandListVulkan->GetVulkanCommandBuffer();

  vk::SubmitInfo vkSubmitInformation     = {};
  vkSubmitInformation.pCommandBuffers    = &vkCommandBuffer;
  vkSubmitInformation.commandBufferCount = 1U;

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
