/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Containers/Blob.h>
#include <Foundation/Math/Size.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

bool xiiGALTextureUtilities::IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping)
{
  return ((mapping.m_R == xiiGALTextureComponentSwizzle::Identity || mapping.m_R == xiiGALTextureComponentSwizzle::R) &&
          (mapping.m_G == xiiGALTextureComponentSwizzle::Identity || mapping.m_G == xiiGALTextureComponentSwizzle::G) &&
          (mapping.m_B == xiiGALTextureComponentSwizzle::Identity || mapping.m_B == xiiGALTextureComponentSwizzle::B) &&
          (mapping.m_A == xiiGALTextureComponentSwizzle::Identity || mapping.m_A == xiiGALTextureComponentSwizzle::A));
}

const xiiGALResourceFormatDescription& xiiGALTextureUtilities::GetResourceFormatProperties(xiiEnum<xiiGALResourceFormat> format)
{
  static xiiGALResourceFormatDescription formatDescriptions[xiiGALResourceFormat::ENUM_COUNT];
  static bool                            s_bIsInitialized = false;

  // Note that this implementation is thread safe. Even if multiple threads call the function, the data may be initialized multiple times but the result will be the same.
  if (!s_bIsInitialized)
  {
#define FILL_TEXTURE_FORMAT_INFO(format, componentSize, componentCount, componentType, isTypeless, blockWidth, blockHeight) \
  formatDescriptions[format].m_Format           = format;                                                                   \
  formatDescriptions[format].m_uiComponentSize  = componentSize;                                                            \
  formatDescriptions[format].m_uiComponentCount = componentCount;                                                           \
  formatDescriptions[format].m_ComponentType    = componentType;                                                            \
  formatDescriptions[format].m_bIsTypeless      = isTypeless;                                                               \
  formatDescriptions[format].m_uiBlockWidth     = blockWidth;                                                               \
  formatDescriptions[format].m_uiBlockHeight    = blockHeight

    // clang-format off

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA32Typeless, 4, 4, xiiGALResourceFormatComponentType::Undefined,        true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA32Float,    4, 4, xiiGALResourceFormatComponentType::Float,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA32UInt,     4, 4, xiiGALResourceFormatComponentType::UnsignedInteger,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA32SInt,     4, 4, xiiGALResourceFormatComponentType::SignedInteger,    false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB32Typeless, 4, 3, xiiGALResourceFormatComponentType::Undefined,        true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB32Float,    4, 3, xiiGALResourceFormatComponentType::Float,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB32UInt,     4, 3, xiiGALResourceFormatComponentType::UnsignedInteger,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB32SInt,     4, 3, xiiGALResourceFormatComponentType::SignedInteger,    false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA16Typeless,    2, 4, xiiGALResourceFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA16Float,       2, 4, xiiGALResourceFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA16UNormalized, 2, 4, xiiGALResourceFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA16UInt,        2, 4, xiiGALResourceFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA16SNormalized, 2, 4, xiiGALResourceFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA16SInt,        2, 4, xiiGALResourceFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG32Typeless, 4, 2, xiiGALResourceFormatComponentType::Undefined,        true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG32Float,    4, 2, xiiGALResourceFormatComponentType::Float,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG32UInt,     4, 2, xiiGALResourceFormatComponentType::UnsignedInteger,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG32SInt,     4, 2, xiiGALResourceFormatComponentType::SignedInteger,    false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R32G8X24Typeless,      4, 2, xiiGALResourceFormatComponentType::DepthStencil, true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::D32FloatS8X24UInt,     4, 2, xiiGALResourceFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R32FloatX8X24Typeless, 4, 2, xiiGALResourceFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::X32TypelessG8X24UInt,  4, 2, xiiGALResourceFormatComponentType::DepthStencil, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB10A2Typeless,    4, 1, xiiGALResourceFormatComponentType::Compound, true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB10A2UNormalized, 4, 1, xiiGALResourceFormatComponentType::Compound, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB10A2UInt,        4, 1, xiiGALResourceFormatComponentType::Compound, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG11B10Float,       4, 1, xiiGALResourceFormatComponentType::Compound, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA8Typeless,        1, 4, xiiGALResourceFormatComponentType::Undefined,              true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA8UNormalized,     1, 4, xiiGALResourceFormatComponentType::UnsignedNormalized,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA8UNormalizedSRGB, 1, 4, xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA8UInt,            1, 4, xiiGALResourceFormatComponentType::UnsignedInteger,        false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA8SNormalized,     1, 4, xiiGALResourceFormatComponentType::SignedNormalized,       false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGBA8SInt,            1, 4, xiiGALResourceFormatComponentType::SignedInteger,          false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG16Typeless,    2, 2, xiiGALResourceFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG16Float,       2, 2, xiiGALResourceFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG16UNormalized, 2, 2, xiiGALResourceFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG16UInt,        2, 2, xiiGALResourceFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG16SNormalized, 2, 2, xiiGALResourceFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG16SInt,        2, 2, xiiGALResourceFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R32Typeless,     4, 1, xiiGALResourceFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::D32Float,        4, 1, xiiGALResourceFormatComponentType::Depth,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R32Float,        4, 1, xiiGALResourceFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R32UInt,         4, 1, xiiGALResourceFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R32SInt,         4, 1, xiiGALResourceFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R24G8Typeless,            4, 1, xiiGALResourceFormatComponentType::DepthStencil, true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::D24UNormalizedS8UInt,     4, 1, xiiGALResourceFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R24UNormalizedX8Typeless, 4, 1, xiiGALResourceFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::X24TypelessG8UInt,        4, 1, xiiGALResourceFormatComponentType::DepthStencil, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG8Typeless,    1, 2, xiiGALResourceFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG8UNormalized, 1, 2, xiiGALResourceFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG8UInt,        1, 2, xiiGALResourceFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG8SNormalized, 1, 2, xiiGALResourceFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG8SInt,        1, 2, xiiGALResourceFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R16Typeless,    2, 1, xiiGALResourceFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R16Float,       2, 1, xiiGALResourceFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::D16UNormalized, 2, 1, xiiGALResourceFormatComponentType::Depth,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R16UNormalized, 2, 1, xiiGALResourceFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R16UInt,        2, 1, xiiGALResourceFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R16SNormalized, 2, 1, xiiGALResourceFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R16SInt,        2, 1, xiiGALResourceFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R8Typeless,    1, 1, xiiGALResourceFormatComponentType::Undefined,           true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R8UNormalized, 1, 1, xiiGALResourceFormatComponentType::UnsignedNormalized,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R8UInt,        1, 1, xiiGALResourceFormatComponentType::UnsignedInteger,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R8SNormalized, 1, 1, xiiGALResourceFormatComponentType::SignedNormalized,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R8SInt,        1, 1, xiiGALResourceFormatComponentType::SignedInteger,       false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::A8UNormalized, 1, 1, xiiGALResourceFormatComponentType::UnsignedNormalized,  false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R1UNormalized, 1, 1, xiiGALResourceFormatComponentType::UnsignedNormalized,  false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RGB9E5SharedExponent, 4, 1, xiiGALResourceFormatComponentType::Compound,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::RG8BG8UNormalized,    1, 4, xiiGALResourceFormatComponentType::UnsignedNormalized,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::GR8GB8UNormalized,    1, 4, xiiGALResourceFormatComponentType::UnsignedNormalized,  false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC1Typeless,        8, 3, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC1UNormalized,     8, 3, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC1UNormalizedSRGB, 8, 3, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC2Typeless,        16, 4, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC2UNormalized,     16, 4, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC2UNormalizedSRGB, 16, 4, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC3Typeless,        16, 4, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC3UNormalized,     16, 4, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC3UNormalizedSRGB, 16, 4, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC4Typeless,    8, 1, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC4UNormalized, 8, 1, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC4SNormalized, 8, 1, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC5Typeless,    16, 2, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC5UNormalized, 16, 2, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC5SNormalized, 16, 2, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::B5G6R5UNormalized,            2, 1, xiiGALResourceFormatComponentType::Compound,               false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::B5G5R5A1UNormalized,          2, 1, xiiGALResourceFormatComponentType::Compound,               false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BGRA8UNormalized,             1, 4, xiiGALResourceFormatComponentType::UnsignedNormalized,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BGRX8UNormalized,             1, 4, xiiGALResourceFormatComponentType::UnsignedNormalized,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized, 4, 1, xiiGALResourceFormatComponentType::Compound,               false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BGRA8Typeless,                1, 4, xiiGALResourceFormatComponentType::Undefined,              true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BGRA8UNormalizedSRGB,         1, 4, xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BGRX8Typeless,                1, 4, xiiGALResourceFormatComponentType::Undefined,              true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BGRX8UNormalizedSRGB,         1, 4, xiiGALResourceFormatComponentType::UnsignedNormalizedSRGB, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC6HTypeless, 16, 3, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC6HUF16,     16, 3, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC6HSF16,     16, 3, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC7Typeless,        16, 4, xiiGALResourceFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC7UNormalized,     16, 4, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::BC7UNormalizedSRGB, 16, 4, xiiGALResourceFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::YUY2, 1, 4, xiiGALResourceFormatComponentType::UnsignedInteger, false, 2, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALResourceFormat::AYUV, 1, 4, xiiGALResourceFormatComponentType::UnsignedInteger, false, 1, 1);

    // clang-format on

#undef FILL_TEXTURE_FORMAT_INFO

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    for (xiiUInt32 i = xiiGALResourceFormat::Unknown; i < xiiGALResourceFormat::ENUM_COUNT; ++i)
    {
      if (!xiiGALResourceFormat::IsMultiplanar(static_cast<xiiGALResourceFormat::Enum>(i)))
      {
        XII_ASSERT_DEV(formatDescriptions[i].m_Format == static_cast<xiiGALResourceFormat::Enum>(i), "Encountered an uninitialized single-plane format.");
      }
      else
      {
        XII_ASSERT_DEV(formatDescriptions[i].m_Format == xiiGALResourceFormat::Unknown, "Multi-planar formats must not be initialized in xiiGALResourceFormatDescription.");
      }
    }
#endif

    s_bIsInitialized = true;
  }

  if (format >= xiiGALResourceFormat::Unknown && format < xiiGALResourceFormat::ENUM_COUNT)
  {
    // Multi-planar formats must NOT be looked up in this descriptor.
    if (xiiGALResourceFormat::IsMultiplanar(format))
    {
      XII_ASSERT_DEV(false, "Multi-planar format {} cannot be retrieved from xiiGALResourceFormatDescription. Use the multi-plane descriptor instead.", xiiArgEnum(format));
    }

    // Single-plane formats must be initialized correctly.
    const xiiGALResourceFormatDescription& description = formatDescriptions[format];
    XII_ASSERT_DEV(description.m_Format == format, "Encountered an unexpected or uninitialized single-plane format {0}.", xiiArgEnum(format));

    return description;
  }

  // Out-of-range format.
  XII_ASSERT_DEV(false, "Texture format {0} is not in the allowed range [0, {1}].", xiiArgEnum(format), xiiGALResourceFormat::ENUM_COUNT - 1);

  return formatDescriptions[xiiGALResourceFormat::Unknown];
}

const xiiGALMultiPlanarFormatDescription& xiiGALTextureUtilities::GetMultiPlanarFormatProperties(xiiEnum<xiiGALResourceFormat> format)
{
  static xiiGALMultiPlanarFormatDescription formatDescriptions[xiiGALResourceFormat::ENUM_COUNT];
  static bool                               s_bIsInitialized = false;

  // Note that this implementation is thread safe. Even if multiple threads call the function, the data may be initialized multiple times but the result will be the same.
  if (!s_bIsInitialized)
  {
    {
      xiiGALMultiPlanarFormatDescription& description = formatDescriptions[xiiGALResourceFormat::NV12];
      description.m_Format                            = xiiGALResourceFormat::NV12;

      // Plane 0: Y (full resolution).
      xiiGALMultiPlanarFormatDescription::Plane& plane0 = description.m_Planes.ExpandAndGetRef();
      plane0.m_SubFormat                                = xiiGALResourceFormat::R8UNormalized;
      plane0.m_uiBytesPerElement                        = 1;
      plane0.m_fWidthFactor                             = 1.0f;
      plane0.m_fHeightFactor                            = 1.0f;

      // Plane 1: UV (half resolution, interleaved).
      xiiGALMultiPlanarFormatDescription::Plane& plane1 = description.m_Planes.ExpandAndGetRef();
      plane1.m_SubFormat                                = xiiGALResourceFormat::RG8UNormalized;
      plane1.m_uiBytesPerElement                        = 2;
      plane1.m_fWidthFactor                             = 0.5f;
      plane1.m_fHeightFactor                            = 0.5f;
    }
    {
      xiiGALMultiPlanarFormatDescription& description = formatDescriptions[xiiGALResourceFormat::P010];
      description.m_Format                            = xiiGALResourceFormat::P010;

      // Plane 0: Y (full resolution, 10-bit stored in 16-bit).
      xiiGALMultiPlanarFormatDescription::Plane& plane0 = description.m_Planes.ExpandAndGetRef();
      plane0.m_SubFormat                                = xiiGALResourceFormat::R16UNormalized;
      plane0.m_uiBytesPerElement                        = 2;
      plane0.m_fWidthFactor                             = 1.0f;
      plane0.m_fHeightFactor                            = 1.0f;

      // Plane 1: UV (half resolution, 10-bit stored in 16-bit per channel).
      xiiGALMultiPlanarFormatDescription::Plane& plane1 = description.m_Planes.ExpandAndGetRef();
      plane1.m_SubFormat                                = xiiGALResourceFormat::RG16UNormalized;
      plane1.m_uiBytesPerElement                        = 4;
      plane1.m_fWidthFactor                             = 0.5f;
      plane1.m_fHeightFactor                            = 0.5f;
    }
    {
      xiiGALMultiPlanarFormatDescription& description = formatDescriptions[xiiGALResourceFormat::P016];
      description.m_Format                            = xiiGALResourceFormat::P016;

      // Plane 0: Y (full resolution, 16-bit).
      xiiGALMultiPlanarFormatDescription::Plane& plane0 = description.m_Planes.ExpandAndGetRef();
      plane0.m_SubFormat                                = xiiGALResourceFormat::R16UNormalized;
      plane0.m_uiBytesPerElement                        = 2;
      plane0.m_fWidthFactor                             = 1.0f;
      plane0.m_fHeightFactor                            = 1.0f;

      // Plane 1: UV (half resolution, 16-bit per channel).
      xiiGALMultiPlanarFormatDescription::Plane& plane1 = description.m_Planes.ExpandAndGetRef();
      plane1.m_SubFormat                                = xiiGALResourceFormat::RG16UNormalized;
      plane1.m_uiBytesPerElement                        = 4;
      plane1.m_fWidthFactor                             = 0.5f;
      plane1.m_fHeightFactor                            = 0.5f;
    }
    {
      xiiGALMultiPlanarFormatDescription& description = formatDescriptions[xiiGALResourceFormat::P216];
      description.m_Format                            = xiiGALResourceFormat::P216;

      // Plane 0: Y (full resolution, 16-bit).
      xiiGALMultiPlanarFormatDescription::Plane& plane0 = description.m_Planes.ExpandAndGetRef();
      plane0.m_SubFormat                                = xiiGALResourceFormat::R16UNormalized;
      plane0.m_uiBytesPerElement                        = 2;
      plane0.m_fWidthFactor                             = 1.0f;
      plane0.m_fHeightFactor                            = 1.0f;

      // Plane 1: UV (half horizontal resolution, full vertical resolution).
      xiiGALMultiPlanarFormatDescription::Plane& plane1 = description.m_Planes.ExpandAndGetRef();
      plane1.m_SubFormat                                = xiiGALResourceFormat::RG16UNormalized;
      plane1.m_uiBytesPerElement                        = 4;
      plane1.m_fWidthFactor                             = 0.5f;
      plane1.m_fHeightFactor                            = 1.0f;
    }
    {
      xiiGALMultiPlanarFormatDescription& description = formatDescriptions[xiiGALResourceFormat::P416];
      description.m_Format                            = xiiGALResourceFormat::P416;

      // Plane 0: Y (full resolution, 16-bit).
      xiiGALMultiPlanarFormatDescription::Plane& plane0 = description.m_Planes.ExpandAndGetRef();
      plane0.m_SubFormat                                = xiiGALResourceFormat::R16UNormalized;
      plane0.m_uiBytesPerElement                        = 2;
      plane0.m_fWidthFactor                             = 1.0f;
      plane0.m_fHeightFactor                            = 1.0f;

      // Plane 1: U (full resolution, 16-bit).
      xiiGALMultiPlanarFormatDescription::Plane& plane1 = description.m_Planes.ExpandAndGetRef();
      plane1.m_SubFormat                                = xiiGALResourceFormat::R16UNormalized;
      plane1.m_uiBytesPerElement                        = 2;
      plane1.m_fWidthFactor                             = 1.0f;
      plane1.m_fHeightFactor                            = 1.0f;

      // Plane 2: V (full resolution, 16-bit).
      xiiGALMultiPlanarFormatDescription::Plane& plane2 = description.m_Planes.ExpandAndGetRef();
      plane2.m_SubFormat                                = xiiGALResourceFormat::R16UNormalized;
      plane2.m_uiBytesPerElement                        = 2;
      plane2.m_fWidthFactor                             = 1.0f;
      plane2.m_fHeightFactor                            = 1.0f;
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    for (xiiUInt32 i = xiiGALResourceFormat::Unknown; i < xiiGALResourceFormat::ENUM_COUNT; ++i)
    {
      if (xiiGALResourceFormat::IsMultiplanar(static_cast<xiiGALResourceFormat::Enum>(i)))
      {
        XII_ASSERT_DEV(formatDescriptions[i].m_Format == static_cast<xiiGALResourceFormat::Enum>(i), "Encountered an uninitialized multi-plane format.");
        XII_ASSERT_DEV(formatDescriptions[i].IsValid(), "Encountered an invalid multi-plane format ({}).", xiiArgEnum(formatDescriptions[i].m_Format));
      }
      else
      {
        XII_ASSERT_DEV(formatDescriptions[i].m_Format == xiiGALResourceFormat::Unknown, "Single-plane formats must not be initialized in xiiGALMultiPlanarFormatDescription.");
      }
    }
#endif

    s_bIsInitialized = true;
  }

  if (format >= xiiGALResourceFormat::Unknown && format < xiiGALResourceFormat::ENUM_COUNT)
  {
    // Single-plane formats must NOT be looked up in this descriptor.
    if (!xiiGALResourceFormat::IsMultiplanar(format))
    {
      XII_ASSERT_DEV(false, "Single-plane format {} cannot be retrieved from xiiGALMultiPlanarFormatDescription. Use the single-plane descriptor instead.", xiiArgEnum(format));
    }

    // Multi-planar formats must be initialized correctly.
    const xiiGALMultiPlanarFormatDescription& description = formatDescriptions[format];
    XII_ASSERT_DEV(description.m_Format == format, "Encountered an unexpected or uninitialized multi-planar format {0}.", xiiArgEnum(format));

    return description;
  }

  // Out-of-range format.
  XII_ASSERT_DEV(false, "Texture format {0} is not in the allowed range [0, {1}].", xiiArgEnum(format), xiiGALResourceFormat::ENUM_COUNT - 1);

  return formatDescriptions[xiiGALResourceFormat::Unknown];
}

class ResourceFormatToViewFormatConverter
{
public:
  ResourceFormatToViewFormatConverter()
  {
    m_ViewFormats.SetCountUninitialized(xiiGALResourceFormat::ENUM_COUNT);

#define INIT_TEX_VIEW_FORMAT_INFO(textureFormat, SRVFormat, RTVFormat, DSVFormat, UAVFormat)                     \
  {                                                                                                              \
    m_ViewFormats[textureFormat][xiiGALTextureViewType::ShaderResource]       = xiiGALResourceFormat::SRVFormat; \
    m_ViewFormats[textureFormat][xiiGALTextureViewType::RenderTarget]         = xiiGALResourceFormat::RTVFormat; \
    m_ViewFormats[textureFormat][xiiGALTextureViewType::DepthStencil]         = xiiGALResourceFormat::DSVFormat; \
    m_ViewFormats[textureFormat][xiiGALTextureViewType::ReadOnlyDepthStencil] = xiiGALResourceFormat::DSVFormat; \
    m_ViewFormats[textureFormat][xiiGALTextureViewType::UnorderedAccess]      = xiiGALResourceFormat::UAVFormat; \
    m_ViewFormats[textureFormat][xiiGALTextureViewType::ShadingRate]          = xiiGALResourceFormat::Unknown;   \
  }
    static_assert(xiiGALTextureViewType::ENUM_COUNT == 6U, "Please handle the new view type above, if necessary");

    // clang-format off
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::Unknown,                  Unknown, Unknown, Unknown, Unknown);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA32Typeless,           RGBA32Float, RGBA32Float, Unknown, RGBA32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA32Float,              RGBA32Float, RGBA32Float, Unknown, RGBA32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA32UInt,               RGBA32UInt,  RGBA32UInt,  Unknown, RGBA32UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA32SInt,               RGBA32SInt,  RGBA32SInt,  Unknown, RGBA32SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB32Typeless,            RGB32Float, RGB32Float, Unknown, RGB32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB32Float,               RGB32Float, RGB32Float, Unknown, RGB32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB32UInt,                RGB32UInt,  RGB32UInt,  Unknown, RGB32UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB32SInt,                RGB32SInt,  RGB32SInt,  Unknown, RGB32SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA16Typeless,           RGBA16Float,       RGBA16Float,       Unknown, RGBA16Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA16Float,              RGBA16Float,       RGBA16Float,       Unknown, RGBA16Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA16UNormalized,        RGBA16UNormalized, RGBA16UNormalized, Unknown, RGBA16UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA16UInt,               RGBA16UInt,        RGBA16UInt,        Unknown, RGBA16UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA16SNormalized,        RGBA16SNormalized, RGBA16SNormalized, Unknown, RGBA16SNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA16SInt,               RGBA16SInt,        RGBA16SInt,        Unknown, RGBA16SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG32Typeless,             RG32Float, RG32Float, Unknown, RG32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG32Float,                RG32Float, RG32Float, Unknown, RG32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG32UInt,                 RG32UInt,  RG32UInt,  Unknown, RG32UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG32SInt,                 RG32SInt,  RG32SInt,  Unknown, RG32SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R32G8X24Typeless,         R32FloatX8X24Typeless, Unknown, D32FloatS8X24UInt, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::D32FloatS8X24UInt,        R32FloatX8X24Typeless, Unknown, D32FloatS8X24UInt, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R32FloatX8X24Typeless,    R32FloatX8X24Typeless, Unknown, D32FloatS8X24UInt, R32FloatX8X24Typeless);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::X32TypelessG8X24UInt,     X32TypelessG8X24UInt,  Unknown, D32FloatS8X24UInt, X32TypelessG8X24UInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB10A2Typeless,          RGB10A2UNormalized, RGB10A2UNormalized, Unknown, RGB10A2UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB10A2UNormalized,       RGB10A2UNormalized, RGB10A2UNormalized, Unknown, RGB10A2UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB10A2UInt,              RGB10A2UInt,        RGB10A2UInt,        Unknown, RGB10A2UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG11B10Float,             RG11B10Float,       RG11B10Float,       Unknown, RG11B10Float);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA8Typeless,            RGBA8UNormalizedSRGB, RGBA8UNormalizedSRGB, Unknown, RGBA8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA8UNormalized,         RGBA8UNormalized,     RGBA8UNormalized,     Unknown, RGBA8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA8UNormalizedSRGB,     RGBA8UNormalizedSRGB, RGBA8UNormalizedSRGB, Unknown, RGBA8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA8UInt,                RGBA8UInt,            RGBA8UInt,            Unknown, RGBA8UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA8SNormalized,         RGBA8SNormalized,     RGBA8SNormalized,     Unknown, RGBA8SNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGBA8SInt,                RGBA8SInt,            RGBA8SInt,            Unknown, RGBA8SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG16Typeless,             RG16Float,       RG16Float,       Unknown, RG16Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG16Float,                RG16Float,       RG16Float,       Unknown, RG16Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG16UNormalized,          RG16UNormalized, RG16UNormalized, Unknown, RG16UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG16UInt,                 RG16UInt,        RG16UInt,        Unknown, RG16UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG16SNormalized,          RG16SNormalized, RG16SNormalized, Unknown, RG16SNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG16SInt,                 RG16SInt,        RG16SInt,        Unknown, RG16SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R32Typeless,              R32Float, R32Float, D32Float, R32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::D32Float,                 R32Float, R32Float, D32Float, R32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R32Float,                 R32Float, R32Float, D32Float, R32Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R32UInt,                  R32UInt,  R32UInt,  Unknown,  R32UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R32SInt,                  R32SInt,  R32SInt,  Unknown,  R32SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R24G8Typeless,            R24UNormalizedX8Typeless, Unknown, D24UNormalizedS8UInt, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::D24UNormalizedS8UInt,     R24UNormalizedX8Typeless, Unknown, D24UNormalizedS8UInt, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R24UNormalizedX8Typeless, R24UNormalizedX8Typeless, Unknown, D24UNormalizedS8UInt, R24UNormalizedX8Typeless);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::X24TypelessG8UInt,        X24TypelessG8UInt,        Unknown, D24UNormalizedS8UInt, X24TypelessG8UInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG8Typeless,              RG8UNormalized, RG8UNormalized, Unknown, RG8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG8UNormalized,           RG8UNormalized, RG8UNormalized, Unknown, RG8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG8UInt,                  RG8UInt,        RG8UInt,        Unknown, RG8UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG8SNormalized,           RG8SNormalized, RG8SNormalized, Unknown, RG8SNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG8SInt,                  RG8SInt,        RG8SInt,        Unknown, RG8SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R16Typeless,              R16Float,       R16Float,       Unknown,        R16Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R16Float,                 R16Float,       R16Float,       Unknown,        R16Float);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::D16UNormalized,           R16UNormalized, R16UNormalized, D16UNormalized, R16UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R16UNormalized,           R16UNormalized, R16UNormalized, D16UNormalized, R16UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R16UInt,                  R16UInt,        R16UInt,        Unknown,        R16UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R16SNormalized,           R16SNormalized, R16SNormalized, Unknown,        R16SNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R16SInt,                  R16SInt,        R16SInt,        Unknown,        R16SInt);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R8Typeless,               R8UNormalized, R8UNormalized, Unknown, R8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R8UNormalized,            R8UNormalized, R8UNormalized, Unknown, R8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R8UInt,                   R8UInt,        R8UInt,        Unknown, R8UInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R8SNormalized,            R8SNormalized, R8SNormalized, Unknown, R8SNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R8SInt,                   R8SInt,        R8SInt,        Unknown, R8SInt);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::A8UNormalized,            A8UNormalized, A8UNormalized, Unknown, A8UNormalized);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R1UNormalized,            R1UNormalized, R1UNormalized, Unknown, R1UNormalized);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RGB9E5SharedExponent,     RGB9E5SharedExponent, RGB9E5SharedExponent, Unknown, RGB9E5SharedExponent);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::RG8BG8UNormalized,        RG8BG8UNormalized,    RG8BG8UNormalized,    Unknown, RG8BG8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::GR8GB8UNormalized,        GR8GB8UNormalized,    GR8GB8UNormalized,    Unknown, GR8GB8UNormalized);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC1Typeless,              BC1UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC1UNormalized,           BC1UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC1UNormalizedSRGB,       BC1UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC2Typeless,              BC2UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC2UNormalized,           BC2UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC2UNormalizedSRGB,       BC2UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC3Typeless,              BC3UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC3UNormalized,           BC3UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC3UNormalizedSRGB,       BC3UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC4Typeless,              BC4UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC4UNormalized,           BC4UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC4SNormalized,           BC4SNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC5Typeless,              BC5UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC5UNormalized,           BC5UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC5SNormalized,           BC5SNormalized,     Unknown, Unknown, Unknown);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::B5G6R5UNormalized,        B5G6R5UNormalized,   B5G6R5UNormalized,     Unknown, B5G6R5UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::B5G5R5A1UNormalized,      B5G5R5A1UNormalized, B5G5R5A1UNormalized,   Unknown, B5G5R5A1UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BGRA8UNormalized,         BGRA8UNormalized,    BGRA8UNormalized,      Unknown, BGRA8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BGRX8UNormalized,         BGRX8UNormalized,    BGRX8UNormalized,      Unknown, BGRX8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized, R10G10B10XRBiasA2UNormalized, Unknown,  Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BGRA8Typeless,            BGRA8UNormalizedSRGB, BGRA8UNormalizedSRGB, Unknown, BGRA8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BGRA8UNormalizedSRGB,     BGRA8UNormalizedSRGB, BGRA8UNormalizedSRGB, Unknown, BGRA8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BGRX8Typeless,            BGRX8UNormalizedSRGB, BGRX8UNormalizedSRGB, Unknown, BGRX8UNormalized);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BGRX8UNormalizedSRGB,     BGRX8UNormalizedSRGB, BGRX8UNormalizedSRGB, Unknown, BGRX8UNormalized);

    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC6HTypeless,             BC6HUF16,           Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC6HUF16,                 BC6HUF16,           Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC6HSF16,                 BC6HSF16,           Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC7Typeless,              BC7UNormalizedSRGB, Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC7UNormalized,           BC7UNormalized,     Unknown, Unknown, Unknown);
    INIT_TEX_VIEW_FORMAT_INFO(xiiGALResourceFormat::BC7UNormalizedSRGB,       BC7UNormalizedSRGB, Unknown, Unknown, Unknown);
    // clang-format on
#undef INIT_TVIEW_FORMAT_INFO

    m_ViewFormats[xiiGALResourceFormat::R8UInt][xiiGALTextureViewType::ShadingRate]         = xiiGALResourceFormat::R8UInt;
    m_ViewFormats[xiiGALResourceFormat::RG8UNormalized][xiiGALTextureViewType::ShadingRate] = xiiGALResourceFormat::RG8UNormalized;
  }

  xiiGALResourceFormat::Enum GetViewFormat(xiiGALResourceFormat::Enum format, xiiGALTextureViewType::Enum viewType, xiiUInt32 uiBindFlags)
  {
    XII_ASSERT_DEV(viewType > xiiGALTextureViewType::Undefined && viewType < xiiGALTextureViewType::ENUM_COUNT, "Unexpected texture view type.");
    XII_ASSERT_DEV(format >= xiiGALResourceFormat::Unknown && format < xiiGALResourceFormat::ENUM_COUNT, "Unknown texture format.");

    switch (format)
    {
      case xiiGALResourceFormat::R16Typeless:
      {
        if (uiBindFlags & xiiGALBindFlags::DepthStencil)
        {
          switch (viewType)
          {
            case xiiGALTextureViewType::ShaderResource:
            case xiiGALTextureViewType::RenderTarget:
            case xiiGALTextureViewType::UnorderedAccess:
              return xiiGALResourceFormat::R16UNormalized;

            case xiiGALTextureViewType::DepthStencil:
            case xiiGALTextureViewType::ReadOnlyDepthStencil:
              return xiiGALResourceFormat::D16UNormalized;

            case xiiGALTextureViewType::ShadingRate:
              return xiiGALResourceFormat::Unknown;

            default:
              XII_REPORT_FAILURE("Unexpected texture view type {}.", xiiArgEnum(xiiEnum<xiiGALTextureViewType>(viewType)));
              return xiiGALResourceFormat::Unknown;
          }
          static_assert(xiiGALTextureViewType::ENUM_COUNT == 6U, "Please handle the new view type in the switch above, if necessary.");
        }
        [[fallthrough]];
      }

      default: // Nothing to do.
        break;
    }

    return m_ViewFormats[format][viewType];
  }

private:
  xiiStaticArray<xiiGALResourceFormat::Enum[xiiGALTextureViewType::ENUM_COUNT], xiiGALResourceFormat::ENUM_COUNT> m_ViewFormats;
};

xiiUInt64 xiiGALTextureUtilities::GetStagingTextureLocationOffset(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiArraySlice, xiiUInt32 uiMipLevel, xiiUInt32 uiAlignment, xiiUInt32 uiLocationX, xiiUInt32 uiLocationY, xiiUInt32 uiLocationZ)
{
  XII_ASSERT_DEV(textureDescription.m_uiMipLevels > 0 && textureDescription.m_uiArraySizeOrDepth > 0 && textureDescription.m_Size.HasNonZeroArea() && textureDescription.m_Format != xiiGALResourceFormat::Unknown, "");
  XII_ASSERT_DEV((uiArraySlice < textureDescription.m_uiArraySizeOrDepth && uiMipLevel < textureDescription.m_uiMipLevels) || (uiArraySlice == textureDescription.m_uiArraySizeOrDepth && uiMipLevel == 0), "");

  xiiUInt64 uiOffset = 0;
  if (uiArraySlice > 0)
  {
    xiiUInt64 uiArraySliceSize = 0;

    for (xiiUInt32 uiMip = 0; uiMip < textureDescription.m_uiMipLevels; ++uiMip)
    {
      xiiGALMipLevelProperties mipLevelProperties = GetMipLevelProperties(textureDescription, uiMip);

      uiArraySliceSize += xiiMemoryUtils::AlignSize(mipLevelProperties.m_uiMipSize, xiiUInt64{uiAlignment});
    }

    uiOffset = uiArraySliceSize;

    if (textureDescription.IsArray())
    {
      uiOffset *= uiArraySlice;
    }
  }

  for (xiiUInt32 uiMip = 0; uiMip < uiMipLevel; ++uiMip)
  {
    xiiGALMipLevelProperties mipLevelProperties = GetMipLevelProperties(textureDescription, uiMip);

    uiOffset += xiiMemoryUtils::AlignSize(mipLevelProperties.m_uiMipSize, xiiUInt64{uiAlignment});
  }

  if (uiArraySlice == textureDescription.m_uiArraySizeOrDepth)
  {
    XII_ASSERT_DEV(uiLocationX == 0 && uiLocationY == 0 && uiLocationZ == 0, "Staging buffer size is requested: location must be (0, 0, 0).");
  }
  else if (uiLocationX != 0 || uiLocationY != 0 || uiLocationZ != 0)
  {
    const xiiGALResourceFormatDescription& formatProperties   = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);
    xiiGALMipLevelProperties               mipLevelProperties = GetMipLevelProperties(textureDescription, uiMipLevel);

    XII_ASSERT_DEV(uiLocationX < mipLevelProperties.m_LogicalSize.width && uiLocationY < mipLevelProperties.m_LogicalSize.height && uiLocationZ < mipLevelProperties.m_uiDepth, "The specified location is out of range.");

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
    {
      XII_ASSERT_DEV((uiLocationX % formatProperties.m_uiBlockWidth) == 0 && (uiLocationY % formatProperties.m_uiBlockHeight) == 0, "For compressed resource formats, location must be a multiple of the compressed block size.");
    }
#endif

    // For compressed-block formats, RowSize is the size of one compressed row.
    // For non-compressed formats, BlockHeight is 1.
    uiOffset += (uiLocationZ * mipLevelProperties.m_StorageSize.height + uiLocationY) / formatProperties.m_uiBlockHeight * mipLevelProperties.m_uiRowSize;

    // For non-compressed formats, BlockWidth is 1.
    uiOffset += xiiUInt64{uiLocationX / formatProperties.m_uiBlockWidth} * formatProperties.GetElementSize();

    // Note: This addressing complies with how Vulkan (as well as OpenGL/GLES and Metal) address textures when copying data to/from buffers:
    //       address of (x,y,z) = bufferOffset + (((z * imageHeight) + y) * rowLength + x) * texelBlockSize; (18.4.1)
  }

  return uiOffset;
}

xiiGALBufferToTextureCopyDescription xiiGALTextureUtilities::GetBufferToTextureCopyDescription(xiiGALResourceFormat::Enum format, const xiiBoundingBoxU32& region, xiiUInt32 uiRowStrideAlignment)
{
  xiiGALBufferToTextureCopyDescription bufferToTextureCopyDescription;

  const xiiGALResourceFormatDescription& formatProperties = GetResourceFormatProperties(format);

  XII_ASSERT_DEV(region.IsValid(), "");

  const xiiUInt32 uiUpdateRegionWidth  = region.m_vMax.x - region.m_vMin.x;
  const xiiUInt32 uiUpdateRegionHeight = region.m_vMax.y - region.m_vMin.y;
  const xiiUInt32 uiUpdateRegionDepth  = region.m_vMax.z - region.m_vMin.z;

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    // Align region update size by the block size.

    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockWidth), "Format block width must be a power of 2.");
    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockHeight), "Format block height must be a power of 2.");

    const xiiUInt32 uiBlockAlignedRegionWidth  = xiiMemoryUtils::AlignSize(uiUpdateRegionWidth, xiiUInt32{formatProperties.m_uiBlockWidth});
    const xiiUInt32 uiBlockAlignedRegionHeight = xiiMemoryUtils::AlignSize(uiUpdateRegionHeight, xiiUInt32{formatProperties.m_uiBlockHeight});

    bufferToTextureCopyDescription.m_uiRowSize  = xiiUInt64{uiBlockAlignedRegionWidth} / xiiUInt32{formatProperties.m_uiBlockWidth} * xiiUInt32{formatProperties.m_uiComponentSize};
    bufferToTextureCopyDescription.m_uiRowCount = uiBlockAlignedRegionHeight / formatProperties.m_uiBlockHeight;
  }
  else
  {
    bufferToTextureCopyDescription.m_uiRowSize  = xiiUInt64{uiUpdateRegionWidth} * xiiUInt32{formatProperties.m_uiComponentSize} * xiiUInt32{formatProperties.m_uiComponentCount};
    bufferToTextureCopyDescription.m_uiRowCount = uiUpdateRegionHeight;
  }

  XII_ASSERT_DEV(xiiMath::IsPowerOf2(uiRowStrideAlignment), "");

  bufferToTextureCopyDescription.m_uiRowStride = xiiMemoryUtils::AlignSize(bufferToTextureCopyDescription.m_uiRowSize, xiiUInt64{uiRowStrideAlignment});

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    bufferToTextureCopyDescription.m_uiRowStrideInTexels = static_cast<xiiUInt32>(bufferToTextureCopyDescription.m_uiRowStride / xiiUInt64{formatProperties.m_uiComponentSize} * xiiUInt64{formatProperties.m_uiBlockWidth});
  }
  else
  {
    bufferToTextureCopyDescription.m_uiRowStrideInTexels = static_cast<xiiUInt32>(bufferToTextureCopyDescription.m_uiRowStride / (xiiUInt64{formatProperties.m_uiComponentSize} * xiiUInt64{formatProperties.m_uiComponentCount}));
  }

  bufferToTextureCopyDescription.m_uiDepthStride = static_cast<xiiUInt32>(bufferToTextureCopyDescription.m_uiRowCount * bufferToTextureCopyDescription.m_uiRowStride);
  bufferToTextureCopyDescription.m_uiMemorySize  = uiUpdateRegionDepth * bufferToTextureCopyDescription.m_uiDepthStride;
  bufferToTextureCopyDescription.m_Region        = region;

  return bufferToTextureCopyDescription;
}

void xiiGALTextureUtilities::CopyTextureSubresource(const xiiGALTextureSubResourceData& sourceSubresource, xiiUInt32 uiRowCount, xiiUInt32 uiDepthSliceCount, xiiUInt64 uiRowSize, void* pDestinationData, xiiUInt64 uiDestinationRowStride, xiiUInt64 uiDestinationDepthStride)
{
  XII_ASSERT_DEV(pDestinationData != nullptr, "Destination data pointer must not be null.");
  XII_ASSERT_DEV(sourceSubresource.m_uiStride >= uiRowSize, "Source data row stride ({}) is smaller than the row size ({}).", sourceSubresource.m_uiStride, uiRowSize);
  XII_ASSERT_DEV(sourceSubresource.m_uiDepthStride >= uiRowSize, "Destination data row stride ({}) is smaller than the row size ({}).", uiDestinationDepthStride, uiRowSize);

  for (xiiUInt32 uiZ = 0; uiZ < uiDepthSliceCount; ++uiZ)
  {
    const xiiUInt8* pSourceSlice      = xiiMemoryUtils::AddByteOffset(sourceSubresource.m_pData.GetPtr(), sourceSubresource.m_uiDepthStride * uiZ);
    void*           pDestinationSlice = xiiMemoryUtils::AddByteOffset(pDestinationData, uiDestinationDepthStride * uiZ);

    for (xiiUInt32 uiY = 0; uiY < uiRowCount; ++uiY)
    {
      memcpy(xiiMemoryUtils::AddByteOffset(pDestinationSlice, uiDestinationRowStride * uiY), xiiMemoryUtils::AddByteOffset(pSourceSlice, sourceSubresource.m_uiStride * uiY), uiRowSize);
    }
  }
}

xiiEnum<xiiGALResourceFormat> xiiGALTextureUtilities::GetDefaultTextureViewFormat(xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALTextureViewType> viewType, xiiBitflags<xiiGALBindFlags> bindFlags)
{
  static ResourceFormatToViewFormatConverter formatConverter;
  return formatConverter.GetViewFormat(format, viewType, bindFlags.GetValue());
}

const xiiGALSparseTextureProperties xiiGALTextureUtilities::GetSparseTextureProperties(xiiEnum<xiiGALResourceFormat> format, xiiEnum<xiiGALResourceDimension> dimension, xiiUInt32 uiSampleCount)
{
  /// \todo GraphicsFoundation: To be implemented.

  XII_IGNORE_UNUSED(format);
  XII_IGNORE_UNUSED(dimension);
  XII_IGNORE_UNUSED(uiSampleCount);

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALSparseTextureProperties();
}

xiiVec3U32 xiiGALTextureUtilities::GetMipLevelSize(xiiUInt32 uiMipLevelSize, const xiiGALTextureCreationDescription& textureDescription)
{
  xiiVec3U32 size = {textureDescription.m_Size.width, textureDescription.m_Size.height, textureDescription.m_uiArraySizeOrDepth};
  size.x          = xiiMath::Max(1U, size.x >> uiMipLevelSize);
  size.y          = xiiMath::Max(1U, size.y >> uiMipLevelSize);
  if (textureDescription.Is3D())
  {
    size.z = xiiMath::Max(1U, size.z >> uiMipLevelSize);
  }
  return size;
}

xiiUInt32 xiiGALTextureUtilities::GetMipSize(xiiUInt32 uiSize, xiiUInt32 uiMipLevel)
{
  for (xiiUInt32 i = 0; i < uiMipLevel; ++i)
  {
    uiSize = uiSize / 2;
  }
  return xiiMath::Max(1U, uiSize);
}

xiiGALMipLevelProperties xiiGALTextureUtilities::GetMipLevelProperties(const xiiGALTextureCreationDescription& textureDescription, xiiUInt32 uiMipLevel)
{
  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(textureDescription.m_Format);

  xiiGALMipLevelProperties mipLevelProperties;
  mipLevelProperties.m_LogicalSize.width  = xiiMath::Max(textureDescription.m_Size.width >> uiMipLevel, 1U);
  mipLevelProperties.m_LogicalSize.height = xiiMath::Max(textureDescription.m_Size.height >> uiMipLevel, 1U);
  mipLevelProperties.m_uiDepth            = textureDescription.Is3D() ? xiiMath::Max(textureDescription.m_uiArraySizeOrDepth >> uiMipLevel, 1U) : 1U;

  if (formatProperties.m_ComponentType == xiiGALResourceFormatComponentType::Compressed)
  {
    XII_ASSERT_DEV(formatProperties.m_uiBlockWidth > 1 && formatProperties.m_uiBlockHeight > 1, "The compressed format requires a block width and height greater than 1.");
    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockWidth), "Compressed block width is expected to be a power of 2.");
    XII_ASSERT_DEV(xiiMath::IsPowerOf2(formatProperties.m_uiBlockHeight), "Compressed block height is expected to be a power of 2.");

    // For block-compression formats, all parameters are still specified in texels rather than compressed texel blocks (18.4.1).
    mipLevelProperties.m_StorageSize.width  = xiiMemoryUtils::AlignSize(mipLevelProperties.m_LogicalSize.width, xiiUInt32{formatProperties.m_uiBlockWidth});
    mipLevelProperties.m_StorageSize.height = xiiMemoryUtils::AlignSize(mipLevelProperties.m_LogicalSize.height, xiiUInt32{formatProperties.m_uiBlockHeight});
    mipLevelProperties.m_uiRowSize          = xiiUInt64{mipLevelProperties.m_StorageSize.width} / xiiUInt32{formatProperties.m_uiBlockWidth} * xiiUInt32{formatProperties.m_uiComponentSize}; // Component size is the block size.
    mipLevelProperties.m_uiDepthSliceSize   = mipLevelProperties.m_StorageSize.height / xiiUInt32{formatProperties.m_uiBlockHeight} * mipLevelProperties.m_uiRowSize;
    mipLevelProperties.m_uiMipSize          = mipLevelProperties.m_uiDepthSliceSize * mipLevelProperties.m_uiDepth;
  }
  else
  {
    mipLevelProperties.m_StorageSize      = mipLevelProperties.m_LogicalSize;
    mipLevelProperties.m_uiRowSize        = xiiUInt64{mipLevelProperties.m_StorageSize.width} * xiiUInt32{formatProperties.m_uiComponentSize} * xiiUInt32{formatProperties.m_uiComponentCount};
    mipLevelProperties.m_uiDepthSliceSize = mipLevelProperties.m_uiRowSize * mipLevelProperties.m_StorageSize.height;
    mipLevelProperties.m_uiMipSize        = mipLevelProperties.m_uiDepthSliceSize * mipLevelProperties.m_uiDepth;
  }

  return mipLevelProperties;
}

xiiUInt32 xiiGALTextureUtilities::GetMipLevelCount(const xiiGALTextureCreationDescription& textureDescription)
{
  const xiiUInt32 uiDepth = textureDescription.Is3D() ? textureDescription.m_uiArraySizeOrDepth : 1U;
  return xiiMath::Log2i(xiiMath::Max(textureDescription.m_Size.width, textureDescription.m_Size.height, uiDepth, 1U)) + 1U;
}

xiiGALTextureCreationDescription xiiGALTextureUtilities::GetDefaultTexture1DDescription() noexcept
{
  return xiiGALTextureCreationDescription{
    .m_Type               = xiiGALResourceDimension::Texture1D,
    .m_Size               = xiiSizeU32(0, 0),
    .m_uiArraySizeOrDepth = 1U,
    .m_Format             = xiiGALResourceFormat::Unknown,
    .m_uiMipLevels        = 1U,
    .m_uiSampleCount      = xiiGALSampleCount::OneSample,
    .m_BindFlags          = xiiGALBindFlags::ShaderResource,
    .m_CPUAccessFlags     = xiiGALCPUAccessFlag::None,
    .m_MiscFlags          = xiiGALMiscTextureFlags::None,
  };
}

xiiGALTextureCreationDescription xiiGALTextureUtilities::GetDefaultTexture2DDescription() noexcept
{
  return xiiGALTextureCreationDescription{
    .m_Type               = xiiGALResourceDimension::Texture2D,
    .m_Size               = xiiSizeU32(0, 0),
    .m_uiArraySizeOrDepth = 1U,
    .m_Format             = xiiGALResourceFormat::Unknown,
    .m_uiMipLevels        = 1U,
    .m_uiSampleCount      = xiiGALSampleCount::OneSample,
    .m_BindFlags          = xiiGALBindFlags::ShaderResource,
    .m_Usage              = xiiGALResourceUsage::Immutable,
    .m_CPUAccessFlags     = xiiGALCPUAccessFlag::None,
    .m_MiscFlags          = xiiGALMiscTextureFlags::None,
  };
}

xiiGALTextureCreationDescription xiiGALTextureUtilities::GetDefaultTexture3DDescription() noexcept
{
  return xiiGALTextureCreationDescription{
    .m_Type               = xiiGALResourceDimension::Texture3D,
    .m_Size               = xiiSizeU32(0, 0),
    .m_uiArraySizeOrDepth = 1U,
    .m_Format             = xiiGALResourceFormat::Unknown,
    .m_uiMipLevels        = 1U,
    .m_uiSampleCount      = xiiGALSampleCount::OneSample,
    .m_BindFlags          = xiiGALBindFlags::ShaderResource,
    .m_Usage              = xiiGALResourceUsage::Immutable,
    .m_CPUAccessFlags     = xiiGALCPUAccessFlag::None,
    .m_MiscFlags          = xiiGALMiscTextureFlags::None,
  };
}

xiiGALTextureCreationDescription xiiGALTextureUtilities::GetDefaultTextureCubeDescription() noexcept
{
  return xiiGALTextureCreationDescription{
    .m_Type               = xiiGALResourceDimension::TextureCube,
    .m_Size               = xiiSizeU32(0, 0),
    .m_uiArraySizeOrDepth = 6U,
    .m_Format             = xiiGALResourceFormat::Unknown,
    .m_uiMipLevels        = 1U,
    .m_uiSampleCount      = xiiGALSampleCount::OneSample,
    .m_BindFlags          = xiiGALBindFlags::ShaderResource,
    .m_Usage              = xiiGALResourceUsage::Immutable,
    .m_CPUAccessFlags     = xiiGALCPUAccessFlag::None,
    .m_MiscFlags          = xiiGALMiscTextureFlags::None,
  };
}

xiiGALTextureData xiiGALTextureUtilities::GetZeroMemoryInitialData(const xiiGALTextureCreationDescription description, xiiHybridArray<xiiGALTextureSubResourceData, 2U>& out_subresourceData, xiiDynamicArray<xiiUInt8>& out_Data)
{
  out_subresourceData.Clear();
  out_Data.Clear();

  const xiiUInt32 uiTotalSubResources = description.m_uiMipLevels * description.m_uiArraySizeOrDepth;
  out_subresourceData.Reserve(uiTotalSubResources);

  // First compute total size needed.
  xiiUInt64 uiTotalSize = 0;
  for (xiiUInt32 uiArraySlice = 0; uiArraySlice < description.m_uiArraySizeOrDepth; ++uiArraySlice)
  {
    for (xiiUInt32 uiMipLevel = 0; uiMipLevel < description.m_uiMipLevels; ++uiMipLevel)
    {
      const xiiGALMipLevelProperties mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(description, uiMipLevel);

      uiTotalSize += mipLevelProperties.m_uiMipSize;
    }
  }

  // Allocate once.
  out_Data.SetCountUninitialized(static_cast<xiiUInt32>(uiTotalSize));
  memset(out_Data.GetData(), 0, static_cast<size_t>(uiTotalSize));

  // Now assign subresource pointers into the already allocated buffer.
  xiiUInt64 uiCurrentOffset = 0;
  for (xiiUInt32 uiArraySlice = 0; uiArraySlice < description.m_uiArraySizeOrDepth; ++uiArraySlice)
  {
    for (xiiUInt32 uiMipLevel = 0; uiMipLevel < description.m_uiMipLevels; ++uiMipLevel)
    {
      const xiiGALMipLevelProperties mipLevelProperties = xiiGALTextureUtilities::GetMipLevelProperties(description, uiMipLevel);
      const xiiUInt64                uiSubResourceSize  = mipLevelProperties.m_uiMipSize;

      xiiGALTextureSubResourceData& subResourceData = out_subresourceData.ExpandAndGetRef();
      subResourceData.m_pData                       = xiiMakeByteArrayPtr(out_Data.GetData() + uiCurrentOffset, static_cast<xiiUInt32>(uiSubResourceSize));
      subResourceData.m_uiStride                    = mipLevelProperties.m_uiRowSize;
      subResourceData.m_uiDepthStride               = mipLevelProperties.m_uiDepthSliceSize;

      uiCurrentOffset += uiSubResourceSize;
    }
  }

  return xiiGALTextureData{out_subresourceData};
}

void xiiGALTextureUtilities::CopySubresourceToMemory(const xiiGALTextureCreationDescription& description, const xiiGALMappedTextureSubresource& subresourceData, const xiiGALTextureMipLevelData& mipLevelData, xiiArrayPtr<xiiUInt8> pTargetData, xiiUInt32 uiTargetRowStride)
{
  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(description.m_Format);

  if (subresourceData.m_uiStride == uiTargetRowStride)
  {
    const xiiUInt32 uiMemorySize = formatProperties.GetElementSize() * xiiGALTextureUtilities::GetMipSize(description.m_Size.width, mipLevelData.m_uiMipLevel) * xiiGALTextureUtilities::GetMipSize(description.m_Size.height, mipLevelData.m_uiMipLevel);

    XII_ASSERT_DEBUG(uiMemorySize <= pTargetData.GetCount(), "");

    memcpy(pTargetData.GetPtr(), subresourceData.m_pData, uiMemorySize);
  }
  else
  {
    // Copy row by row.
    const xiiUInt32 uiHeight = xiiGALTextureUtilities::GetMipSize(description.m_Size.height, mipLevelData.m_uiMipLevel);

    for (xiiUInt32 y = 0; y < uiHeight; ++y)
    {
      const xiiUInt8* pSource      = xiiMemoryUtils::AddByteOffset(static_cast<xiiUInt8*>(subresourceData.m_pData), y * subresourceData.m_uiStride);
      xiiUInt8*       pDestination = xiiMemoryUtils::AddByteOffset(pTargetData.GetPtr(), y * uiTargetRowStride);
      const xiiUInt32 uiCopySize   = formatProperties.GetElementSize() * xiiGALTextureUtilities::GetMipSize(description.m_Size.width, mipLevelData.m_uiMipLevel);

      XII_ASSERT_DEBUG(pDestination + uiCopySize <= pTargetData.GetEndPtr(), "");

      memcpy(pDestination, pSource, uiCopySize);
    }
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Utilities_Implementation_TextureUtilities);
