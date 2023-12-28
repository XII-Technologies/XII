#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Texture.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiGALMiscTextureFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::None),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::GenerateMips),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::Memoryless),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::SparseAlias),
  XII_BITFLAGS_CONSTANT(xiiGALMiscTextureFlags::Subsampled),
XII_END_STATIC_REFLECTED_BITFLAGS;

// clang-format on

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALTexture, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGALTexture::xiiGALTexture(const xiiGALTextureCreationDescription& creationDescription) :
  xiiGALResource(), m_Description(creationDescription)
{
}

xiiGALTexture::~xiiGALTexture() = default;

void xiiGALTexture::CreateDefaultResourceViews()
{
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Resources_Implementation_Texture);
