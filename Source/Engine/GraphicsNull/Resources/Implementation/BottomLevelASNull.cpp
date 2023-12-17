#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/BottomLevelASNull.h>

xiiGALBottomLevelASNull::xiiGALBottomLevelASNull(const xiiGALBottomLevelASCreationDescription& creationDescription) :
  xiiGALBottomLevelAS(creationDescription)
{
}

xiiGALBottomLevelASNull::~xiiGALBottomLevelASNull() = default;

xiiResult xiiGALBottomLevelASNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALBottomLevelASNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_BottomLevelASNull);
