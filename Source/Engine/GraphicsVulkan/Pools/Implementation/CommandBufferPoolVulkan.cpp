#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Pools/CommandBufferPoolVulkan.h>

#define XII_GAL_POOL_CHECK_AND_RETURN(code)                                                                                                                                       \
  do                                                                                                                                                                              \
  {                                                                                                                                                                               \
    auto s = (code);                                                                                                                                                              \
    if (static_cast<vk::Result>(s) != vk::Result::eSuccess)                                                                                                                       \
    {                                                                                                                                                                             \
      xiiLog::Error("Vulkan call '{0}' failed with: {1} in {2}:{3}", XII_PP_STRINGIFY(code), vk::to_string(static_cast<vk::Result>(s)).data(), XII_SOURCE_FILE, XII_SOURCE_LINE); \
      return VK_NULL_HANDLE;                                                                                                                                                      \
    }                                                                                                                                                                             \
  } while (false)

xiiGALCommandBufferPoolVulkan::xiiGALCommandBufferPoolVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALQueueInformationVulkan& queueInformation, vk::CommandPoolCreateFlags vkCommandPoolCreateFlags) :
  m_pDeviceVulkan(pDeviceVulkan), m_vkSupportedStageFlags(pDeviceVulkan->GetVulkanLogicalDeviceSupportedStagesFlags(queueInformation.m_uiQueueFamilyIndex)), m_vkSupportedAccessFlags(pDeviceVulkan->GetVulkanLogicalDeviceSupportedAccessFlags(queueInformation.m_uiQueueFamilyIndex))
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

  vk::CommandPoolCreateInfo commandPoolCreationDescription = {};
  commandPoolCreationDescription.pNext                     = nullptr;
  commandPoolCreationDescription.flags                     = vkCommandPoolCreateFlags;
  commandPoolCreationDescription.queueFamilyIndex          = queueInformation.m_uiQueueFamilyIndex;

  VK_ASSERT_DEV(vkLogicalDevice.createCommandPool(&commandPoolCreationDescription, nullptr, &m_vkCommandPool, pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
}

xiiGALCommandBufferPoolVulkan::~xiiGALCommandBufferPoolVulkan()
{
  vk::Device vkLogicalDevice = m_pDeviceVulkan->GetVulkanLogicalDevice();

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_BufferCounter == 0, "{} command buffer(s) have not been returned to the pool. If there are outstanding references to these buffers in release queues, xiiGALCommandBufferPoolVulkan::ReclaimCommandBuffer() will crash when attempting to return the buffer to the pool.", m_BufferCounter);
#endif

  for (vk::CommandBuffer& vkCommandBuffer : m_CommandBuffers)
  {
    vkLogicalDevice.freeCommandBuffers(m_vkCommandPool, 1U, &vkCommandBuffer, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
  }

  vkLogicalDevice.destroyCommandPool(m_vkCommandPool, nullptr, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());
}

void xiiGALCommandBufferPoolVulkan::SetDebugName(xiiStringView sName)
{
  xiiStringBuilder tmp;
  m_pDeviceVulkan->SetVulkanObjectDebugName(m_vkCommandPool, sName.GetData(tmp));
}

vk::CommandBuffer xiiGALCommandBufferPoolVulkan::RequestCommandBuffer(xiiStringView sDebugName)
{
  vk::CommandBuffer vkCommandBuffer = VK_NULL_HANDLE;

  {
    XII_LOCK(m_PoolMutex);

    if (!m_CommandBuffers.IsEmpty())
    {
      vkCommandBuffer = m_CommandBuffers.PeekFront();

      // vk::CommandBufferResetFlagBits::eReleaseResources - specifies that most or all memory resources currently owned by the command buffer should be returned to the parent command pool.
      vkCommandBuffer.reset(vk::CommandBufferResetFlagBits{}, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader());

      m_CommandBuffers.PopFront();
    }
  }

  // If no command buffers were ready to be reused, create a new one.
  if (vkCommandBuffer == VK_NULL_HANDLE)
  {
    vk::CommandBufferAllocateInfo vkCommandBufferAllocateInfo = {};
    vkCommandBufferAllocateInfo.pNext                         = nullptr;
    vkCommandBufferAllocateInfo.commandPool                   = m_vkCommandPool;
    vkCommandBufferAllocateInfo.level                         = vk::CommandBufferLevel::ePrimary;
    vkCommandBufferAllocateInfo.commandBufferCount            = 1U;

    XII_GAL_POOL_CHECK_AND_RETURN(m_pDeviceVulkan->GetVulkanLogicalDevice().allocateCommandBuffers(&vkCommandBufferAllocateInfo, &vkCommandBuffer, m_pDeviceVulkan->GetVulkanDynamicDispatchLoader()));
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  ++m_BufferCounter;
#endif

  if (!sDebugName.IsEmpty())
  {
    xiiStringBuilder tmp;
    m_pDeviceVulkan->SetVulkanObjectDebugName(vkCommandBuffer, sDebugName.GetData(tmp));
  }

  return vkCommandBuffer;
}

void xiiGALCommandBufferPoolVulkan::ReclaimCommandBuffer(vk::CommandBuffer&& vkCommandBuffer)
{
  XII_LOCK(m_PoolMutex);

  m_CommandBuffers.PushBack(vkCommandBuffer);

  vkCommandBuffer = VK_NULL_HANDLE;

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  --m_BufferCounter;
#endif
}

#undef XII_GAL_POOL_CHECK_AND_RETURN
