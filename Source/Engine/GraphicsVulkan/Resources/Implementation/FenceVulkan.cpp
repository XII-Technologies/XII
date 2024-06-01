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
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALFenceVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FenceVulkan);
