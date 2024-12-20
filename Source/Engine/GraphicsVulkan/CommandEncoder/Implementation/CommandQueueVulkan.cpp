#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription), m_vkCommandBuffers(pDeviceVulkan->GetAllocator()), m_pAvailableCommandLists(pDeviceVulkan->GetAllocator()), m_pUsedCommandLists(pDeviceVulkan->GetAllocator())
{
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

  m_vkCommandPool = m_vkDevice.createCommandPool(commandPoolCreationDescription, nullptr, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandQueueVulkan::DeInitializePlatform()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_ASSERT_DEV(m_pUsedCommandLists.IsEmpty(), "");
  m_pUsedCommandLists.Clear();

  for (xiiGALCommandListVulkan* pAvailableCommandListVulkan : m_pAvailableCommandLists)
  {
    XII_DELETE(pDeviceVulkan->GetAllocator(), pAvailableCommandListVulkan);
  }
  m_pAvailableCommandLists.Clear();

  for (vk::CommandBuffer& vkCommandBuffer : m_vkCommandBuffers)
  {
    m_vkDevice.freeCommandBuffers(m_vkCommandPool, 1U, &vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }
  m_vkCommandBuffers.Clear();
  m_vkCommandBuffers.Compact();

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
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_ASSERT_DEV(m_vkDevice != nullptr, "");
  XII_ASSERT_DEV(m_vkCommandPool != nullptr, "");

  if (!m_pAvailableCommandLists.IsEmpty())
  {
    xiiGALCommandListVulkan* pCommandList = m_pAvailableCommandLists.GetReverseIterator().Key();

    XII_VERIFY(m_pAvailableCommandLists.Remove(pCommandList), "");
    m_pUsedCommandLists.Insert(pCommandList);

    pCommandList->Begin();

    return pCommandList;
  }
  else
  {
    vk::CommandBuffer vkCommandBuffer;

    vk::CommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
    vkCommandBufferAllocateInfo.pNext                         = nullptr;
    vkCommandBufferAllocateInfo.commandPool                   = m_vkCommandPool;
    vkCommandBufferAllocateInfo.level                         = vk::CommandBufferLevel::ePrimary;
    vkCommandBufferAllocateInfo.commandBufferCount            = 1U;

    VK_ASSERT_DEV(m_vkDevice.allocateCommandBuffers(&vkCommandBufferAllocateInfo, &vkCommandBuffer, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));

    m_vkCommandBuffers.PushBack(vkCommandBuffer);

    xiiGALCommandListCreationDescription commandListDescription = {.m_QueueType = m_Description.m_QueueType};
    xiiGALCommandListVulkan*             pCommandListVulkan     = XII_DEFAULT_NEW(xiiGALCommandListVulkan, pDeviceVulkan, this, commandListDescription, vkCommandBuffer);

    m_pUsedCommandLists.Insert(pCommandListVulkan);

    pCommandListVulkan->Begin();

    return pCommandListVulkan;
  }
}

void xiiGALCommandQueueVulkan::Flush()
{
}

xiiUInt64 xiiGALCommandQueueVulkan::Submit(xiiGALCommandList* pCommandList, bool bReset)
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

    XII_VERIFY(m_pUsedCommandLists.Remove(pCommandListVulkan), "");
    m_pAvailableCommandLists.Insert(pCommandListVulkan);
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
