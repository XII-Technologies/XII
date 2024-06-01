#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/QueryVulkan.h>

xiiGALQueryVulkan::xiiGALQueryVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(pDeviceVulkan, creationDescription)
{
}

xiiGALQueryVulkan::~xiiGALQueryVulkan() = default;

xiiResult xiiGALQueryVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALQueryVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_QueryVulkan);
