
#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct XII_RENDERERFOUNDATION_DLL xiiGALResourceFormat
{
  using StorageType = xiiUInt8;

  enum Enum : xiiUInt8
  {
    Invalid = 0,

    RGBAFloat,
    XYZWFloat = RGBAFloat,
    RGBAUInt,
    RGBAInt,

    RGBFloat,
    XYZFloat = RGBFloat,
    UVWFloat = RGBFloat,
    RGBUInt,
    RGBInt,

    B5G6R5UNormalized,
    BGRAUByteNormalized,
    BGRAUByteNormalizedsRGB,

    RGBAHalf,
    XYZWHalf = RGBAHalf,
    RGBAUShort,
    RGBAUShortNormalized,
    RGBAShort,
    RGBAShortNormalized,

    RGFloat,
    XYFloat = RGFloat,
    UVFloat = RGFloat,
    RGUInt,
    RGInt,

    RGB10A2UInt,
    RGB10A2UIntNormalized,
    RG11B10Float,

    RGBAUByteNormalized,
    RGBAUByteNormalizedsRGB,
    RGBAUByte,
    RGBAByteNormalized,
    RGBAByte,

    RGHalf,
    XYHalf = RGHalf,
    UVHalf = RGHalf,
    RGUShort,
    RGUShortNormalized,
    RGShort,
    RGShortNormalized,
    RGUByte,
    RGUByteNormalized,
    RGByte,
    RGByteNormalized,

    DFloat,

    RFloat,
    RUInt,
    RInt,
    RHalf,
    RUShort,
    RUShortNormalized,
    RShort,
    RShortNormalized,
    RUByte,
    RUByteNormalized,
    RByte,
    RByteNormalized,

    AUByteNormalized,

    D16,
    D24S8,

    BC1,
    BC1sRGB,
    BC2,
    BC2sRGB,
    BC3,
    BC3sRGB,
    BC4UNormalized,
    BC4Normalized,
    BC5UNormalized,
    BC5Normalized,
    BC6UFloat,
    BC6Float,
    BC7UNormalized,
    BC7UNormalizedsRGB,

    ENUM_COUNT,

    Default = RGBAUByteNormalizedsRGB
  };


  // General format Meta-Informations:

  /// \brief The size in bits per element (usually pixels, except for mesh stream elements) of a single element of the given resource format.
  static xiiUInt32 GetBitsPerElement(xiiGALResourceFormat::Enum format);

  /// \brief The number of color channels this format contains.
  static xiiUInt8 GetChannelCount(xiiGALResourceFormat::Enum format);

  /// \todo A combination of propertyflags, something like srgb, normalized, ...
  // Would be very useful for some GL stuff and Testing.

  /// \brief Returns whether the given resource format is a depth format
  static bool IsDepthFormat(xiiGALResourceFormat::Enum format);
  static bool IsStencilFormat(xiiGALResourceFormat::Enum format);

  static bool IsSrgb(xiiGALResourceFormat::Enum format);

private:
  static const xiiUInt8 s_BitsPerElement[xiiGALResourceFormat::ENUM_COUNT];

  static const xiiUInt8 s_ChannelCount[xiiGALResourceFormat::ENUM_COUNT];
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RENDERERFOUNDATION_DLL, xiiGALResourceFormat);


template <typename NativeFormatType, NativeFormatType InvalidFormat>
class xiiGALFormatLookupEntry
{
public:
  inline xiiGALFormatLookupEntry();

  inline xiiGALFormatLookupEntry(NativeFormatType storage);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& VA(NativeFormatType vertexAttributeType);

  inline xiiGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType resourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eVertexAttributeType;
  NativeFormatType m_eResourceViewType;
};

// Reusable table class to store lookup information (from xiiGALResourceFormat to the various formats for texture/buffer storage, views)
template <typename FormatClass>
class xiiGALFormatLookupTable
{
public:
  xiiGALFormatLookupTable();

  XII_ALWAYS_INLINE const FormatClass& GetFormatInfo(xiiGALResourceFormat::Enum format) const;

  XII_ALWAYS_INLINE void SetFormatInfo(xiiGALResourceFormat::Enum format, const FormatClass& newFormatInfo);

private:
  FormatClass m_Formats[xiiGALResourceFormat::ENUM_COUNT];
};

#include <RendererFoundation/Resources/Implementation/ResourceFormats_inl.h>
