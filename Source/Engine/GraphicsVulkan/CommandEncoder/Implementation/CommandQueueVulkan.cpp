#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALCommandQueueCreationDescription& creationDescription) :
  xiiGALCommandQueue(pDeviceVulkan, creationDescription), m_vkCommandBuffers(pDeviceVulkan->GetAllocator()), m_pAvailableCommandLists(pDeviceVulkan->GetAllocator()), m_pUsedCommandLists(pDeviceVulkan->GetAllocator())
{
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan() = default;

void xiiGALCommandQueueVulkan::InitializePlatform(xiiUInt32 uiQueueFamilyIndex)
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  m_vkDevice           = pDeviceVulkan->GetVulkanLogicalDevice();
  m_uiQueueFamilyIndex = uiQueueFamilyIndex;

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

void xiiGALCommandQueueVulkan::SetDebugNamePlatform(xiiStringView sName)
{
}

xiiGALCommandList* xiiGALCommandQueueVulkan::BeginCommandList()
{
  xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(m_pDevice);

  XII_ASSERT_DEV(m_vkDevice != nullptr, "");
  XII_ASSERT_DEV(m_vkCommandPool != nullptr, "");

  if (!m_pAvailableCommandLists.IsEmpty())
  {
    xiiGALCommandListVulkan* pCommandList = m_pAvailableCommandLists.GetReverseIterator().Key();

    m_pAvailableCommandLists.Remove(pCommandList);
    m_pUsedCommandLists.Insert(pCommandList);

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

    return pCommandListVulkan;
  }

  return nullptr;
}

xiiUInt64 xiiGALCommandQueueVulkan::Submit(xiiGALCommandList* pCommandList, bool bReset)
{
  return 0U;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
