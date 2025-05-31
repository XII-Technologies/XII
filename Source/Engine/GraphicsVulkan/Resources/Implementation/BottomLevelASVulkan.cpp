#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelASVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALBottomLevelASVulkan::xiiGALBottomLevelASVulkan(xiiSharedPtr<xiiGALDeviceVulkan> pDeviceVulkan, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(std::move(pDeviceVulkan), creationDescription)
{
}

xiiGALBottomLevelASVulkan::~xiiGALBottomLevelASVulkan() = default;

xiiResult xiiGALBottomLevelASVulkan::InitPlatform()
{
  return XII_FAILURE;
}

void xiiGALBottomLevelASVulkan::SetDebugNamePlatform(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
}

xiiUInt32 xiiGALBottomLevelASVulkan::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
  return xiiUInt32();
}

xiiUInt32 xiiGALBottomLevelASVulkan::GetGeometryIndex(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);
  return xiiUInt32();
}

xiiUInt32 xiiGALBottomLevelASVulkan::GetActualGeometryCount() const
{
  return xiiUInt32();
}

xiiGALScratchBufferSizeDescription xiiGALBottomLevelASVulkan::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription();
}

XII_STATICLINK_FILE(GraphicsVulkan, GraphicsVulkan_Resources_Implementation_BottomLevelASVulkan);
