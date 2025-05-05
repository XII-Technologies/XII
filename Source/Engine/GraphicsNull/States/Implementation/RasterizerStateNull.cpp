#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/RasterizerStateNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALRasterizerStateNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALRasterizerStateNull::xiiGALRasterizerStateNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(pDeviceNull, creationDescription)
{
}

xiiGALRasterizerStateNull::~xiiGALRasterizerStateNull() = default;

xiiResult xiiGALRasterizerStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_RasterizerStateNull);
