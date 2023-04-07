#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>

bool xiiTextureUtils::s_bForceFullQualityAlways = false;

xiiGALResourceFormat::Enum xiiTextureUtils::ImageFormatToGalFormat(xiiImageFormat::Enum format, bool bSRGB)
{
  switch (format)
  {
    case xiiImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::RGBAUByteNormalizedsRGB;
      else
        return xiiGALResourceFormat::RGBAUByteNormalized;

      // case xiiImageFormat::R8G8B8A8_TYPELESS:
    case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
      return xiiGALResourceFormat::RGBAUByteNormalizedsRGB;

    case xiiImageFormat::R8G8B8A8_UINT:
      return xiiGALResourceFormat::RGBAUInt;

    case xiiImageFormat::R8G8B8A8_SNORM:
      return xiiGALResourceFormat::RGBAByteNormalized;

    case xiiImageFormat::R8G8B8A8_SINT:
      return xiiGALResourceFormat::RGBAInt;

    case xiiImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return xiiGALResourceFormat::BGRAUByteNormalized;

    case xiiImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return xiiGALResourceFormat::BGRAUByteNormalized;

      // case xiiImageFormat::B8G8R8A8_TYPELESS:
    case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case xiiImageFormat::B8G8R8X8_TYPELESS:
    case xiiImageFormat::B8G8R8X8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case xiiImageFormat::B8G8R8_UNORM:

      // case xiiImageFormat::BC1_TYPELESS:
    case xiiImageFormat::BC1_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC1sRGB;
      else
        return xiiGALResourceFormat::BC1;

    case xiiImageFormat::BC1_UNORM_SRGB:
      return xiiGALResourceFormat::BC1sRGB;

      // case xiiImageFormat::BC2_TYPELESS:
    case xiiImageFormat::BC2_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC2sRGB;
      else
        return xiiGALResourceFormat::BC2;

    case xiiImageFormat::BC2_UNORM_SRGB:
      return xiiGALResourceFormat::BC2sRGB;

      // case xiiImageFormat::BC3_TYPELESS:
    case xiiImageFormat::BC3_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC3sRGB;
      else
        return xiiGALResourceFormat::BC3;

    case xiiImageFormat::BC3_UNORM_SRGB:
      return xiiGALResourceFormat::BC3sRGB;

      // case xiiImageFormat::BC4_TYPELESS:
    case xiiImageFormat::BC4_UNORM:
      return xiiGALResourceFormat::BC4UNormalized;

    case xiiImageFormat::BC4_SNORM:
      return xiiGALResourceFormat::BC4Normalized;

      // case xiiImageFormat::BC5_TYPELESS:
    case xiiImageFormat::BC5_UNORM:
      return xiiGALResourceFormat::BC5UNormalized;

    case xiiImageFormat::BC5_SNORM:
      return xiiGALResourceFormat::BC5Normalized;

      // case xiiImageFormat::BC6H_TYPELESS:
    case xiiImageFormat::BC6H_UF16:
      return xiiGALResourceFormat::BC6UFloat;

    case xiiImageFormat::BC6H_SF16:
      return xiiGALResourceFormat::BC6Float;

      // case xiiImageFormat::BC7_TYPELESS:
    case xiiImageFormat::BC7_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC7UNormalizedsRGB;
      else
        return xiiGALResourceFormat::BC7UNormalized;

    case xiiImageFormat::BC7_UNORM_SRGB:
      return xiiGALResourceFormat::BC7UNormalizedsRGB;

    case xiiImageFormat::B5G6R5_UNORM:
      return xiiGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case xiiImageFormat::R16_FLOAT:
      return xiiGALResourceFormat::RHalf;

    case xiiImageFormat::R32_FLOAT:
      return xiiGALResourceFormat::RFloat;

    case xiiImageFormat::R16G16_FLOAT:
      return xiiGALResourceFormat::RGHalf;

    case xiiImageFormat::R32G32_FLOAT:
      return xiiGALResourceFormat::RGFloat;

    case xiiImageFormat::R32G32B32_FLOAT:
      return xiiGALResourceFormat::RGBFloat;

    case xiiImageFormat::R16G16B16A16_FLOAT:
      return xiiGALResourceFormat::RGBAHalf;

    case xiiImageFormat::R32G32B32A32_FLOAT:
      return xiiGALResourceFormat::RGBAFloat;

    case xiiImageFormat::R16G16B16A16_UNORM:
      return xiiGALResourceFormat::RGBAUShortNormalized;

    case xiiImageFormat::R8_UNORM:
      return xiiGALResourceFormat::RUByteNormalized;

    case xiiImageFormat::R8G8_UNORM:
      return xiiGALResourceFormat::RGUByteNormalized;

    case xiiImageFormat::R16G16_UNORM:
      return xiiGALResourceFormat::RGUShortNormalized;

    case xiiImageFormat::R11G11B10_FLOAT:
      return xiiGALResourceFormat::RG11B10Float;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return xiiGALResourceFormat::Invalid;
}

xiiImageFormat::Enum xiiTextureUtils::GalFormatToImageFormat(xiiGALResourceFormat::Enum format)
{
  switch (format)
  {
    case xiiGALResourceFormat::RGBAFloat:
      return xiiImageFormat::R32G32B32A32_FLOAT;
    case xiiGALResourceFormat::RGBAUInt:
      return xiiImageFormat::R8G8B8A8_UINT;
    case xiiGALResourceFormat::RGBAInt:
      return xiiImageFormat::R8G8B8A8_SINT;
    case xiiGALResourceFormat::RGBFloat:
      return xiiImageFormat::R32G32B32_FLOAT;
    case xiiGALResourceFormat::RGBUInt:
      return xiiImageFormat::R32G32B32_UINT;
    case xiiGALResourceFormat::RGBInt:
      return xiiImageFormat::R32G32B32_SINT;
    case xiiGALResourceFormat::B5G6R5UNormalized:
      return xiiImageFormat::B5G6R5_UNORM;
    case xiiGALResourceFormat::BGRAUByteNormalized:
      return xiiImageFormat::B8G8R8A8_UNORM;
    case xiiGALResourceFormat::BGRAUByteNormalizedsRGB:
      return xiiImageFormat::B8G8R8A8_UNORM_SRGB;
    case xiiGALResourceFormat::RGBAHalf:
      return xiiImageFormat::R16G16B16A16_FLOAT;
    case xiiGALResourceFormat::RGBAUShort:
      return xiiImageFormat::R16G16B16A16_UINT;
    case xiiGALResourceFormat::RGBAUShortNormalized:
      return xiiImageFormat::R16G16B16A16_UNORM;
    case xiiGALResourceFormat::RGBAShort:
      return xiiImageFormat::R16G16B16A16_SINT;
    case xiiGALResourceFormat::RGBAShortNormalized:
      return xiiImageFormat::R16G16B16A16_SNORM;
    case xiiGALResourceFormat::RGFloat:
      return xiiImageFormat::R32G32_FLOAT;
    case xiiGALResourceFormat::RGUInt:
      return xiiImageFormat::R32G32_UINT;
    case xiiGALResourceFormat::RGInt:
      return xiiImageFormat::R32G32_SINT;
    case xiiGALResourceFormat::RG11B10Float:
      return xiiImageFormat::R11G11B10_FLOAT;
    case xiiGALResourceFormat::RGBAUByteNormalized:
      return xiiImageFormat::R8G8B8A8_UNORM;
    case xiiGALResourceFormat::RGBAUByteNormalizedsRGB:
      return xiiImageFormat::R8G8B8A8_UNORM_SRGB;
    case xiiGALResourceFormat::RGBAUByte:
      return xiiImageFormat::R8G8B8A8_UINT;
    case xiiGALResourceFormat::RGBAByteNormalized:
      return xiiImageFormat::R8G8B8A8_SNORM;
    case xiiGALResourceFormat::RGBAByte:
      return xiiImageFormat::R8G8B8A8_SINT;
    case xiiGALResourceFormat::RGHalf:
      return xiiImageFormat::R16G16_FLOAT;
    case xiiGALResourceFormat::RGUShort:
      return xiiImageFormat::R16G16_UINT;
    case xiiGALResourceFormat::RGUShortNormalized:
      return xiiImageFormat::R16G16_UNORM;
    case xiiGALResourceFormat::RGShort:
      return xiiImageFormat::R16G16_SINT;
    case xiiGALResourceFormat::RGShortNormalized:
      return xiiImageFormat::R16G16_SNORM;
    case xiiGALResourceFormat::RGUByte:
      return xiiImageFormat::R8G8_UINT;
    case xiiGALResourceFormat::RGUByteNormalized:
      return xiiImageFormat::R8G8_UNORM;
    case xiiGALResourceFormat::RGByte:
      return xiiImageFormat::R8G8_SINT;
    case xiiGALResourceFormat::RGByteNormalized:
      return xiiImageFormat::R8G8_SNORM;
    case xiiGALResourceFormat::DFloat:
      return xiiImageFormat::R32_FLOAT;
    case xiiGALResourceFormat::RFloat:
      return xiiImageFormat::R32_FLOAT;
    case xiiGALResourceFormat::RUInt:
      return xiiImageFormat::R32_UINT;
    case xiiGALResourceFormat::RInt:
      return xiiImageFormat::R32_SINT;
    case xiiGALResourceFormat::RHalf:
      return xiiImageFormat::R16_FLOAT;
    case xiiGALResourceFormat::RUShort:
      return xiiImageFormat::R16_UINT;
    case xiiGALResourceFormat::RUShortNormalized:
      return xiiImageFormat::R16_UNORM;
    case xiiGALResourceFormat::RShort:
      return xiiImageFormat::R16_SINT;
    case xiiGALResourceFormat::RShortNormalized:
      return xiiImageFormat::R16_SNORM;
    case xiiGALResourceFormat::RUByte:
      return xiiImageFormat::R8_UINT;
    case xiiGALResourceFormat::RUByteNormalized:
      return xiiImageFormat::R8_UNORM;
    case xiiGALResourceFormat::RByte:
      return xiiImageFormat::R8_SINT;
    case xiiGALResourceFormat::RByteNormalized:
      return xiiImageFormat::R8_SNORM;
    case xiiGALResourceFormat::AUByteNormalized:
      return xiiImageFormat::R8_UNORM;
    case xiiGALResourceFormat::D16:
      return xiiImageFormat::R16_FLOAT;
    case xiiGALResourceFormat::BC1:
      return xiiImageFormat::BC1_UNORM;
    case xiiGALResourceFormat::BC1sRGB:
      return xiiImageFormat::BC1_UNORM_SRGB;
    case xiiGALResourceFormat::BC2:
      return xiiImageFormat::BC2_UNORM;
    case xiiGALResourceFormat::BC2sRGB:
      return xiiImageFormat::BC2_UNORM_SRGB;
    case xiiGALResourceFormat::BC3:
      return xiiImageFormat::BC3_UNORM;
    case xiiGALResourceFormat::BC3sRGB:
      return xiiImageFormat::BC3_UNORM_SRGB;
    case xiiGALResourceFormat::BC4UNormalized:
      return xiiImageFormat::BC4_UNORM;
    case xiiGALResourceFormat::BC4Normalized:
      return xiiImageFormat::BC4_SNORM;
    case xiiGALResourceFormat::BC5UNormalized:
      return xiiImageFormat::BC5_UNORM;
    case xiiGALResourceFormat::BC5Normalized:
      return xiiImageFormat::BC5_SNORM;
    case xiiGALResourceFormat::BC6UFloat:
      return xiiImageFormat::BC6H_UF16;
    case xiiGALResourceFormat::BC6Float:
      return xiiImageFormat::BC6H_SF16;
    case xiiGALResourceFormat::BC7UNormalized:
      return xiiImageFormat::BC7_UNORM;
    case xiiGALResourceFormat::BC7UNormalizedsRGB:
      return xiiImageFormat::BC7_UNORM_SRGB;
    case xiiGALResourceFormat::RGB10A2UInt:
    case xiiGALResourceFormat::RGB10A2UIntNormalized:
    case xiiGALResourceFormat::D24S8:
    default:
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      xiiStringBuilder sFormat;
      XII_ASSERT_DEBUG(xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALResourceFormat>(), format, sFormat, xiiReflectionUtils::EnumConversionMode::ValueNameOnly), "Cannot convert GAL format '{}' to string", format);
      XII_ASSERT_DEBUG(false, "The GL format: '{}' does not have a matching image format.", sFormat);
#endif
    }
  }
  return xiiImageFormat::UNKNOWN;
}

xiiImageFormat::Enum xiiTextureUtils::GalFormatToImageFormat(xiiGALResourceFormat::Enum format, bool bRemoveSRGB)
{
  xiiImageFormat::Enum imageFormat = GalFormatToImageFormat(format);
  if (bRemoveSRGB)
  {
    imageFormat = xiiImageFormat::AsLinear(imageFormat);
  }
  return imageFormat;
}

void xiiTextureUtils::ConfigureSampler(xiiTextureFilterSetting::Enum filter, xiiGALSamplerStateCreationDescription& out_Sampler)
{
  const xiiTextureFilterSetting::Enum thisFilter = xiiRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  out_Sampler.m_MinFilter       = xiiGALTextureFilterMode::Linear;
  out_Sampler.m_MagFilter       = xiiGALTextureFilterMode::Linear;
  out_Sampler.m_MipFilter       = xiiGALTextureFilterMode::Linear;
  out_Sampler.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
    case xiiTextureFilterSetting::FixedNearest:
      out_Sampler.m_MinFilter = xiiGALTextureFilterMode::Point;
      out_Sampler.m_MagFilter = xiiGALTextureFilterMode::Point;
      out_Sampler.m_MipFilter = xiiGALTextureFilterMode::Point;
      break;
    case xiiTextureFilterSetting::FixedBilinear:
      out_Sampler.m_MipFilter = xiiGALTextureFilterMode::Point;
      break;
    case xiiTextureFilterSetting::FixedTrilinear:
      break;
    case xiiTextureFilterSetting::FixedAnisotropic2x:
      out_Sampler.m_MinFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MipFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 2;
      break;
    case xiiTextureFilterSetting::FixedAnisotropic4x:
      out_Sampler.m_MinFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MipFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 4;
      break;
    case xiiTextureFilterSetting::FixedAnisotropic8x:
      out_Sampler.m_MinFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MipFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 8;
      break;
    case xiiTextureFilterSetting::FixedAnisotropic16x:
      out_Sampler.m_MinFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MagFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_MipFilter       = xiiGALTextureFilterMode::Anisotropic;
      out_Sampler.m_uiMaxAnisotropy = 16;
      break;
    default:
      break;
  }
}

XII_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureUtils);
