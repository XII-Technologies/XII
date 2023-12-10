#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/BufferNull.h>
#include <GraphicsNull/Resources/BufferViewNull.h>

xiiGALBufferViewNull::xiiGALBufferViewNull(xiiGALBuffer* pBuffer, const xiiGALBufferViewCreationDescription& creationDescription) :
  xiiGALBufferView(pBuffer, creationDescription)
{
}

xiiGALBufferViewNull::~xiiGALBufferViewNull() = default;

xiiResult xiiGALBufferViewNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALBufferViewNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_BufferViewNull);
