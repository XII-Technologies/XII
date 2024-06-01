#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/FramebufferVulkan.h>
#include <GraphicsVulkan/Resources/RenderPassVulkan.h>
#include <GraphicsVulkan/Resources/TextureViewVulkan.h>
#include <GraphicsVulkan/Resources/TextureVulkan.h>

xiiGALFramebufferVulkan::xiiGALFramebufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(pDeviceVulkan, creationDescription)
{
}

xiiGALFramebufferVulkan::~xiiGALFramebufferVulkan() = default;

xiiResult xiiGALFramebufferVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALFramebufferVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_FramebufferVulkan);
