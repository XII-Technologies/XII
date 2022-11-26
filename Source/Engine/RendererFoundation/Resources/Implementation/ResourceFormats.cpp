#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceFormats.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiGALResourceFormat, 1)
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAFloat),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAUInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBFloat),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBUInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::B5G6R5UNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BGRAUByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BGRAUByteNormalizedsRGB),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAHalf),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAUShort),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAUShortNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAShort),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAShortNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGFloat),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGUInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGB10A2UInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGB10A2UIntNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RG11B10Float),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAUByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAUByteNormalizedsRGB),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAUByte),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGBAByte),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGHalf),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGUShort),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGUShortNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGShort),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGShortNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGUByte),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGUByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGByte),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RGByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::DFloat),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RFloat),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RUInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RInt),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RHalf),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RUShort),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RUShortNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RShort),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RShortNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RUByte),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RUByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RByte),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::RByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::AUByteNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::D16),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::D24S8),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC1),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC1sRGB),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC2),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC2sRGB),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC3),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC3sRGB),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC4UNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC4Normalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC5UNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC5Normalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC6UFloat),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC6Float),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC7UNormalized),
  XII_ENUM_CONSTANT(xiiGALResourceFormat::BC7UNormalizedsRGB)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
const xiiUInt8 xiiGALResourceFormat::s_BitsPerElement[xiiGALResourceFormat::ENUM_COUNT] =
{
  0, // Invalid

  128, // RGBAFloat, XYZWFloat
  128, // RGBAUInt
  128, // RGBAInt

  96, // RGBFloat, XYZFloat, UVWFloat
  96, // RGBUInt
  96, // RGBInt

  16, // B5G6R5UNormalized

  32, // BGRAUByteNormalized
  32, // BGRAUByteNormalizedsRGB

  64, // RGBAHalf, XYZWHalf
  64, // RGBAUShort
  64, // RGBAUShortNormalized
  64, // RGBAShort
  64, // RGBAShortNormalized

  64, // RGFloat, XYFloat, UVFloat
  64, // RGUInt
  64, // RGInt

  32, // RGB10A2UInt
  32, // RGB10A2UIntNormalized
  32, // RG11B10Float

  32, // RGBAUByteNormalized
  32, // RGBAUByteNormalizedsRGB
  32, // RGBAUByte
  32, // RGBAByteNormalized
  32, // RGBAByte

  32, // RGHalf, XYHalf, UVHalf
  32, // RGUShort
  32, // RGUShortNormalized
  32, // RGShort
  32, // RGShortNormalized

  16, // RGUByte
  16, // RGUByteNormalized
  16, // RGByte
  16, // RGByteNormalized

  32, // DFloat
  32, // RFloat
  32, // RUInt
  32, // RInt

  16, // RHalf
  16, // RUShort
  16, // RUShortNormalized
  16, // RShort
  16, // RShortNormalized

  8, // RUByte
  8, // RUByteNormalized
  8, // RByte
  8, // RByteNormalized
  8, // AUByteNormalized

  16, // D16
  32, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  4, // BC1
  4, // BC1sRGB

  8, // BC2
  8, // BC2sRGB
  8, // BC3
  8, // BC3sRGB

  4, // BC4UNormalized
  4, // BC4Normalized

  8, // BC5UNormalized
  8, // BC5Normalized
  8, // BC6UFloat
  8, // BC6Float
  8, // BC7UNormalized
  8  // BC7UNormalizedsRGB
};

const xiiUInt8 xiiGALResourceFormat::s_ChannelCount[xiiGALResourceFormat::ENUM_COUNT] =
{
  0, // Invalid

  4, // RGBAFloat, XYZWFloat
  4, // RGBAUInt
  4, // RGBAInt

  3, // RGBFloat, XYZFloat, UVWFloat
  3, // RGBUInt
  3, // RGBInt

  3, // B5G6R5UNormalized

  4, // BGRAUByteNormalized
  4, // BGRAUByteNormalizedsRGB

  4, // RGBAHalf, XYZWHalf
  4, // RGBAUShort
  4, // RGBAUShortNormalized
  4, // RGBAShort
  4, // RGBAShortNormalized

  2, // RGFloat, XYFloat, UVFloat
  2, // RGUInt
  2, // RGInt

  4, // RGB10A2UInt
  4, // RGB10A2UIntNormalized
  3, // RG11B10Float

  4, // RGBAUByteNormalized
  4, // RGBAUByteNormalizedsRGB
  4, // RGBAUByte
  4, // RGBAByteNormalized
  4, // RGBAByte

  2, // RGHalf, XYHalf, UVHalf
  2, // RGUShort
  2, // RGUShortNormalized
  2, // RGShort
  2, // RGShortNormalized

  2, // RGUByte
  2, // RGUByteNormalized
  2, // RGByte
  2, // RGByteNormalized

  1, // DFloat
  1, // RFloat
  1, // RUInt
  1, // RInt

  1, // RHalf
  1, // RUShort
  1, // RUShortNormalized
  1, // RShort
  1, // RShortNormalized

  1, // RUByte
  1, // RUByteNormalized
  1, // RByte
  1, // RByteNormalized
  1, // AUByteNormalized

  1, // D16
  2, // D24S8

  // For compressed formats see: http://msdn.microsoft.com/en-us/library/windows/desktop/hh308955%28v=vs.85%29.aspx

  4, // BC1
  4, // BC1sRGB
  4, // BC2
  4, // BC2sRGB
  4, // BC3
  4, // BC3sRGB
  1, // BC4UNormalized
  1, // BC4Normalized
  2, // BC5UNormalized
  2, // BC5Normalized
  3, // BC6UFloat
  3, // BC6Float
  4, // BC7UNormalized
  4  // BC7UNormalizedsRGB
};
// clang-format off


XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceFormats);

