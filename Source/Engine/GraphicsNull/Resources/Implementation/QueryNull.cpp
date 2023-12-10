#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/QueryNull.h>

xiiGALQueryNull::xiiGALQueryNull(const xiiGALQueryCreationDescription& creationDescription) :
  xiiGALQuery(creationDescription)
{
}

xiiGALQueryNull::~xiiGALQueryNull() = default;

xiiResult xiiGALQueryNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALQueryNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_QueryNull);
