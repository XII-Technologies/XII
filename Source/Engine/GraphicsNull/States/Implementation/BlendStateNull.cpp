#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/BlendStateNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALBlendStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALBlendStateNull::xiiGALBlendStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(pDeviceNull, creationDescription)
{
}

xiiGALBlendStateNull::~xiiGALBlendStateNull() = default;

xiiResult xiiGALBlendStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_BlendStateNull);
