#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/TextureView.h>

// clang-format off

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALUnorderedAccessViewFlags, 1)
  XII_ENUM_CONSTANT(xiiGALUnorderedAccessViewFlags::Unspecified),
  XII_ENUM_CONSTANT(xiiGALUnorderedAccessViewFlags::Read),
  XII_ENUM_CONSTANT(xiiGALUnorderedAccessViewFlags::Write),
  XII_ENUM_CONSTANT(xiiGALUnorderedAccessViewFlags::ReadWrite),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureViewFlags, 1)
  XII_ENUM_CONSTANT(xiiGALTextureViewFlags::None),
  XII_ENUM_CONSTANT(xiiGALTextureViewFlags::AllowMipGeneration),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALTextureComponentSwizzle, 1)
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::Identity),
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::Zero),
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::One),
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::R),
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::G),
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::B),
  XII_ENUM_CONSTANT(xiiGALTextureComponentSwizzle::A),
XII_END_STATIC_REFLECTED_ENUM;

// clang-format on
