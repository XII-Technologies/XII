#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/PipelineResourceSignatureNull.h>

xiiGALPipelineResourceSignatureNull::xiiGALPipelineResourceSignatureNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(pDeviceNull, creationDescription)
{
}

xiiGALPipelineResourceSignatureNull::~xiiGALPipelineResourceSignatureNull() = default;

xiiResult xiiGALPipelineResourceSignatureNull::InitPlatform()
{
  return XII_SUCCESS;
}

xiiResult xiiGALPipelineResourceSignatureNull::DeInitPlatform()
{
  return XII_SUCCESS;
}

bool xiiGALPipelineResourceSignatureNull::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  XII_IGNORE_UNUSED(pPipelineResourceSignature);
  return true;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_PipelineResourceSignatureNull);
