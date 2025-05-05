#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/DepthStencilStateNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALDepthStencilStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALDepthStencilStateNull::xiiGALDepthStencilStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(pDeviceNull, creationDescription)
{
}

xiiGALDepthStencilStateNull::~xiiGALDepthStencilStateNull() = default;

xiiResult xiiGALDepthStencilStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_DepthStencilStateNull);
