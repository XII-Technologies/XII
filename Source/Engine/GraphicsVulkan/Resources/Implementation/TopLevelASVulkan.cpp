#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>

xiiGALTopLevelASVulkan::xiiGALTopLevelASVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(pDeviceVulkan, creationDescription)
{
}

xiiGALTopLevelASVulkan::~xiiGALTopLevelASVulkan() = default;

xiiResult xiiGALTopLevelASVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALTopLevelASVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

void xiiGALTopLevelASVulkan::SetDebugNamePlatform(xiiStringView sName)
{
}

xiiGALTopLevelASInstanceDescription xiiGALTopLevelASVulkan::GetInstanceDescription(xiiStringView sName) const
{
  return xiiGALTopLevelASInstanceDescription();
}

xiiGALTopLevelASBuildDescription xiiGALTopLevelASVulkan::GetBuildDescription() const
{
  return xiiGALTopLevelASBuildDescription();
}

xiiGALScratchBufferSizeDescription xiiGALTopLevelASVulkan::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_TopLevelASVulkan);
