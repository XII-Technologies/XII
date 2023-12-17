#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/SamplerNull.h>

xiiGALSamplerNull::xiiGALSamplerNull(const xiiGALSamplerCreationDescription& creationDescription) :
  xiiGALSampler(creationDescription)
{
}

xiiGALSamplerNull::~xiiGALSamplerNull() = default;

xiiResult xiiGALSamplerNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALSamplerNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_SamplerNull);
