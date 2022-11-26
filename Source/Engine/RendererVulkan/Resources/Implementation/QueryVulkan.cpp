#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>

xiiGALQueryVulkan::xiiGALQueryVulkan(const xiiGALQueryCreationDescription& Description) :
  xiiGALQuery(Description)
{
}

xiiGALQueryVulkan::~xiiGALQueryVulkan() {}

xiiResult xiiGALQueryVulkan::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceVulkan* pVulkanDevice = static_cast<xiiGALDeviceVulkan*>(pDevice);

  if (true)
  {
    return XII_SUCCESS;
  }
  else
  {
    xiiLog::Error("Creation of native Vulkan query failed!");
    return XII_FAILURE;
  }
}

xiiResult xiiGALQueryVulkan::DeInitPlatform(xiiGALDevice* pDevice)
{
  // TODO
  return XII_SUCCESS;
}

void xiiGALQueryVulkan::SetDebugNamePlatform(const char* szName) const
{
  xiiUInt32 uiLength = xiiStringUtils::GetStringElementCount(szName);

  // TODO
}

XII_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_QueryVulkan);
