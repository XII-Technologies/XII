#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/States/PipelineResourceSignatureNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALPipelineResourceSignatureNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALPipelineResourceSignatureNull::xiiGALPipelineResourceSignatureNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALPipelineResourceSignatureCreationDescription& creationDescription) :
  xiiGALPipelineResourceSignature(pDeviceNull, creationDescription)
{
}

xiiGALPipelineResourceSignatureNull::~xiiGALPipelineResourceSignatureNull() = default;

xiiResult xiiGALPipelineResourceSignatureNull::InitPlatform()
{
  return XII_SUCCESS;
}

bool xiiGALPipelineResourceSignatureNull::IsCompatibleWith(const xiiGALPipelineResourceSignature* pPipelineResourceSignature) const
{
  XII_IGNORE_UNUSED(pPipelineResourceSignature);
  return true;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_States_Implementation_PipelineResourceSignatureNull);
