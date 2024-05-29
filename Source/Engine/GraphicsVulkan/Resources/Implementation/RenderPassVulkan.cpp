#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>

xiiGALRenderPassVulkan::xiiGALRenderPassVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALRenderPass(pDeviceVulkan, creationDescription)
{
}

xiiGALRenderPassVulkan::~xiiGALRenderPassVulkan() = default;

xiiResult xiiGALRenderPassVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);
  
  return XII_FAILURE;
}

xiiResult xiiGALRenderPassVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_RenderPassVulkan);
