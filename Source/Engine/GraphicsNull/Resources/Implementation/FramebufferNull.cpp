#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/FramebufferNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALFramebufferNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALFramebufferNull::xiiGALFramebufferNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALFramebufferCreationDescription& creationDescription) :
  xiiGALFramebuffer(pDeviceNull, creationDescription)
{
}

xiiGALFramebufferNull::~xiiGALFramebufferNull() = default;

xiiResult xiiGALFramebufferNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_FramebufferNull);
