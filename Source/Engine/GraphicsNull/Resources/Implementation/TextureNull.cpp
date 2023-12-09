#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/TextureNull.h>

xiiGALTextureNull::xiiGALTextureNull(const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALTexture(creationDescription)
{
}

xiiGALTextureNull::~xiiGALTextureNull() = default;

xiiResult xiiGALTextureNull::InitPlatform(xiiGALDevice* pDevice, const xiiGALTextureData* pInitialData)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_TextureNull);
