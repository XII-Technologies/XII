#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/FenceNull.h>

xiiGALFenceNull::xiiGALFenceNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALFenceCreationDescription& creationDescription) :
  xiiGALFence(pDeviceNull, creationDescription)
{
}

xiiUInt64 xiiGALFenceNull::GetCompletedValue()
{
  return xiiUInt64();
}

xiiGALFenceNull::~xiiGALFenceNull() = default;

xiiResult xiiGALFenceNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiResult xiiGALFenceNull::DeInitPlatform()
{
  return XII_SUCCESS;
}

void xiiGALFenceNull::Signal(xiiUInt64 uiValue)
{
  XII_IGNORE_UNUSED(uiValue);
}

void xiiGALFenceNull::Wait(xiiUInt64 uiValue)
{
  XII_IGNORE_UNUSED(uiValue);
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_FenceNull);
