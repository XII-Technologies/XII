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

  return XII_FAILURE;
}

xiiResult xiiGALFenceVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

xiiUInt64 xiiGALFenceVulkan::InternalGetCompletedValue()
{
  return xiiUInt64();
}

void xiiGALFenceVulkan::Signal(xiiUInt64 uiValue)
{
}

void xiiGALFenceVulkan::Wait(xiiUInt64 uiValue)
{
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FenceVulkan);
