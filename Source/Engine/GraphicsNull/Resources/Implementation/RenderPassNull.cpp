#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/RenderPassNull.h>

xiiGALRenderPassNull::xiiGALRenderPassNull(const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALRenderPass(creationDescription)
{
}

xiiGALRenderPassNull::~xiiGALRenderPassNull() = default;

xiiResult xiiGALRenderPassNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALRenderPassNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_RenderPassNull);
