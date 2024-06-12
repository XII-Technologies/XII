#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

static xiiImageFormat::Enum DetermineOutputFormatPC(xiiTexConvUsage::Enum targetFormat, xiiTexConvCompressionMode::Enum compressionMode, xiiUInt32 uiNumChannels)
{
  if (targetFormat == xiiTexConvUsage::NormalMap || targetFormat == xiiTexConvUsage::NormalMap_Inverted || targetFormat == xiiTexConvUsage::BumpMap)
  {
    if (compressionMode >= xiiTexConvCompressionMode::High)
      return xiiImageFormat::BC5_UNORM;

    if (compressionMode >= xiiTexConvCompressionMode::Medium)
      return xiiImageFormat::R8G8_UNORM;

    return xiiImageFormat::R16G16_UNORM;
  }

  if (targetFormat == xiiTexConvUsage::Color)
  {
    if (compressionMode >= xiiTexConvCompressionMode::High && uiNumChannels < 4)
      return xiiImageFormat::BC1_UNORM_SRGB;

    if (compressionMode >= xiiTexConvCompressionMode::Medium)
      return xiiImageFormat::BC7_UNORM_SRGB;

    return xiiImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == xiiTexConvUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= xiiTexConvCompressionMode::Medium)
          return xiiImageFormat::BC4_UNORM;

        return xiiImageFormat::R8_UNORM;

      case 2:
        if (compressionMode >= xiiTexConvCompressionMode::Medium)
          return xiiImageFormat::BC5_UNORM;

        return xiiImageFormat::R8G8_UNORM;

      case 3:
        if (compressionMode >= xiiTexConvCompressionMode::High)
          return xiiImageFormat::BC1_UNORM;

        if (compressionMode >= xiiTexConvCompressionMode::Medium)
          return xiiImageFormat::BC7_UNORM;

        return xiiImageFormat::R8G8B8A8_UNORM;

      case 4:
        if (compressionMode >= xiiTexConvCompressionMode::Medium)
          return xiiImageFormat::BC7_UNORM;

        return xiiImageFormat::R8G8B8A8_UNORM;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (targetFormat == xiiTexConvUsage::Hdr)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= xiiTexConvCompressionMode::High)
          return xiiImageFormat::BC6H_UF16;

        return xiiImageFormat::R16_FLOAT;

      case 2:
        return xiiImageFormat::R16G16_FLOAT;

      case 3:
        if (compressionMode >= xiiTexConvCompressionMode::High)
          return xiiImageFormat::BC6H_UF16;

        if (compressionMode >= xiiTexConvCompressionMode::Medium)
          return xiiImageFormat::R11G11B10_FLOAT;

        return xiiImageFormat::R16G16B16A16_FLOAT;

      case 4:
        return xiiImageFormat::R16G16B16A16_FLOAT;
    }
  }

  return xiiImageFormat::UNKNOWN;
}

xiiResult xiiTexConvProcessor::ChooseOutputFormat(xiiEnum<xiiImageFormat>& out_Format, xiiEnum<xiiTexConvUsage> usage, xiiUInt32 uiNumChannels) const
{
  XII_PROFILE_SCOPE("ChooseOutputFormat");

  XII_ASSERT_DEV(out_Format == xiiImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
      // case  xiiTexConvTargetPlatform::Android:
      //  out_Format = DetermineOutputFormatAndroid(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
      //  break;

    case xiiTexConvTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  if (out_Format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Failed to decide for an output image format.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_OutputFormat);
