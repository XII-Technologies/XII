#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/Resources/Sampler.h>

bool xiiTextureUtils::s_bForceFullQualityAlways = false;

xiiEnum<xiiGALTextureFormat> xiiTextureUtils::ImageFormatToGalFormat(xiiEnum<xiiImageFormat> format, bool bSRGB)
{
  switch (format)
  {
    case xiiImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::RGBA8UNormalizedSRGB;
      else
        return xiiGALTextureFormat::RGBA8UNormalized;

      // case xiiImageFormat::R8G8B8A8_TYPELESS:
    case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
      return xiiGALTextureFormat::RGBA8UNormalizedSRGB;

    case xiiImageFormat::R8G8B8A8_UINT:
      return xiiGALTextureFormat::RGBA8UInt;

    case xiiImageFormat::R8G8B8A8_SNORM:
      return xiiGALTextureFormat::RGBA8SNormalized;

    case xiiImageFormat::R8G8B8A8_SINT:
      return xiiGALTextureFormat::RGBA8SInt;

    case xiiImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::BGRA8UNormalizedSRGB;
      else
        return xiiGALTextureFormat::BGRA8UNormalized;

    case xiiImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::BGRX8UNormalizedSRGB;
      else
        return xiiGALTextureFormat::BGRX8UNormalized;

      // case xiiImageFormat::B8G8R8A8_TYPELESS:
    case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
      return xiiGALTextureFormat::BGRA8UNormalizedSRGB;

      // case xiiImageFormat::B8G8R8X8_TYPELESS:
    case xiiImageFormat::B8G8R8X8_UNORM_SRGB:
      return xiiGALTextureFormat::BGRX8UNormalizedSRGB;

      // case xiiImageFormat::B8G8R8_UNORM:

      // case xiiImageFormat::BC1_TYPELESS:
    case xiiImageFormat::BC1_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::BC1UNormalizedSRGB;
      else
        return xiiGALTextureFormat::BC1UNormalized;

    case xiiImageFormat::BC1_UNORM_SRGB:
      return xiiGALTextureFormat::BC1UNormalizedSRGB;

      // case xiiImageFormat::BC2_TYPELESS:
    case xiiImageFormat::BC2_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::BC2UNormalizedSRGB;
      else
        return xiiGALTextureFormat::BC2UNormalized;

    case xiiImageFormat::BC2_UNORM_SRGB:
      return xiiGALTextureFormat::BC2UNormalizedSRGB;

      // case xiiImageFormat::BC3_TYPELESS:
    case xiiImageFormat::BC3_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::BC3UNormalizedSRGB;
      else
        return xiiGALTextureFormat::BC3UNormalized;

    case xiiImageFormat::BC3_UNORM_SRGB:
      return xiiGALTextureFormat::BC3UNormalizedSRGB;

      // case xiiImageFormat::BC4_TYPELESS:
    case xiiImageFormat::BC4_UNORM:
      return xiiGALTextureFormat::BC4UNormalized;

    case xiiImageFormat::BC4_SNORM:
      return xiiGALTextureFormat::BC4SNormalized;

      // case xiiImageFormat::BC5_TYPELESS:
    case xiiImageFormat::BC5_UNORM:
      return xiiGALTextureFormat::BC5UNormalized;

    case xiiImageFormat::BC5_SNORM:
      return xiiGALTextureFormat::BC5SNormalized;

      // case xiiImageFormat::BC6H_TYPELESS:
    case xiiImageFormat::BC6H_UF16:
      return xiiGALTextureFormat::BC6HUF16;

    case xiiImageFormat::BC6H_SF16:
      return xiiGALTextureFormat::BC6HSF16;

      // case xiiImageFormat::BC7_TYPELESS:
    case xiiImageFormat::BC7_UNORM:
      if (bSRGB)
        return xiiGALTextureFormat::BC7UNormalizedSRGB;
      else
        return xiiGALTextureFormat::BC7UNormalized;

    case xiiImageFormat::BC7_UNORM_SRGB:
      return xiiGALTextureFormat::BC7UNormalizedSRGB;

    case xiiImageFormat::B5G6R5_UNORM:
      return xiiGALTextureFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case xiiImageFormat::R16_FLOAT:
      return xiiGALTextureFormat::R16Float;

    case xiiImageFormat::R32_FLOAT:
      return xiiGALTextureFormat::R32Float;

    case xiiImageFormat::R16G16_FLOAT:
      return xiiGALTextureFormat::RG16Float;

    case xiiImageFormat::R32G32_FLOAT:
      return xiiGALTextureFormat::RG32Float;

    case xiiImageFormat::R32G32B32_FLOAT:
      return xiiGALTextureFormat::RGB32Float;

    case xiiImageFormat::R16G16B16A16_FLOAT:
      return xiiGALTextureFormat::RGBA16Float;

    case xiiImageFormat::R32G32B32A32_FLOAT:
      return xiiGALTextureFormat::RGBA32Float;

    case xiiImageFormat::R16G16B16A16_UNORM:
      return xiiGALTextureFormat::RGBA16UNormalized;

    case xiiImageFormat::R8_UNORM:
      return xiiGALTextureFormat::R8UNormalized;

    case xiiImageFormat::R8G8_UNORM:
      return xiiGALTextureFormat::RG8UNormalized;

    case xiiImageFormat::R16G16_UNORM:
      return xiiGALTextureFormat::RG16UNormalized;

    case xiiImageFormat::R11G11B10_FLOAT:
      return xiiGALTextureFormat::RG11B10Float;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiGALTextureFormat::Unknown;
}

xiiEnum<xiiImageFormat> xiiTextureUtils::GalFormatToImageFormat(xiiEnum<xiiGALTextureFormat> format)
{
  switch (format)
  {
    case xiiGALTextureFormat::RGBA32Float:
      return xiiImageFormat::R32G32B32A32_FLOAT;
    case xiiGALTextureFormat::RGBA32UInt:
      return xiiImageFormat::R32G32B32A32_UINT;
    case xiiGALTextureFormat::RGBA32SInt:
      return xiiImageFormat::R32G32B32A32_SINT;
    case xiiGALTextureFormat::RGB32Float:
      return xiiImageFormat::R32G32B32_FLOAT;
    case xiiGALTextureFormat::RGB32UInt:
      return xiiImageFormat::R32G32B32_UINT;
    case xiiGALTextureFormat::RGB32SInt:
      return xiiImageFormat::R32G32B32_SINT;
    case xiiGALTextureFormat::B5G6R5UNormalized:
      return xiiImageFormat::B5G6R5_UNORM;
    case xiiGALTextureFormat::BGRA8UNormalized:
      return xiiImageFormat::B8G8R8A8_UNORM;
    case xiiGALTextureFormat::BGRA8UNormalizedSRGB:
      return xiiImageFormat::B8G8R8A8_UNORM_SRGB;
    case xiiGALTextureFormat::RGBA16Float:
      return xiiImageFormat::R16G16B16A16_FLOAT;
    case xiiGALTextureFormat::RGBA16UInt:
      return xiiImageFormat::R16G16B16A16_UINT;
    case xiiGALTextureFormat::RGBA16UNormalized:
      return xiiImageFormat::R16G16B16A16_UNORM;
    case xiiGALTextureFormat::RGBA16SInt:
      return xiiImageFormat::R16G16B16A16_SINT;
    case xiiGALTextureFormat::RGBA16SNormalized:
      return xiiImageFormat::R16G16B16A16_SNORM;
    case xiiGALTextureFormat::RG32Float:
      return xiiImageFormat::R32G32_FLOAT;
    case xiiGALTextureFormat::RG32UInt:
      return xiiImageFormat::R32G32_UINT;
    case xiiGALTextureFormat::RG32SInt:
      return xiiImageFormat::R32G32_SINT;
    case xiiGALTextureFormat::RG11B10Float:
      return xiiImageFormat::R11G11B10_FLOAT;
    case xiiGALTextureFormat::RGBA8UNormalized:
      return xiiImageFormat::R8G8B8A8_UNORM;
    case xiiGALTextureFormat::RGBA8UNormalizedSRGB:
      return xiiImageFormat::R8G8B8A8_UNORM_SRGB;
    case xiiGALTextureFormat::RGBA8UInt:
      return xiiImageFormat::R8G8B8A8_UINT;
    case xiiGALTextureFormat::RGBA8SNormalized:
      return xiiImageFormat::R8G8B8A8_SNORM;
    case xiiGALTextureFormat::RGBA8SInt:
      return xiiImageFormat::R8G8B8A8_SINT;
    case xiiGALTextureFormat::RG16Float:
      return xiiImageFormat::R16G16_FLOAT;
    case xiiGALTextureFormat::RG16UInt:
      return xiiImageFormat::R16G16_UINT;
    case xiiGALTextureFormat::RG16UNormalized:
      return xiiImageFormat::R16G16_UNORM;
    case xiiGALTextureFormat::RG16SInt:
      return xiiImageFormat::R16G16_SINT;
    case xiiGALTextureFormat::RG16SNormalized:
      return xiiImageFormat::R16G16_SNORM;
    case xiiGALTextureFormat::RG8UInt:
      return xiiImageFormat::R8G8_UINT;
    case xiiGALTextureFormat::RG8UNormalized:
      return xiiImageFormat::R8G8_UNORM;
    case xiiGALTextureFormat::RG8SInt:
      return xiiImageFormat::R8G8_SINT;
    case xiiGALTextureFormat::RG8SNormalized:
      return xiiImageFormat::R8G8_SNORM;
    case xiiGALTextureFormat::D32Float:
      return xiiImageFormat::D32_FLOAT;
    case xiiGALTextureFormat::R32Float:
      return xiiImageFormat::R32_FLOAT;
    case xiiGALTextureFormat::R32UInt:
      return xiiImageFormat::R32_UINT;
    case xiiGALTextureFormat::R32SInt:
      return xiiImageFormat::R32_SINT;
    case xiiGALTextureFormat::R16Float:
      return xiiImageFormat::R16_FLOAT;
    case xiiGALTextureFormat::R16UInt:
      return xiiImageFormat::R16_UINT;
    case xiiGALTextureFormat::R16UNormalized:
      return xiiImageFormat::R16_UNORM;
    case xiiGALTextureFormat::R16SInt:
      return xiiImageFormat::R16_SINT;
    case xiiGALTextureFormat::R16SNormalized:
      return xiiImageFormat::R16_SNORM;
    case xiiGALTextureFormat::R8UInt:
      return xiiImageFormat::R8_UINT;
    case xiiGALTextureFormat::R8UNormalized:
      return xiiImageFormat::R8_UNORM;
    case xiiGALTextureFormat::R8SInt:
      return xiiImageFormat::R8_SINT;
    case xiiGALTextureFormat::R8SNormalized:
      return xiiImageFormat::R8_SNORM;
    case xiiGALTextureFormat::A8UNormalized:
      return xiiImageFormat::R8_UNORM;
    case xiiGALTextureFormat::D16UNormalized:
      return xiiImageFormat::D16_UNORM;
    case xiiGALTextureFormat::BC1UNormalized:
      return xiiImageFormat::BC1_UNORM;
    case xiiGALTextureFormat::BC1UNormalizedSRGB:
      return xiiImageFormat::BC1_UNORM_SRGB;
    case xiiGALTextureFormat::BC2UNormalized:
      return xiiImageFormat::BC2_UNORM;
    case xiiGALTextureFormat::BC2UNormalizedSRGB:
      return xiiImageFormat::BC2_UNORM_SRGB;
    case xiiGALTextureFormat::BC3UNormalized:
      return xiiImageFormat::BC3_UNORM;
    case xiiGALTextureFormat::BC3UNormalizedSRGB:
      return xiiImageFormat::BC3_UNORM_SRGB;
    case xiiGALTextureFormat::BC4UNormalized:
      return xiiImageFormat::BC4_UNORM;
    case xiiGALTextureFormat::BC4SNormalized:
      return xiiImageFormat::BC4_SNORM;
    case xiiGALTextureFormat::BC5UNormalized:
      return xiiImageFormat::BC5_UNORM;
    case xiiGALTextureFormat::BC5SNormalized:
      return xiiImageFormat::BC5_SNORM;
    case xiiGALTextureFormat::BC6HUF16:
      return xiiImageFormat::BC6H_UF16;
    case xiiGALTextureFormat::BC6HSF16:
      return xiiImageFormat::BC6H_SF16;
    case xiiGALTextureFormat::BC7UNormalized:
      return xiiImageFormat::BC7_UNORM;
    case xiiGALTextureFormat::BC7UNormalizedSRGB:
      return xiiImageFormat::BC7_UNORM_SRGB;
    case xiiGALTextureFormat::RGB10A2UInt:
    case xiiGALTextureFormat::RGB10A2UNormalized:
    case xiiGALTextureFormat::D24UNormalizedS8UInt:
    default:
    {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
      xiiStringBuilder sFormat;
      XII_ASSERT_DEBUG(xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiGALTextureFormat>(), format, sFormat, xiiReflectionUtils::EnumConversionMode::ValueNameOnly), "Cannot convert GAL format '{}' to string", format);
      XII_ASSERT_DEBUG(false, "The GL format: '{}' does not have a matching image format.", sFormat);
#endif
    }
  }
  return xiiImageFormat::UNKNOWN;
}

xiiEnum<xiiImageFormat> xiiTextureUtils::GalFormatToImageFormat(xiiEnum<xiiGALTextureFormat> format, bool bRemoveSRGB)
{
  xiiEnum<xiiImageFormat> imageFormat = GalFormatToImageFormat(format);
  if (bRemoveSRGB)
  {
    imageFormat = xiiImageFormat::AsLinear(imageFormat);
  }
  return imageFormat;
}

void xiiTextureUtils::ConfigureSampler(xiiEnum<xiiTextureFilterSetting> filter, xiiGALSamplerCreationDescription& out_sampler)
{
  const xiiEnum<xiiTextureFilterSetting> thisFilter = xiiRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  out_sampler.m_MinFilter       = xiiGALFilterType::Linear;
  out_sampler.m_MagFilter       = xiiGALFilterType::Linear;
  out_sampler.m_MipFilter       = xiiGALFilterType::Linear;
  out_sampler.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
    case xiiTextureFilterSetting::FixedNearest:
      out_sampler.m_MinFilter = xiiGALFilterType::Point;
      out_sampler.m_MagFilter = xiiGALFilterType::Point;
      out_sampler.m_MipFilter = xiiGALFilterType::Point;
      break;
    case xiiTextureFilterSetting::FixedBilinear:
      out_sampler.m_MipFilter = xiiGALFilterType::Point;
      break;
    case xiiTextureFilterSetting::FixedTrilinear:
      break;
    case xiiTextureFilterSetting::FixedAnisotropic2x:
      out_sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MipFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 2;
      break;
    case xiiTextureFilterSetting::FixedAnisotropic4x:
      out_sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MipFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 4;
      break;
    case xiiTextureFilterSetting::FixedAnisotropic8x:
      out_sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MipFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 8;
      break;
    case xiiTextureFilterSetting::FixedAnisotropic16x:
      out_sampler.m_MinFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MagFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_MipFilter       = xiiGALFilterType::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 16;
      break;
    default:
      break;
  }
}

xiiEnum<xiiGALTextureAddressMode> xiiTextureUtils::GALTextureAddressMode(xiiEnum<xiiImageAddressMode> mode)
{
  switch (mode)
  {
    case xiiImageAddressMode::Repeat:
      return xiiGALTextureAddressMode::Wrap;
    case xiiImageAddressMode::Clamp:
      return xiiGALTextureAddressMode::Clamp;
    case xiiImageAddressMode::ClampBorder:
      return xiiGALTextureAddressMode::Border;
    case xiiImageAddressMode::Mirror:
      return xiiGALTextureAddressMode::Mirror;
  }
  return xiiEnum<xiiGALTextureAddressMode>();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_TextureUtils);
