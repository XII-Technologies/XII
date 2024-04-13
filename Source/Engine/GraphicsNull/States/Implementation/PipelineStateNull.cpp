#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/PipelineStateNull.h>

xiiGALPipelineStateNull::xiiGALPipelineStateNull(xiiGALDeviceNull* pDeviceNull, const xiiGALPipelineStateCreationDescription& creationDescription) :
  xiiGALPipelineState(pDeviceNull, creationDescription)
{
}

xiiGALPipelineStateNull::~xiiGALPipelineStateNull() = default;

xiiResult xiiGALPipelineStateNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiResult xiiGALPipelineStateNull::DeInitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_PipelineStateNull);
