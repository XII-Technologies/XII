#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/TopLevelASNull.h>

xiiGALTopLevelASNull::xiiGALTopLevelASNull(const xiiGALTopLevelASCreationDescription& creationDescription) :
  xiiGALTopLevelAS(creationDescription)
{
}

xiiGALTopLevelASNull::~xiiGALTopLevelASNull() = default;

xiiResult xiiGALTopLevelASNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALTopLevelASNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_TopLevelASNull);
