#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/SamplerNull.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALSamplerNull, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiGALSamplerNull::xiiGALSamplerNull(xiiSharedPtr<xiiGALDeviceNull> pDeviceNull, const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(pDeviceNull, creationDescription)
{
}

xiiGALSamplerNull::~xiiGALSamplerNull() = default;

xiiResult xiiGALSamplerNull::InitPlatform()
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_SamplerNull);
