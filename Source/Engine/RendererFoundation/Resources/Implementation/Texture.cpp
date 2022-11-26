#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

xiiGALTexture::xiiGALTexture(const xiiGALTextureCreationDescription& Description) :
  xiiGALResource(Description)
{
}

xiiGALTexture::~xiiGALTexture() {}



XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
