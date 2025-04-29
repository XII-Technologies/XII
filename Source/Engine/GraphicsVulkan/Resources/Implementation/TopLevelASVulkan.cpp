#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/TopLevelASVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTopLevelASVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTopLevelASVulkan::xiiGALTopLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(pDeviceVulkan, creationDescription)
{
}

xiiGALTopLevelASVulkan::~xiiGALTopLevelASVulkan() = default;

xiiResult xiiGALTopLevelASVulkan::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALTopLevelASVulkan::SetDebugNamePlatform(xiiStringView sName)
{
  XII_IGNORE_UNUSED(sName);
}

xiiGALTopLevelASInstanceDescription xiiGALTopLevelASVulkan::GetInstanceDescription(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);

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
