#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/TopLevelASNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTopLevelASNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALTopLevelASNull::xiiGALTopLevelASNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(pDeviceNull, creationDescription)
{
}

xiiGALTopLevelASNull::~xiiGALTopLevelASNull() = default;

xiiResult xiiGALTopLevelASNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiGALTopLevelASInstanceDescription xiiGALTopLevelASNull::GetInstanceDescription(xiiStringView sName) const
{
  XII_IGNORE_UNUSED(sName);

  return xiiGALTopLevelASInstanceDescription();
}

xiiGALTopLevelASBuildDescription xiiGALTopLevelASNull::GetBuildDescription() const
{
  return xiiGALTopLevelASBuildDescription();
}

xiiGALScratchBufferSizeDescription xiiGALTopLevelASNull::GetScratchBufferSizeDescription() const
{
  return xiiGALScratchBufferSizeDescription();
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_TopLevelASNull);
