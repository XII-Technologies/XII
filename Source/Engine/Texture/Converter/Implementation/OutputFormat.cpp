/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Converter/TextureConverterProcessor.h>

static xiiEnum<xiiGALResourceFormat> DetermineOutputFormatPC(xiiTextureConverterUsage::Enum targetFormat, xiiTextureConverterCompressionMode::Enum compressionMode, xiiUInt32 uiNumChannels)
{
  if (targetFormat == xiiTextureConverterUsage::NormalMap || targetFormat == xiiTextureConverterUsage::NormalMap_Inverted || targetFormat == xiiTextureConverterUsage::BumpMap)
  {
    if (compressionMode >= xiiTextureConverterCompressionMode::High)
      return xiiGALResourceFormat::BC5UNormalized;

    if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
      return xiiGALResourceFormat::RG8UNormalized;

    // TODO: in the rare case that the input texture has higher precision, we could use R16G16_UNORM or R16G16_FLOAT here
    // R16G16_UNORM isn't supported on all platforms, so R16G16_FLOAT may be better
    // return xiiGALResourceFormat::RG16Float;
    return xiiGALResourceFormat::RG8UNormalized;
  }

  if (targetFormat == xiiTextureConverterUsage::Color)
  {
    if (compressionMode >= xiiTextureConverterCompressionMode::High && uiNumChannels < 4)
      return xiiGALResourceFormat::BC1UNormalizedSRGB;

    if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
      return xiiGALResourceFormat::BC7UNormalizedSRGB;

    return xiiGALResourceFormat::RGBA8UNormalizedSRGB;
  }

  if (targetFormat == xiiTextureConverterUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiGALResourceFormat::BC4UNormalized;

        return xiiGALResourceFormat::R8UNormalized;

      case 2:
        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiGALResourceFormat::BC5UNormalized;

        return xiiGALResourceFormat::RG8UNormalized;

      case 3:
        if (compressionMode >= xiiTextureConverterCompressionMode::High)
          return xiiGALResourceFormat::BC1UNormalized;

        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiGALResourceFormat::BC7UNormalized;

        return xiiGALResourceFormat::RGBA8UNormalized;

      case 4:
        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiGALResourceFormat::BC7UNormalized;

        return xiiGALResourceFormat::RGBA8UNormalized;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (targetFormat == xiiTextureConverterUsage::Hdr)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= xiiTextureConverterCompressionMode::High)
          return xiiGALResourceFormat::BC6HUF16;

        return xiiGALResourceFormat::R16Float;

      case 2:
        return xiiGALResourceFormat::RG16Float;

      case 3:
        if (compressionMode >= xiiTextureConverterCompressionMode::High)
          return xiiGALResourceFormat::BC6HUF16;

        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiGALResourceFormat::RG11B10Float;

        return xiiGALResourceFormat::RGBA16Float;

      case 4:
        return xiiGALResourceFormat::RGBA16Float;
    }
  }

  return xiiGALResourceFormat::Unknown;
}

xiiResult xiiTextureConverterProcessor::ChooseOutputFormat(xiiEnum<xiiGALResourceFormat>& out_Format, xiiEnum<xiiTextureConverterUsage> usage, xiiUInt32 uiNumChannels) const
{
  XII_PROFILE_SCOPE("ChooseOutputFormat");

  XII_ASSERT_DEV(out_Format == xiiGALResourceFormat::Unknown, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
    case xiiTextureConverterTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (out_Format == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Error("Failed to decide for an output image format.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}
