#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/BufferNull.h>

xiiGALBufferNull::xiiGALBufferNull(const xiiGALBufferCreationDescription& creationDescription) :
  xiiGALBuffer(creationDescription)
{
}

xiiGALBufferNull::~xiiGALBufferNull() = default;

xiiResult xiiGALBufferNull::InitPlatform(xiiGALDevice* pDevice, const xiiGALBufferData* pInitialData)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALBufferNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_BufferNull);
