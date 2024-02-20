#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/States/PipelineStateNull.h>

xiiGALPipelineStateNull::xiiGALPipelineStateNull(const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(creationDescription)
{
}

xiiGALPipelineStateNull::~xiiGALPipelineStateNull() = default;

xiiResult xiiGALPipelineStateNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineStateNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_PipelineStateNull);
