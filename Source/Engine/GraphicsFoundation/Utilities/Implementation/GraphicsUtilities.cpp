#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

const xiiGALTextureFormatDescription& xiiGALGraphicsUtilities::GetTextureFormatProperties(xiiEnum<xiiGALTextureFormat> format)
{
  static xiiGALTextureFormatDescription formatDescriptions[xiiGALTextureFormat::ENUM_COUNT];
  static bool                           bIsInitialized = false;

  // Note that this implementation is thread safe. Even if multiple threads call the function, the data may be initialized multiple times but the result will be the same.
  if (!bIsInitialized)
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

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA32Typeless, 4, 4, xiiGALTextureFormatComponentType::Undefined,        true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA32Float,    4, 4, xiiGALTextureFormatComponentType::Float,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA32UInt,     4, 4, xiiGALTextureFormatComponentType::UnsignedInteger,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA32SInt,     4, 4, xiiGALTextureFormatComponentType::SignedInteger,    false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB32Typeless, 4, 3, xiiGALTextureFormatComponentType::Undefined,        true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB32Float,    4, 3, xiiGALTextureFormatComponentType::Float,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB32UInt,     4, 3, xiiGALTextureFormatComponentType::UnsignedInteger,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB32SInt,     4, 3, xiiGALTextureFormatComponentType::SignedInteger,    false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA16Typeless,    2, 4, xiiGALTextureFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA16Float,       2, 4, xiiGALTextureFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA16UNormalized, 2, 4, xiiGALTextureFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA16UInt,        2, 4, xiiGALTextureFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA16SNormalized, 2, 4, xiiGALTextureFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA16SInt,        2, 4, xiiGALTextureFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG32Typeless, 4, 2, xiiGALTextureFormatComponentType::Undefined,        true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG32Float,    4, 2, xiiGALTextureFormatComponentType::Float,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG32UInt,     4, 2, xiiGALTextureFormatComponentType::UnsignedInteger,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG32SInt,     4, 2, xiiGALTextureFormatComponentType::SignedInteger,    false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R32G8X24Typeless,      4, 2, xiiGALTextureFormatComponentType::DepthStencil, true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::D32FloatS8X24UInt,     4, 2, xiiGALTextureFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R32FloatX8X24Typeless, 4, 2, xiiGALTextureFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::X32TypelessG8X24UInt,  4, 2, xiiGALTextureFormatComponentType::DepthStencil, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB10A2Typeless,    4, 1, xiiGALTextureFormatComponentType::Compound, true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB10A2UNormalized, 4, 1, xiiGALTextureFormatComponentType::Compound, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB10A2UInt,        4, 1, xiiGALTextureFormatComponentType::Compound, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG11B10Float,       4, 1, xiiGALTextureFormatComponentType::Compound, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA8Typeless,        1, 4, xiiGALTextureFormatComponentType::Undefined,              true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA8UNormalized,     1, 4, xiiGALTextureFormatComponentType::UnsignedNormalized,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA8UNormalizedSRGB, 1, 4, xiiGALTextureFormatComponentType::UnsignedNormalizedSRGB, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA8UInt,            1, 4, xiiGALTextureFormatComponentType::UnsignedInteger,        false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA8SNormalized,     1, 4, xiiGALTextureFormatComponentType::SignedNormalized,       false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGBA8SInt,            1, 4, xiiGALTextureFormatComponentType::SignedInteger,          false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG16Typeless,    2, 2, xiiGALTextureFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG16Float,       2, 2, xiiGALTextureFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG16UNormalized, 2, 2, xiiGALTextureFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG16UInt,        2, 2, xiiGALTextureFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG16SNormalized, 2, 2, xiiGALTextureFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG16SInt,        2, 2, xiiGALTextureFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R32Typeless,     4, 1, xiiGALTextureFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::D32Float,        4, 1, xiiGALTextureFormatComponentType::Depth,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R32Float,        4, 1, xiiGALTextureFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R32UInt,         4, 1, xiiGALTextureFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R32SInt,         4, 1, xiiGALTextureFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R24G8Typeless,            4, 1, xiiGALTextureFormatComponentType::DepthStencil, true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::D24UNormalizedS8UInt,     4, 1, xiiGALTextureFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R24UNormalizedX8Typeless, 4, 1, xiiGALTextureFormatComponentType::DepthStencil, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::X24TypelessG8UInt,        4, 1, xiiGALTextureFormatComponentType::DepthStencil, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG8Typeless,    1, 2, xiiGALTextureFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG8UNormalized, 1, 2, xiiGALTextureFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG8UInt,        1, 2, xiiGALTextureFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG8SNormalized, 1, 2, xiiGALTextureFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG8SInt,        1, 2, xiiGALTextureFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R16Typeless,    2, 1, xiiGALTextureFormatComponentType::Undefined,          true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R16Float,       2, 1, xiiGALTextureFormatComponentType::Float,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::D16UNormalized, 2, 1, xiiGALTextureFormatComponentType::Depth,              false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R16UNormalized, 2, 1, xiiGALTextureFormatComponentType::UnsignedNormalized, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R16UInt,        2, 1, xiiGALTextureFormatComponentType::UnsignedInteger,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R16SNormalized, 2, 1, xiiGALTextureFormatComponentType::SignedNormalized,   false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R16SInt,        2, 1, xiiGALTextureFormatComponentType::SignedInteger,      false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R8Typeless,    1, 1, xiiGALTextureFormatComponentType::Undefined,           true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R8UNormalized, 1, 1, xiiGALTextureFormatComponentType::UnsignedNormalized,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R8UInt,        1, 1, xiiGALTextureFormatComponentType::UnsignedInteger,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R8SNormalized, 1, 1, xiiGALTextureFormatComponentType::SignedNormalized,    false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R8SInt,        1, 1, xiiGALTextureFormatComponentType::SignedInteger,       false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::A8UNormalized, 1, 1, xiiGALTextureFormatComponentType::UnsignedNormalized,  false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R1UNormalized, 1, 1, xiiGALTextureFormatComponentType::UnsignedNormalized,  false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RGB9E5SharedExponent, 4, 1, xiiGALTextureFormatComponentType::Compound,            false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::RG8BG8UNormalized,    1, 4, xiiGALTextureFormatComponentType::UnsignedNormalized,  false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::GR8GB8UNormalized,    1, 4, xiiGALTextureFormatComponentType::UnsignedNormalized,  false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC1Typeless,        8, 3, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC1UNormalized,     8, 3, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC1UNormalizedSRGB, 8, 3, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC2Typeless,        16, 4, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC2UNormalized,     16, 4, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC2UNormalizedSRGB, 16, 4, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC3Typeless,        16, 4, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC3UNormalized,     16, 4, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC3UNormalizedSRGB, 16, 4, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC4Typeless,    8, 1, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC4UNormalized, 8, 1, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC4SNormalized, 8, 1, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC5Typeless,    16, 2, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC5UNormalized, 16, 2, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC5SNormalized, 16, 2, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::B5G6R5UNormalized,            2, 1, xiiGALTextureFormatComponentType::Compound,               false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::B5G5R5A1UNormalized,          2, 1, xiiGALTextureFormatComponentType::Compound,               false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BGRA8UNormalized,             1, 4, xiiGALTextureFormatComponentType::UnsignedNormalized,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BGRX8UNormalized,             1, 4, xiiGALTextureFormatComponentType::UnsignedNormalized,     false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::R10G10B10XRBiasA2UNormalized, 4, 1, xiiGALTextureFormatComponentType::Compound,               false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BGRA8Typeless,                1, 4, xiiGALTextureFormatComponentType::Undefined,              true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BGRA8UNormalizedSRGB,         1, 4, xiiGALTextureFormatComponentType::UnsignedNormalizedSRGB, false, 1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BGRX8Typeless,                1, 4, xiiGALTextureFormatComponentType::Undefined,              true,  1, 1);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BGRX8UNormalizedSRGB,         1, 4, xiiGALTextureFormatComponentType::UnsignedNormalizedSRGB, false, 1, 1);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC6HTypeless, 16, 3, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC6HUF16,     16, 3, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC6HSF16,     16, 3, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC7Typeless,        16, 4, xiiGALTextureFormatComponentType::Compressed,  true,  4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC7UNormalized,     16, 4, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);
    FILL_TEXTURE_FORMAT_INFO(xiiGALTextureFormat::BC7UNormalizedSRGB, 16, 4, xiiGALTextureFormatComponentType::Compressed,  false, 4, 4);

    // clang-format on

#undef FILL_TEXTURE_FORMAT_INFO

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    for (xiiUInt32 i = xiiGALTextureFormat::Unknown; i < xiiGALTextureFormat::ENUM_COUNT; ++i)
    {
      XII_ASSERT_DEV(formatDescriptions[i].m_Format == static_cast<xiiGALTextureFormat::Enum>(i), "Encountered an uninitialized format.");
    }
#endif

    bIsInitialized = true;
  }

  if (format >= xiiGALTextureFormat::Unknown && format < xiiGALTextureFormat::ENUM_COUNT)
  {
    const auto& description = formatDescriptions[format];
    XII_ASSERT_DEV(description.m_Format == format, "Encountered an unexpected format.");
    return description;
  }

  XII_ASSERT_DEV(false, "Texture format {0} is not in the allowed rage [0, {1}].", format.GetValue(), 0, xiiGALTextureFormat::ENUM_COUNT - 1);
  return formatDescriptions[xiiGALTextureFormat::Unknown];
}

const xiiGALSparseTextureProperties xiiGALGraphicsUtilities::GetSparseTextureProperties(xiiEnum<xiiGALTextureFormat> format, xiiEnum<xiiGALResourceDimension> dimension, xiiUInt32 uiSampleCount)
{
  /// \todo GraphicsFoundation: To be implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALSparseTextureProperties();
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Utilities_Implementation_GraphicsUtilities);
