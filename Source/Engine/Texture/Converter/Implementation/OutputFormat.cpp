/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Converter/TextureConverterProcessor.h>

static xiiImageFormat::Enum DetermineOutputFormatPC(xiiTextureConverterUsage::Enum targetFormat, xiiTextureConverterCompressionMode::Enum compressionMode, xiiUInt32 uiNumChannels)
{
  if (targetFormat == xiiTextureConverterUsage::NormalMap || targetFormat == xiiTextureConverterUsage::NormalMap_Inverted || targetFormat == xiiTextureConverterUsage::BumpMap)
  {
    if (compressionMode >= xiiTextureConverterCompressionMode::High)
      return xiiImageFormat::BC5_UNORM;

    if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
      return xiiImageFormat::R8G8_UNORM;

    // TODO: in the rare case that the input texture has higher precision, we could use R16G16_UNORM or R16G16_FLOAT here
    // R16G16_UNORM isn't supported on all platforms, so R16G16_FLOAT may be better
    // return xiiImageFormat::R16G16_FLOAT;
    return xiiImageFormat::R8G8_UNORM;
  }

  if (targetFormat == xiiTextureConverterUsage::Color)
  {
    if (compressionMode >= xiiTextureConverterCompressionMode::High && uiNumChannels < 4)
      return xiiImageFormat::BC1_UNORM_SRGB;

    if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
      return xiiImageFormat::BC7_UNORM_SRGB;

    return xiiImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == xiiTextureConverterUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiImageFormat::BC4_UNORM;

        return xiiImageFormat::R8_UNORM;

      case 2:
        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiImageFormat::BC5_UNORM;

        return xiiImageFormat::R8G8_UNORM;

      case 3:
        if (compressionMode >= xiiTextureConverterCompressionMode::High)
          return xiiImageFormat::BC1_UNORM;

        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiImageFormat::BC7_UNORM;

        return xiiImageFormat::R8G8B8A8_UNORM;

      case 4:
        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiImageFormat::BC7_UNORM;

        return xiiImageFormat::R8G8B8A8_UNORM;

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
          return xiiImageFormat::BC6H_UF16;

        return xiiImageFormat::R16_FLOAT;

      case 2:
        return xiiImageFormat::R16G16_FLOAT;

      case 3:
        if (compressionMode >= xiiTextureConverterCompressionMode::High)
          return xiiImageFormat::BC6H_UF16;

        if (compressionMode >= xiiTextureConverterCompressionMode::Medium)
          return xiiImageFormat::R11G11B10_FLOAT;

        return xiiImageFormat::R16G16B16A16_FLOAT;

      case 4:
        return xiiImageFormat::R16G16B16A16_FLOAT;
    }
  }

  return xiiImageFormat::UNKNOWN;
}

xiiResult xiiTextureConverterProcessor::ChooseOutputFormat(xiiEnum<xiiImageFormat>& out_Format, xiiEnum<xiiTextureConverterUsage> usage, xiiUInt32 uiNumChannels) const
{
  XII_PROFILE_SCOPE("ChooseOutputFormat");

  XII_ASSERT_DEV(out_Format == xiiImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
    case xiiTextureConverterTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (out_Format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Failed to decide for an output image format.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}
