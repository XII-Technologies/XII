#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BufferVulkan.h>

xiiGALBufferVulkan::xiiGALBufferVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(pDeviceVulkan, creationDescription)
{
}

xiiGALBufferVulkan::~xiiGALBufferVulkan() = default;

xiiResult xiiGALBufferVulkan::InitPlatform(const xiiGALBufferData* pInitialData)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALBufferVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BufferVulkan);
