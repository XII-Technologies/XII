#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/States/RasterizerStateNull.h>

xiiGALRasterizerStateNull::xiiGALRasterizerStateNull(const xiiGALRasterizerStateCreationDescription& creationDescription) :
  xiiGALRasterizerState(creationDescription)
{
}

xiiGALRasterizerStateNull::~xiiGALRasterizerStateNull() = default;

xiiResult xiiGALRasterizerStateNull::InitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

xiiResult xiiGALRasterizerStateNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_RasterizerStateNull);
