/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/ImageFormatMappings.h>

#include <directx/dxgiformat.h>

#define MAKE_FOURCC(a, b, c, d) (a) | ((b) << 8) | ((c) << 16) | ((d) << 24)

xiiUInt32 xiiImageFormatMappings::ToDxgiFormat(xiiEnum<xiiGALResourceFormat> format)
{
  switch (format)
  {
    case xiiGALResourceFormat::Unknown:
      return DXGI_FORMAT_UNKNOWN;
    case xiiGALResourceFormat::RGBA32Typeless:
      return DXGI_FORMAT_R32G32B32A32_TYPELESS;
    case xiiGALResourceFormat::RGBA32Float:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case xiiGALResourceFormat::RGBA32UInt:
      return DXGI_FORMAT_R32G32B32A32_UINT;
    case xiiGALResourceFormat::RGBA32SInt:
      return DXGI_FORMAT_R32G32B32A32_SINT;
    case xiiGALResourceFormat::RGB32Typeless:
      return DXGI_FORMAT_R32G32B32_TYPELESS;
    case xiiGALResourceFormat::RGB32Float:
      return DXGI_FORMAT_R32G32B32_FLOAT;
    case xiiGALResourceFormat::RGB32UInt:
      return DXGI_FORMAT_R32G32B32_UINT;
    case xiiGALResourceFormat::RGB32SInt:
      return DXGI_FORMAT_R32G32B32_SINT;
    case xiiGALResourceFormat::RGBA16Typeless:
      return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    case xiiGALResourceFormat::RGBA16Float:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case xiiGALResourceFormat::RGBA16UNormalized:
      return DXGI_FORMAT_R16G16B16A16_UNORM;
    case xiiGALResourceFormat::RGBA16UInt:
      return DXGI_FORMAT_R16G16B16A16_UINT;
    case xiiGALResourceFormat::RGBA16SNormalized:
      return DXGI_FORMAT_R16G16B16A16_SNORM;
    case xiiGALResourceFormat::RGBA16SInt:
      return DXGI_FORMAT_R16G16B16A16_SINT;
    case xiiGALResourceFormat::RG32Typeless:
      return DXGI_FORMAT_R32G32_TYPELESS;
    case xiiGALResourceFormat::RG32Float:
      return DXGI_FORMAT_R32G32_FLOAT;
    case xiiGALResourceFormat::RG32UInt:
      return DXGI_FORMAT_R32G32_UINT;
    case xiiGALResourceFormat::RG32SInt:
      return DXGI_FORMAT_R32G32_SINT;
    case xiiGALResourceFormat::R32G8X24Typeless:
      return DXGI_FORMAT_R32G8X24_TYPELESS;
    case xiiGALResourceFormat::D32FloatS8X24UInt:
      return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    case xiiGALResourceFormat::R32FloatX8X24Typeless:
      return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case xiiGALResourceFormat::X32TypelessG8X24UInt:
      return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
    case xiiGALResourceFormat::RGB10A2Typeless:
      return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case xiiGALResourceFormat::RGB10A2UNormalized:
      return DXGI_FORMAT_R10G10B10A2_UNORM;
    case xiiGALResourceFormat::RGB10A2UInt:
      return DXGI_FORMAT_R10G10B10A2_UINT;
    case xiiGALResourceFormat::RG11B10Float:
      return DXGI_FORMAT_R11G11B10_FLOAT;
    case xiiGALResourceFormat::RGBA8Typeless:
      return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case xiiGALResourceFormat::RGBA8UNormalized:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case xiiGALResourceFormat::RGBA8UInt:
      return DXGI_FORMAT_R8G8B8A8_UINT;
    case xiiGALResourceFormat::RGBA8SNormalized:
      return DXGI_FORMAT_R8G8B8A8_SNORM;
    case xiiGALResourceFormat::RGBA8SInt:
      return DXGI_FORMAT_R8G8B8A8_SINT;
    case xiiGALResourceFormat::RG16Typeless:
      return DXGI_FORMAT_R16G16_TYPELESS;
    case xiiGALResourceFormat::RG16Float:
      return DXGI_FORMAT_R16G16_FLOAT;
    case xiiGALResourceFormat::RG16UNormalized:
      return DXGI_FORMAT_R16G16_UNORM;
    case xiiGALResourceFormat::RG16UInt:
      return DXGI_FORMAT_R16G16_UINT;
    case xiiGALResourceFormat::RG16SNormalized:
      return DXGI_FORMAT_R16G16_SNORM;
    case xiiGALResourceFormat::RG16SInt:
      return DXGI_FORMAT_R16G16_SINT;
    case xiiGALResourceFormat::R32Typeless:
      return DXGI_FORMAT_R32_TYPELESS;
    case xiiGALResourceFormat::D32Float:
      return DXGI_FORMAT_D32_FLOAT;
    case xiiGALResourceFormat::R32Float:
      return DXGI_FORMAT_R32_FLOAT;
    case xiiGALResourceFormat::R32UInt:
      return DXGI_FORMAT_R32_UINT;
    case xiiGALResourceFormat::R32SInt:
      return DXGI_FORMAT_R32_SINT;
    case xiiGALResourceFormat::R24G8Typeless:
      return DXGI_FORMAT_R24G8_TYPELESS;
    case xiiGALResourceFormat::D24UNormalizedS8UInt:
      return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case xiiGALResourceFormat::R24UNormalizedX8Typeless:
      return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case xiiGALResourceFormat::X24TypelessG8UInt:
      return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    case xiiGALResourceFormat::RG8Typeless:
      return DXGI_FORMAT_R8G8_TYPELESS;
    case xiiGALResourceFormat::RG8UNormalized:
      return DXGI_FORMAT_R8G8_UNORM;
    case xiiGALResourceFormat::RG8UInt:
      return DXGI_FORMAT_R8G8_UINT;
    case xiiGALResourceFormat::RG8SNormalized:
      return DXGI_FORMAT_R8G8_SNORM;
    case xiiGALResourceFormat::RG8SInt:
      return DXGI_FORMAT_R8G8_SINT;
    case xiiGALResourceFormat::R16Typeless:
      return DXGI_FORMAT_R16_TYPELESS;
    case xiiGALResourceFormat::R16Float:
      return DXGI_FORMAT_R16_FLOAT;
    case xiiGALResourceFormat::D16UNormalized:
      return DXGI_FORMAT_D16_UNORM;
    case xiiGALResourceFormat::R16UNormalized:
      return DXGI_FORMAT_R16_UNORM;
    case xiiGALResourceFormat::R16UInt:
      return DXGI_FORMAT_R16_UINT;
    case xiiGALResourceFormat::R16SNormalized:
      return DXGI_FORMAT_R16_SNORM;
    case xiiGALResourceFormat::R16SInt:
      return DXGI_FORMAT_R16_SINT;
    case xiiGALResourceFormat::R8Typeless:
      return DXGI_FORMAT_R8_TYPELESS;
    case xiiGALResourceFormat::R8UNormalized:
      return DXGI_FORMAT_R8_UNORM;
    case xiiGALResourceFormat::R8UInt:
      return DXGI_FORMAT_R8_UINT;
    case xiiGALResourceFormat::R8SNormalized:
      return DXGI_FORMAT_R8_SNORM;
    case xiiGALResourceFormat::R8SInt:
      return DXGI_FORMAT_R8_SINT;
    case xiiGALResourceFormat::A8UNormalized:
      return DXGI_FORMAT_A8_UNORM;
    case xiiGALResourceFormat::R1UNormalized:
      return DXGI_FORMAT_R1_UNORM;
    case xiiGALResourceFormat::RGB9E5SharedExponent:
      return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    case xiiGALResourceFormat::RG8BG8UNormalized:
      return DXGI_FORMAT_R8G8_B8G8_UNORM;
    case xiiGALResourceFormat::GR8GB8UNormalized:
      return DXGI_FORMAT_G8R8_G8B8_UNORM;
    case xiiGALResourceFormat::BC1Typeless:
      return DXGI_FORMAT_BC1_TYPELESS;
    case xiiGALResourceFormat::BC1UNormalized:
      return DXGI_FORMAT_BC1_UNORM;
    case xiiGALResourceFormat::BC1UNormalizedSRGB:
      return DXGI_FORMAT_BC1_UNORM_SRGB;
    case xiiGALResourceFormat::BC2Typeless:
      return DXGI_FORMAT_BC2_TYPELESS;
    case xiiGALResourceFormat::BC2UNormalized:
      return DXGI_FORMAT_BC2_UNORM;
    case xiiGALResourceFormat::BC2UNormalizedSRGB:
      return DXGI_FORMAT_BC2_UNORM_SRGB;
    case xiiGALResourceFormat::BC3Typeless:
      return DXGI_FORMAT_BC3_TYPELESS;
    case xiiGALResourceFormat::BC3UNormalized:
      return DXGI_FORMAT_BC3_UNORM;
    case xiiGALResourceFormat::BC3UNormalizedSRGB:
      return DXGI_FORMAT_BC3_UNORM_SRGB;
    case xiiGALResourceFormat::BC4Typeless:
      return DXGI_FORMAT_BC4_TYPELESS;
    case xiiGALResourceFormat::BC4UNormalized:
      return DXGI_FORMAT_BC4_UNORM;
    case xiiGALResourceFormat::BC4SNormalized:
      return DXGI_FORMAT_BC4_SNORM;
    case xiiGALResourceFormat::BC5Typeless:
      return DXGI_FORMAT_BC5_TYPELESS;
    case xiiGALResourceFormat::BC5UNormalized:
      return DXGI_FORMAT_BC5_UNORM;
    case xiiGALResourceFormat::BC5SNormalized:
      return DXGI_FORMAT_BC5_SNORM;
    case xiiGALResourceFormat::B5G6R5UNormalized:
      return DXGI_FORMAT_B5G6R5_UNORM;
    case xiiGALResourceFormat::B5G5R5A1UNormalized:
      return DXGI_FORMAT_B5G5R5A1_UNORM;
    case xiiGALResourceFormat::BGRA8UNormalized:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case xiiGALResourceFormat::BGRX8UNormalized:
      return DXGI_FORMAT_B8G8R8X8_UNORM;
    case xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized:
      return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    case xiiGALResourceFormat::BGRA8Typeless:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    case xiiGALResourceFormat::BGRX8Typeless:
      return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case xiiGALResourceFormat::BGRX8UNormalizedSRGB:
      return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    case xiiGALResourceFormat::BC6HTypeless:
      return DXGI_FORMAT_BC6H_TYPELESS;
    case xiiGALResourceFormat::BC6HUF16:
      return DXGI_FORMAT_BC6H_UF16;
    case xiiGALResourceFormat::BC6HSF16:
      return DXGI_FORMAT_BC6H_SF16;
    case xiiGALResourceFormat::BC7Typeless:
      return DXGI_FORMAT_BC7_TYPELESS;
    case xiiGALResourceFormat::BC7UNormalized:
      return DXGI_FORMAT_BC7_UNORM;
    case xiiGALResourceFormat::BC7UNormalizedSRGB:
      return DXGI_FORMAT_BC7_UNORM_SRGB;
    case xiiGALResourceFormat::NV12:
      return DXGI_FORMAT_NV12;
    case xiiGALResourceFormat::P010:
      return DXGI_FORMAT_P010;
    case xiiGALResourceFormat::P016:
      return DXGI_FORMAT_P016;
    case xiiGALResourceFormat::YUY2:
      return DXGI_FORMAT_YUY2;
    case xiiGALResourceFormat::AYUV:
      return DXGI_FORMAT_AYUV;
    case xiiGALResourceFormat::P216:
      return DXGI_FORMAT_Y216;
    case xiiGALResourceFormat::P416:
      return DXGI_FORMAT_Y416;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return DXGI_FORMAT_UNKNOWN;
}

xiiEnum<xiiGALResourceFormat> xiiImageFormatMappings::FromDxgiFormat(xiiUInt32 uiDxgiFormat)
{
  switch (uiDxgiFormat)
  {
    case DXGI_FORMAT_UNKNOWN:
      return xiiGALResourceFormat::Unknown;
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      return xiiGALResourceFormat::RGBA32Typeless;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
      return xiiGALResourceFormat::RGBA32Float;
    case DXGI_FORMAT_R32G32B32A32_UINT:
      return xiiGALResourceFormat::RGBA32UInt;
    case DXGI_FORMAT_R32G32B32A32_SINT:
      return xiiGALResourceFormat::RGBA32SInt;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
      return xiiGALResourceFormat::RGB32Typeless;
    case DXGI_FORMAT_R32G32B32_FLOAT:
      return xiiGALResourceFormat::RGB32Float;
    case DXGI_FORMAT_R32G32B32_UINT:
      return xiiGALResourceFormat::RGB32UInt;
    case DXGI_FORMAT_R32G32B32_SINT:
      return xiiGALResourceFormat::RGB32SInt;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      return xiiGALResourceFormat::RGBA16Typeless;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return xiiGALResourceFormat::RGBA16Float;
    case DXGI_FORMAT_R16G16B16A16_UNORM:
      return xiiGALResourceFormat::RGBA16UNormalized;
    case DXGI_FORMAT_R16G16B16A16_UINT:
      return xiiGALResourceFormat::RGBA16UInt;
    case DXGI_FORMAT_R16G16B16A16_SNORM:
      return xiiGALResourceFormat::RGBA16SNormalized;
    case DXGI_FORMAT_R16G16B16A16_SINT:
      return xiiGALResourceFormat::RGBA16SInt;
    case DXGI_FORMAT_R32G32_TYPELESS:
      return xiiGALResourceFormat::RG32Typeless;
    case DXGI_FORMAT_R32G32_FLOAT:
      return xiiGALResourceFormat::RG32Float;
    case DXGI_FORMAT_R32G32_UINT:
      return xiiGALResourceFormat::RG32UInt;
    case DXGI_FORMAT_R32G32_SINT:
      return xiiGALResourceFormat::RG32SInt;
    case DXGI_FORMAT_R32G8X24_TYPELESS:
      return xiiGALResourceFormat::R32G8X24Typeless;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
      return xiiGALResourceFormat::D32FloatS8X24UInt;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
      return xiiGALResourceFormat::R32FloatX8X24Typeless;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
      return xiiGALResourceFormat::X32TypelessG8X24UInt;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      return xiiGALResourceFormat::RGB10A2Typeless;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
      return xiiGALResourceFormat::RGB10A2UNormalized;
    case DXGI_FORMAT_R10G10B10A2_UINT:
      return xiiGALResourceFormat::RGB10A2UInt;
    case DXGI_FORMAT_R11G11B10_FLOAT:
      return xiiGALResourceFormat::RG11B10Float;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return xiiGALResourceFormat::RGBA8Typeless;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
      return xiiGALResourceFormat::RGBA8UNormalized;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return xiiGALResourceFormat::RGBA8UNormalizedSRGB;
    case DXGI_FORMAT_R8G8B8A8_UINT:
      return xiiGALResourceFormat::RGBA8UInt;
    case DXGI_FORMAT_R8G8B8A8_SNORM:
      return xiiGALResourceFormat::RGBA8SNormalized;
    case DXGI_FORMAT_R8G8B8A8_SINT:
      return xiiGALResourceFormat::RGBA8SInt;
    case DXGI_FORMAT_R16G16_TYPELESS:
      return xiiGALResourceFormat::RG16Typeless;
    case DXGI_FORMAT_R16G16_FLOAT:
      return xiiGALResourceFormat::RG16Float;
    case DXGI_FORMAT_R16G16_UNORM:
      return xiiGALResourceFormat::RG16UNormalized;
    case DXGI_FORMAT_R16G16_UINT:
      return xiiGALResourceFormat::RG16UInt;
    case DXGI_FORMAT_R16G16_SNORM:
      return xiiGALResourceFormat::RG16SNormalized;
    case DXGI_FORMAT_R16G16_SINT:
      return xiiGALResourceFormat::RG16SInt;
    case DXGI_FORMAT_R32_TYPELESS:
      return xiiGALResourceFormat::R32Typeless;
    case DXGI_FORMAT_D32_FLOAT:
      return xiiGALResourceFormat::D32Float;
    case DXGI_FORMAT_R32_FLOAT:
      return xiiGALResourceFormat::R32Float;
    case DXGI_FORMAT_R32_UINT:
      return xiiGALResourceFormat::R32UInt;
    case DXGI_FORMAT_R32_SINT:
      return xiiGALResourceFormat::R32SInt;
    case DXGI_FORMAT_R24G8_TYPELESS:
      return xiiGALResourceFormat::R24G8Typeless;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return xiiGALResourceFormat::D24UNormalizedS8UInt;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      return xiiGALResourceFormat::R24UNormalizedX8Typeless;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      return xiiGALResourceFormat::X24TypelessG8UInt;
    case DXGI_FORMAT_R8G8_TYPELESS:
      return xiiGALResourceFormat::RG8Typeless;
    case DXGI_FORMAT_R8G8_UNORM:
      return xiiGALResourceFormat::RG8UNormalized;
    case DXGI_FORMAT_R8G8_UINT:
      return xiiGALResourceFormat::RG8UInt;
    case DXGI_FORMAT_R8G8_SNORM:
      return xiiGALResourceFormat::RG8SNormalized;
    case DXGI_FORMAT_R8G8_SINT:
      return xiiGALResourceFormat::RG8SInt;
    case DXGI_FORMAT_R16_TYPELESS:
      return xiiGALResourceFormat::R16Typeless;
    case DXGI_FORMAT_R16_FLOAT:
      return xiiGALResourceFormat::R16Float;
    case DXGI_FORMAT_D16_UNORM:
      return xiiGALResourceFormat::D16UNormalized;
    case DXGI_FORMAT_R16_UNORM:
      return xiiGALResourceFormat::R16UNormalized;
    case DXGI_FORMAT_R16_UINT:
      return xiiGALResourceFormat::R16UInt;
    case DXGI_FORMAT_R16_SNORM:
      return xiiGALResourceFormat::R16SNormalized;
    case DXGI_FORMAT_R16_SINT:
      return xiiGALResourceFormat::R16SInt;
    case DXGI_FORMAT_R8_TYPELESS:
      return xiiGALResourceFormat::R8Typeless;
    case DXGI_FORMAT_R8_UNORM:
      return xiiGALResourceFormat::R8UNormalized;
    case DXGI_FORMAT_R8_UINT:
      return xiiGALResourceFormat::R8UInt;
    case DXGI_FORMAT_R8_SNORM:
      return xiiGALResourceFormat::R8SNormalized;
    case DXGI_FORMAT_R8_SINT:
      return xiiGALResourceFormat::R8SInt;
    case DXGI_FORMAT_A8_UNORM:
      return xiiGALResourceFormat::A8UNormalized;
    case DXGI_FORMAT_R1_UNORM:
      return xiiGALResourceFormat::R1UNormalized;
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
      return xiiGALResourceFormat::RGB9E5SharedExponent;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
      return xiiGALResourceFormat::RG8BG8UNormalized;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
      return xiiGALResourceFormat::GR8GB8UNormalized;
    case DXGI_FORMAT_BC1_TYPELESS:
      return xiiGALResourceFormat::BC1Typeless;
    case DXGI_FORMAT_BC1_UNORM:
      return xiiGALResourceFormat::BC1UNormalized;
    case DXGI_FORMAT_BC1_UNORM_SRGB:
      return xiiGALResourceFormat::BC1UNormalizedSRGB;
    case DXGI_FORMAT_BC2_TYPELESS:
      return xiiGALResourceFormat::BC2Typeless;
    case DXGI_FORMAT_BC2_UNORM:
      return xiiGALResourceFormat::BC2UNormalized;
    case DXGI_FORMAT_BC2_UNORM_SRGB:
      return xiiGALResourceFormat::BC2UNormalizedSRGB;
    case DXGI_FORMAT_BC3_TYPELESS:
      return xiiGALResourceFormat::BC3Typeless;
    case DXGI_FORMAT_BC3_UNORM:
      return xiiGALResourceFormat::BC3UNormalized;
    case DXGI_FORMAT_BC3_UNORM_SRGB:
      return xiiGALResourceFormat::BC3UNormalizedSRGB;
    case DXGI_FORMAT_BC4_TYPELESS:
      return xiiGALResourceFormat::BC4Typeless;
    case DXGI_FORMAT_BC4_UNORM:
      return xiiGALResourceFormat::BC4UNormalized;
    case DXGI_FORMAT_BC4_SNORM:
      return xiiGALResourceFormat::BC4SNormalized;
    case DXGI_FORMAT_BC5_TYPELESS:
      return xiiGALResourceFormat::BC5Typeless;
    case DXGI_FORMAT_BC5_UNORM:
      return xiiGALResourceFormat::BC5UNormalized;
    case DXGI_FORMAT_BC5_SNORM:
      return xiiGALResourceFormat::BC5SNormalized;
    case DXGI_FORMAT_B5G6R5_UNORM:
      return xiiGALResourceFormat::B5G6R5UNormalized;
    case DXGI_FORMAT_B5G5R5A1_UNORM:
      return xiiGALResourceFormat::B5G5R5A1UNormalized;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
      return xiiGALResourceFormat::BGRA8UNormalized;
    case DXGI_FORMAT_B8G8R8X8_UNORM:
      return xiiGALResourceFormat::BGRX8UNormalized;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
      return xiiGALResourceFormat::R10G10B10XRBiasA2UNormalized;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return xiiGALResourceFormat::BGRA8Typeless;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRA8UNormalizedSRGB;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      return xiiGALResourceFormat::BGRX8Typeless;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRX8UNormalizedSRGB;
    case DXGI_FORMAT_BC6H_TYPELESS:
      return xiiGALResourceFormat::BC6HTypeless;
    case DXGI_FORMAT_BC6H_UF16:
      return xiiGALResourceFormat::BC6HUF16;
    case DXGI_FORMAT_BC6H_SF16:
      return xiiGALResourceFormat::BC6HSF16;
    case DXGI_FORMAT_BC7_TYPELESS:
      return xiiGALResourceFormat::BC7Typeless;
    case DXGI_FORMAT_BC7_UNORM:
      return xiiGALResourceFormat::BC7UNormalized;
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return xiiGALResourceFormat::BC7UNormalizedSRGB;
    case DXGI_FORMAT_NV12:
      return xiiGALResourceFormat::NV12;
    case DXGI_FORMAT_P010:
      return xiiGALResourceFormat::P010;
    case DXGI_FORMAT_P016:
      return xiiGALResourceFormat::P016;
    case DXGI_FORMAT_YUY2:
      return xiiGALResourceFormat::YUY2;
    case DXGI_FORMAT_AYUV:
      return xiiGALResourceFormat::AYUV;
    case DXGI_FORMAT_Y216:
      return xiiGALResourceFormat::P216;
    case DXGI_FORMAT_Y416:
      return xiiGALResourceFormat::P416;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiGALResourceFormat::Unknown;
}

xiiUInt32 xiiImageFormatMappings::ToFourCc(xiiEnum<xiiGALResourceFormat> format)
{
  switch (format)
  {
    // BC / DXT Compressed Formats.
    case xiiGALResourceFormat::BC1UNormalized:
      return MAKE_FOURCC('D', 'X', 'T', '1');
    case xiiGALResourceFormat::BC2UNormalized:
      return MAKE_FOURCC('D', 'X', 'T', '3');
    case xiiGALResourceFormat::BC3UNormalized:
      return MAKE_FOURCC('D', 'X', 'T', '5');
    case xiiGALResourceFormat::BC4UNormalized:
      return MAKE_FOURCC('B', 'C', '4', 'U');
    case xiiGALResourceFormat::BC5UNormalized:
      return MAKE_FOURCC('B', 'C', '5', 'U');

    // Packed YUV Formats (Single-plane).
    case xiiGALResourceFormat::YUY2:
      return MAKE_FOURCC('Y', 'U', 'Y', '2');
    case xiiGALResourceFormat::AYUV:
      return MAKE_FOURCC('A', 'Y', 'U', 'V');

    // Multi-planar YUV Formats (2-plane).
    case xiiGALResourceFormat::NV12:
      return MAKE_FOURCC('N', 'V', '1', '2');
    case xiiGALResourceFormat::P010:
      return MAKE_FOURCC('P', '0', '1', '0');
    case xiiGALResourceFormat::P016:
      return MAKE_FOURCC('P', '0', '1', '6');

    // High-bit-depth YUV formats (2-plane or 3-plane).
    case xiiGALResourceFormat::P216:
      return MAKE_FOURCC('P', '2', '1', '6');
    case xiiGALResourceFormat::P416:
      return MAKE_FOURCC('P', '4', '1', '6');

    default:
      return 0U;
  }
}

xiiEnum<xiiGALResourceFormat> xiiImageFormatMappings::FromFourCc(xiiUInt32 uiFourCc)
{
  switch (uiFourCc)
  {
    // BC / DXT Compressed Formats.
    case MAKE_FOURCC('D', 'X', 'T', '1'):
      return xiiGALResourceFormat::BC1UNormalized;

    case MAKE_FOURCC('D', 'X', 'T', '2'):
    case MAKE_FOURCC('D', 'X', 'T', '3'):
      return xiiGALResourceFormat::BC2UNormalized;

    case MAKE_FOURCC('D', 'X', 'T', '4'):
    case MAKE_FOURCC('D', 'X', 'T', '5'):
      return xiiGALResourceFormat::BC3UNormalized;

    case MAKE_FOURCC('A', 'T', 'I', '1'):
    case MAKE_FOURCC('B', 'C', '4', 'U'):
      return xiiGALResourceFormat::BC4UNormalized;

    case MAKE_FOURCC('A', 'T', 'I', '2'):
    case MAKE_FOURCC('B', 'C', '5', 'U'):
      return xiiGALResourceFormat::BC5UNormalized;

    // Packed YUV Formats (Single-plane).
    case MAKE_FOURCC('Y', 'U', 'Y', '2'): // YUY2 4:2:2 packed
      return xiiGALResourceFormat::YUY2;

    case MAKE_FOURCC('A', 'Y', 'U', 'V'): // AYUV 4:4:4 packed
      return xiiGALResourceFormat::AYUV;

    // Multi-planar YUV Formats (2-plane).
    case MAKE_FOURCC('N', 'V', '1', '2'): // NV12 = Y + interleaved UV
      return xiiGALResourceFormat::NV12;

    case MAKE_FOURCC('P', '0', '1', '0'): // P010 = 10-bit YUV420
      return xiiGALResourceFormat::P010;

    case MAKE_FOURCC('P', '0', '1', '6'): // P016 = 16-bit YUV420
      return xiiGALResourceFormat::P016;

    // High-bit-depth YUV formats (2-plane or 3-plane).
    case MAKE_FOURCC('P', '2', '1', '6'): // P216 = YUV422 16-bit
      return xiiGALResourceFormat::P216;

    case MAKE_FOURCC('P', '4', '1', '6'): // P416 = YUV444 16-bit
      return xiiGALResourceFormat::P416;

    default:
      return xiiGALResourceFormat::Unknown;
  }
}
