#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/Resources/Sampler.h>

bool xiiTextureUtils::s_bForceFullQualityAlways = false;

xiiEnum<xiiGALResourceFormat> xiiTextureUtils::ImageFormatToGalFormat(xiiEnum<xiiImageFormat> format, bool bSRGB)
{
  switch (format)
  {
    case xiiImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::RGBA8UNormalizedSRGB;
      else
        return xiiGALResourceFormat::RGBA8UNormalized;

      // case xiiImageFormat::R8G8B8A8_TYPELESS:
    case xiiImageFormat::R8G8B8A8_UNORM_SRGB:
      return xiiGALResourceFormat::RGBA8UNormalizedSRGB;

    case xiiImageFormat::R8G8B8A8_UINT:
      return xiiGALResourceFormat::RGBA8UInt;

    case xiiImageFormat::R8G8B8A8_SNORM:
      return xiiGALResourceFormat::RGBA8SNormalized;

    case xiiImageFormat::R8G8B8A8_SINT:
      return xiiGALResourceFormat::RGBA8SInt;

    case xiiImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BGRA8UNormalizedSRGB;
      else
        return xiiGALResourceFormat::BGRA8UNormalized;

    case xiiImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BGRX8UNormalizedSRGB;
      else
        return xiiGALResourceFormat::BGRX8UNormalized;

      // case xiiImageFormat::B8G8R8A8_TYPELESS:
    case xiiImageFormat::B8G8R8A8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRA8UNormalizedSRGB;

      // case xiiImageFormat::B8G8R8X8_TYPELESS:
    case xiiImageFormat::B8G8R8X8_UNORM_SRGB:
      return xiiGALResourceFormat::BGRX8UNormalizedSRGB;

      // case xiiImageFormat::B8G8R8_UNORM:

      // case xiiImageFormat::BC1_TYPELESS:
    case xiiImageFormat::BC1_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC1UNormalizedSRGB;
      else
        return xiiGALResourceFormat::BC1UNormalized;

    case xiiImageFormat::BC1_UNORM_SRGB:
      return xiiGALResourceFormat::BC1UNormalizedSRGB;

      // case xiiImageFormat::BC2_TYPELESS:
    case xiiImageFormat::BC2_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC2UNormalizedSRGB;
      else
        return xiiGALResourceFormat::BC2UNormalized;

    case xiiImageFormat::BC2_UNORM_SRGB:
      return xiiGALResourceFormat::BC2UNormalizedSRGB;

      // case xiiImageFormat::BC3_TYPELESS:
    case xiiImageFormat::BC3_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC3UNormalizedSRGB;
      else
        return xiiGALResourceFormat::BC3UNormalized;

    case xiiImageFormat::BC3_UNORM_SRGB:
      return xiiGALResourceFormat::BC3UNormalizedSRGB;

      // case xiiImageFormat::BC4_TYPELESS:
    case xiiImageFormat::BC4_UNORM:
      return xiiGALResourceFormat::BC4UNormalized;

    case xiiImageFormat::BC4_SNORM:
      return xiiGALResourceFormat::BC4SNormalized;

      // case xiiImageFormat::BC5_TYPELESS:
    case xiiImageFormat::BC5_UNORM:
      return xiiGALResourceFormat::BC5UNormalized;

    case xiiImageFormat::BC5_SNORM:
      return xiiGALResourceFormat::BC5SNormalized;

      // case xiiImageFormat::BC6H_TYPELESS:
    case xiiImageFormat::BC6H_UF16:
      return xiiGALResourceFormat::BC6HUF16;

    case xiiImageFormat::BC6H_SF16:
      return xiiGALResourceFormat::BC6HSF16;

      // case xiiImageFormat::BC7_TYPELESS:
    case xiiImageFormat::BC7_UNORM:
      if (bSRGB)
        return xiiGALResourceFormat::BC7UNormalizedSRGB;
      else
        return xiiGALResourceFormat::BC7UNormalized;

    case xiiImageFormat::BC7_UNORM_SRGB:
      return xiiGALResourceFormat::BC7UNormalizedSRGB;

    case xiiImageFormat::B5G6R5_UNORM:
      return xiiGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case xiiImageFormat::R16_FLOAT:
      return xiiGALResourceFormat::R16Float;

    case xiiImageFormat::R32_FLOAT:
      return xiiGALResourceFormat::R32Float;

    case xiiImageFormat::R16G16_FLOAT:
      return xiiGALResourceFormat::RG16Float;

    case xiiImageFormat::R32G32_FLOAT:
      return xiiGALResourceFormat::RG32Float;

    case xiiImageFormat::R32G32B32_FLOAT:
      return xiiGALResourceFormat::RGB32Float;

    case xiiImageFormat::R16G16B16A16_FLOAT:
      return xiiGALResourceFormat::RGBA16Float;

    case xiiImageFormat::R32G32B32A32_FLOAT:
      return xiiGALResourceFormat::RGBA32Float;

    case xiiImageFormat::R16G16B16A16_UNORM:
      return xiiGALResourceFormat::RGBA16UNormalized;

    case xiiImageFormat::R8_UNORM:
      return xiiGALResourceFormat::R8UNormalized;

    case xiiImageFormat::R8G8_UNORM:
      return xiiGALResourceFormat::RG8UNormalized;

    case xiiImageFormat::R16G16_UNORM:
      return xiiGALResourceFormat::RG16UNormalized;

    case xiiImageFormat::R11G11B10_FLOAT:
      return xiiGALResourceFormat::RG11B10Float;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiGALResourceFormat::Unknown;
}

xiiEnum<xiiImageFormat> xiiTextureUtils::GalFormatToImageFormat(xiiEnum<xiiGALResourceFormat> format)
{
  switch (format)
  {
    case xiiGALResourceFormat::RGBA32Float:
      return xiiImageFormat::R32G32B32A32_FLOAT;
    case xiiGALResourceFormat::RGBA32UInt:
      return xiiImageFormat::R32G32B32A32_UINT;
    case xiiGALResourceFormat::RGBA32SInt:
      return xiiImageFormat::R32G32B32A32_SINT;
    case xiiGALResourceFormat::RGB32Float:
      return xiiImageFormat::R32G32B32_FLOAT;
    case xiiGALResourceFormat::RGB32UInt:
      return xiiImageFormat::R32G32B32_UINT;
    case xiiGALResourceFormat::RGB32SInt:
      return xiiImageFormat::R32G32B32_SINT;
    case xiiGALResourceFormat::B5G6R5UNormalized:
      return xiiImageFormat::B5G6R5_UNORM;
    case xiiGALResourceFormat::BGRA8UNormalized:
      return xiiImageFormat::B8G8R8A8_UNORM;
    case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      return xiiImageFormat::B8G8R8A8_UNORM_SRGB;
    case xiiGALResourceFormat::RGBA16Float:
      return xiiImageFormat::R16G16B16A16_FLOAT;
    case xiiGALResourceFormat::RGBA16UInt:
      return xiiImageFormat::R16G16B16A16_UINT;
    case xiiGALResourceFormat::RGBA16UNormalized:
      return xiiImageFormat::R16G16B16A16_UNORM;
    case xiiGALResourceFormat::RGBA16SInt:
      return xiiImageFormat::R16G16B16A16_SINT;
    case xiiGALResourceFormat::RGBA16SNormalized:
      return xiiImageFormat::R16G16B16A16_SNORM;
    case xiiGALResourceFormat::RG32Float:
      return xiiImageFormat::R32G32_FLOAT;
    case xiiGALResourceFormat::RG32UInt:
      return xiiImageFormat::R32G32_UINT;
    case xiiGALResourceFormat::RG32SInt:
      return xiiImageFormat::R32G32_SINT;
    case xiiGALResourceFormat::RG11B10Float:
      return xiiImageFormat::R11G11B10_FLOAT;
    case xiiGALResourceFormat::RGBA8UNormalized:
      return xiiImageFormat::R8G8B8A8_UNORM;
    case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      return xiiImageFormat::R8G8B8A8_UNORM_SRGB;
    case xiiGALResourceFormat::RGBA8UInt:
      return xiiImageFormat::R8G8B8A8_UINT;
    case xiiGALResourceFormat::RGBA8SNormalized:
      return xiiImageFormat::R8G8B8A8_SNORM;
    case xiiGALResourceFormat::RGBA8SInt:
      return xiiImageFormat::R8G8B8A8_SINT;
    case xiiGALResourceFormat::RG16Float:
      return xiiImageFormat::R16G16_FLOAT;
    case xiiGALResourceFormat::RG16UInt:
      return xiiImageFormat::R16G16_UINT;
    case xiiGALResourceFormat::RG16UNormalized:
      return xiiImageFormat::R16G16_UNORM;
    case xiiGALResourceFormat::RG16SInt:
      return xiiImageFormat::R16G16_SINT;
    case xiiGALResourceFormat::RG16SNormalized:
      return xiiImageFormat::R16G16_SNORM;
    case xiiGALResourceFormat::RG8UInt:
      return xiiImageFormat::R8G8_UINT;
    case xiiGALResourceFormat::RG8UNormalized:
      return xiiImageFormat::R8G8_UNORM;
    case xiiGALResourceFormat::RG8SInt:
      return xiiImageFormat::R8G8_SINT;
    case xiiGALResourceFormat::RG8SNormalized:
      return xiiImageFormat::R8G8_SNORM;
    case xiiGALResourceFormat::D32Float:
      return xiiImageFormat::D32_FLOAT;
    case xiiGALResourceFormat::R32Float:
      return xiiImageFormat::R32_FLOAT;
    case xiiGALResourceFormat::R32UInt:
      return xiiImageFormat::R32_UINT;
    case xiiGALResourceFormat::R32SInt:
      return xiiImageFormat::R32_SINT;
    case xiiGALResourceFormat::R16Float:
      return xiiImageFormat::R16_FLOAT;
    case xiiGALResourceFormat::R16UInt:
      return xiiImageFormat::R16_UINT;
    case xiiGALResourceFormat::R16UNormalized:
      return xiiImageFormat::R16_UNORM;
    case xiiGALResourceFormat::R16SInt:
      return xiiImageFormat::R16_SINT;
    case xiiGALResourceFormat::R16SNormalized:
      return xiiImageFormat::R16_SNORM;
    case xiiGALResourceFormat::R8UInt:
      return xiiImageFormat::R8_UINT;
    case xiiGALResourceFormat::R8UNormalized:
      return xiiImageFormat::R8_UNORM;
    case xiiGALResourceFormat::R8SInt:
      return xiiImageFormat::R8_SINT;
    case xiiGALResourceFormat::R8SNormalized:
      return xiiImageFormat::R8_SNORM;
    case xiiGALResourceFormat::A8UNormalized:
      return xiiImageFormat::R8_UNORM;
    case xiiGALResourceFormat::D16UNormalized:
      return xiiImageFormat::D16_UNORM;
    case xiiGALResourceFormat::BC1UNormalized:
      return xiiImageFormat::BC1_UNORM;
    case xiiGALResourceFormat::BC1UNormalizedSRGB:
      return xiiImageFormat::BC1_UNORM_SRGB;
    case xiiGALResourceFormat::BC2UNormalized:
      return xiiImageFormat::BC2_UNORM;
    case xiiGALResourceFormat::BC2UNormalizedSRGB:
      return xiiImageFormat::BC2_UNORM_SRGB;
    case xiiGALResourceFormat::BC3UNormalized:
      return xiiImageFormat::BC3_UNORM;
    case xiiGALResourceFormat::BC3UNormalizedSRGB:
      return xiiImageFormat::BC3_UNORM_SRGB;
    case xiiGALResourceFormat::BC4UNormalized:
      return xiiImageFormat::BC4_UNORM;
    case xiiGALResourceFormat::BC4SNormalized:
      return xiiImageFormat::BC4_SNORM;
    case xiiGALResourceFormat::BC5UNormalized:
      return xiiImageFormat::BC5_UNORM;
    case xiiGALResourceFormat::BC5SNormalized:
      return xiiImageFormat::BC5_SNORM;
    case xiiGALResourceFormat::BC6HUF16:
      return xiiImageFormat::BC6H_UF16;
    case xiiGALResourceFormat::BC6HSF16:
      return xiiImageFormat::BC6H_SF16;
    case xiiGALResourceFormat::BC7UNormalized:
      return xiiImageFormat::BC7_UNORM;
    case xiiGALResourceFormat::BC7UNormalizedSRGB:
      return xiiImageFormat::BC7_UNORM_SRGB;
    case xiiGALResourceFormat::RGB10A2UInt:
    case xiiGALResourceFormat::RGB10A2UNormalized:
    case xiiGALResourceFormat::D24UNormalizedS8UInt:
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

xiiEnum<xiiImageFormat> xiiTextureUtils::GalFormatToImageFormat(xiiEnum<xiiGALResourceFormat> format, bool bRemoveSRGB)
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

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return xiiEnum<xiiGALTextureAddressMode>();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Textures_TextureUtils);
