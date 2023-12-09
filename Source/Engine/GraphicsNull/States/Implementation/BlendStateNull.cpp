#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/States/BlendStateNull.h>

xiiGALBlendStateNull::xiiGALBlendStateNull(const xiiGALBlendStateCreationDescription& creationDescription) :
  xiiGALBlendState(creationDescription)
{
}

xiiGALBlendStateNull::~xiiGALBlendStateNull() = default;

xiiResult xiiGALBlendStateNull::InitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

xiiResult xiiGALBlendStateNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_BlendStateNull);
