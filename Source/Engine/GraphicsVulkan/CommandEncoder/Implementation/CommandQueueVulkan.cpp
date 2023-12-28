#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandQueueVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

xiiGALCommandQueueVulkan::xiiGALCommandQueueVulkan() :
  xiiGALCommandQueue()
{
}

xiiGALCommandQueueVulkan::~xiiGALCommandQueueVulkan() = default;

xiiResult xiiGALCommandQueueVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  XII_ASSERT_NOT_IMPLEMENTED;

  return m_pCommandQueue == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALCommandQueueVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pCommandQueue);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandQueueVulkan);
