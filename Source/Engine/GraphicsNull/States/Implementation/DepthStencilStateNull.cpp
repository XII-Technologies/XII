#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/States/DepthStencilStateNull.h>

xiiGALDepthStencilStateNull::xiiGALDepthStencilStateNull(const xiiGALDepthStencilStateCreationDescription& creationDescription) :
  xiiGALDepthStencilState(creationDescription)
{
}

xiiGALDepthStencilStateNull::~xiiGALDepthStencilStateNull() = default;

xiiResult xiiGALDepthStencilStateNull::InitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

xiiResult xiiGALDepthStencilStateNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_DepthStencilStateNull);
