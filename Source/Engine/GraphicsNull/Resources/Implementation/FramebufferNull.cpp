#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/FramebufferNull.h>
#include <GraphicsNull/Resources/RenderPassNull.h>
#include <GraphicsNull/Resources/TextureNull.h>
#include <GraphicsNull/Resources/TextureViewNull.h>

xiiGALFramebufferNull::xiiGALFramebufferNull(const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(creationDescription)
{
}

xiiGALFramebufferNull::~xiiGALFramebufferNull() = default;

xiiResult xiiGALFramebufferNull::InitPlatform(xiiGALDevice* pDevice)
{
  xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALFramebufferNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_FramebufferNull);
