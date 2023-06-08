#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

xiiGALTexture::xiiGALTexture(const xiiGALTextureCreationDescription& Description) :
  xiiGALResource(Description)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(Description.m_szName);
#endif
}

xiiGALTexture::~xiiGALTexture() = default;



XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
