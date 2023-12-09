#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/FenceNull.h>

xiiGALFenceNull::xiiGALFenceNull(const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(creationDescription)
{
}

xiiGALFenceNull::~xiiGALFenceNull() = default;

xiiResult xiiGALFenceNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALFenceNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_FenceNull);
