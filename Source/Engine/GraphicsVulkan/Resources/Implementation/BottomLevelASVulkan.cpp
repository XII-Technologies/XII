#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>

xiiGALBottomLevelASVulkan::xiiGALBottomLevelASVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(pDeviceVulkan, creationDescription)
{
}

xiiGALBottomLevelASVulkan::~xiiGALBottomLevelASVulkan() = default;

xiiResult xiiGALBottomLevelASVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALBottomLevelASVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BottomLevelASVulkan);
