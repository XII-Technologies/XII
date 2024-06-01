#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/SamplerVulkan.h>

xiiGALSamplerVulkan::xiiGALSamplerVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(pDeviceVulkan, creationDescription)
{
}

xiiGALSamplerVulkan::~xiiGALSamplerVulkan() = default;

xiiResult xiiGALSamplerVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALSamplerVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_SamplerVulkan);
