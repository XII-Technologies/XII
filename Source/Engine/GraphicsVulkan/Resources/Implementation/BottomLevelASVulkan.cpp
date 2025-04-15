#include <GraphicsVulkan/GraphicsVulkanPCH.h>

#include <GraphicsVulkan/Device/DeviceVulkan.h>
#include <GraphicsVulkan/Resources/BottomLevelASVulkan.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelASVulkan, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBottomLevelASVulkan::xiiGALBottomLevelASVulkan(xiiGALDeviceVulkan* pDeviceVulkan, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(pDeviceVulkan, creationDescription)
{
}

xiiGALBottomLevelASVulkan::~xiiGALBottomLevelASVulkan() = default;

xiiResult xiiGALBottomLevelASVulkan::InitPlatform()
{
  // xiiGALDeviceVulkan* pDeviceVulkan = static_cast<xiiGALDeviceVulkan*>(pDevice);

  return XII_FAILURE;
}

xiiResult xiiGALBottomLevelASVulkan::DeInitPlatform()
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_SUCCESS;
}

void xiiGALBottomLevelASVulkan::SetDebugNamePlatform(xiiStringView sName)
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
