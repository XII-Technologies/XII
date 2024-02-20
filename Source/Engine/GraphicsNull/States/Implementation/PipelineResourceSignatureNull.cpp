#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/States/PipelineResourceSignatureNull.h>

xiiGALPipelineResourceSignatureNull::xiiGALPipelineResourceSignatureNull(const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(creationDescription)
{
}

xiiGALPipelineResourceSignatureNull::~xiiGALPipelineResourceSignatureNull() = default;

xiiResult xiiGALPipelineResourceSignatureNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALPipelineResourceSignatureNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_PipelineResourceSignatureNull);
