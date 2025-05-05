#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/RenderPassNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRenderPassNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALRenderPassNull::xiiGALRenderPassNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALRenderPassCreationDescription& creationDescription) :
  xiiGALRenderPass(pDeviceNull, creationDescription)
{
}

xiiGALRenderPassNull::~xiiGALRenderPassNull() = default;

xiiResult xiiGALRenderPassNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_RenderPassNull);
