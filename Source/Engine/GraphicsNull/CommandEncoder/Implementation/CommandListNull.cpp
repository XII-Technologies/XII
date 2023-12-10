#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/CommandEncoder/CommandListNull.h>
#include <GraphicsNull/Device/DeviceNull.h>

xiiGALCommandListNull::xiiGALCommandListNull(const xiiGALCommandListCreationDescription& creationDescription) :
  xiiGALCommandList(creationDescription)
{
}

xiiGALCommandListNull::~xiiGALCommandListNull() = default;

xiiResult xiiGALCommandListNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALCommandListNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_CommandEncoder_Implementation_CommandListNull);
