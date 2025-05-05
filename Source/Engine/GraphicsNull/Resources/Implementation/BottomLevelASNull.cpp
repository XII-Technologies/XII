#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/BottomLevelASNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBottomLevelASNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBottomLevelASNull::xiiGALBottomLevelASNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(pDeviceNull, creationDescription)
{
}

xiiGALBottomLevelASNull::~xiiGALBottomLevelASNull() = default;

xiiResult xiiGALBottomLevelASNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiUInt32 xiiGALBottomLevelASNull::GetGeometryDescriptionIndex(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);

  return xiiUInt32();
}

xiiUInt32 xiiGALBottomLevelASNull::GetGeometryIndex(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);

  return xiiUInt32();
}

xiiUInt32 xiiGALBottomLevelASNull::GetActualGeometryCount() const
{
  return xiiUInt32();
}

xiiGALScratchBufferSizeDescription xiiGALBottomLevelASNull::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription();
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_BottomLevelASNull);
