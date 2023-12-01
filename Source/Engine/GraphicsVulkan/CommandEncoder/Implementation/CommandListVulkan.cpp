#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/CommandEncoder/CommandListVulkan.h>
#include <GraphicsVulkan/Device/DeviceVulkan.h>

xiiGALCommandListVulkan::xiiGALCommandListVulkan(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(creationDescription)
{
}

xiiGALCommandListVulkan::~xiiGALCommandListVulkan() = default;

xiiResult xiiGALCommandListVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  XII_ASSERT_NOT_IMPLEMENTED;

  return m_pCommandList == nullptr ? XII_FAILURE : XII_SUCCESS;
}

xiiResult xiiGALCommandListVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  XII_GAL_DILIGENT_PTR_RELEASE(m_pCommandList);

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_CommandEncoder_Implementation_CommandListVulkan);
