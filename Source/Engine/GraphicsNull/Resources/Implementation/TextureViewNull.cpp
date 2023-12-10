#include <GraphicsNull/GraphicsNullPCH.h>

#include <GraphicsNull/Device/DeviceNull.h>
#include <GraphicsNull/Resources/TextureNull.h>
#include <GraphicsNull/Resources/TextureViewNull.h>

xiiGALTextureViewNull::xiiGALTextureViewNull(xiiGALTexture* pTexture, const xiiGALTextureViewCreationDescription& creationDescription) :
  xiiGALTextureView(pTexture, creationDescription)
{
}

xiiGALTextureViewNull::~xiiGALTextureViewNull() = default;

xiiResult xiiGALTextureViewNull::InitPlatform(xiiGALDevice* pDevice)
{
  // xiiGALDeviceNull* pDeviceNull = static_cast<xiiGALDeviceNull*>(pDevice);

  return XII_SUCCESS;
}

xiiResult xiiGALTextureViewNull::DeInitPlatform(xiiGALDevice* pDevice)
{
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsNull, GraphicsNull_Resources_Implementation_TextureViewNull);
