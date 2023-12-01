#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGALTexture, xiiNoBase, 1, xiiRTTINoAllocator)
{
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMiscTextureFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::GenerateMips),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::Memoryless),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::SparseAlias),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::Subsampled),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

xiiGALTexture::xiiGALTexture(const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALResource<xiiGALTextureCreationDescription>(creationDescription)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_sDebugName.Assign(creationDescription.m_sName);
#endif
}

xiiGALTexture::~xiiGALTexture() = default;

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Texture);
