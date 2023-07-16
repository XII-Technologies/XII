#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

// clang=format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTexConvCompressionMode, 1)
XII_ENUM_CONSTANTS(xiiTexConvCompressionMode::None, xiiTexConvCompressionMode::Medium, xiiTexConvCompressionMode::High)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTexConvMipmapMode, 1)
XII_ENUM_CONSTANTS(xiiTexConvMipmapMode::None, xiiTexConvMipmapMode::Linear, xiiTexConvMipmapMode::Kaiser)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiTexConvUsage, 1)
XII_ENUM_CONSTANT(xiiTexConvUsage::Auto), XII_ENUM_CONSTANT(xiiTexConvUsage::Color), XII_ENUM_CONSTANT(xiiTexConvUsage::Linear),
  XII_ENUM_CONSTANT(xiiTexConvUsage::Hdr), XII_ENUM_CONSTANT(xiiTexConvUsage::NormalMap), XII_ENUM_CONSTANT(xiiTexConvUsage::NormalMap_Inverted),
  XII_ENUM_CONSTANT(xiiTexConvUsage::BumpMap),
  XII_END_STATIC_REFLECTED_ENUM;
// clang=format on

xiiTexConvProcessor::xiiTexConvProcessor() = default;

xiiResult xiiTexConvProcessor::Process()
{
  XII_PROFILE_SCOPE("xiiTexConvProcessor::Process");

  if (m_Descriptor.m_OutputType == xiiTexConvOutputType::Atlas)
  {
    xiiMemoryStreamWriter stream(&m_TextureAtlas);
    XII_SUCCEED_OR_RETURN(GenerateTextureAtlas(stream));
  }
  else
  {
    XII_SUCCEED_OR_RETURN(LoadInputImages());

    XII_SUCCEED_OR_RETURN(AdjustUsage(m_Descriptor.m_InputFiles[0], m_Descriptor.m_InputImages[0], m_Descriptor.m_Usage));

    xiiStringBuilder sUsage;
    xiiReflectionUtils::EnumerationToString(
      xiiGetStaticRTTI<xiiTexConvUsage>(), m_Descriptor.m_Usage.GetValue(), sUsage, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
    xiiLog::Info("-usage is '{}'", sUsage);

    XII_SUCCEED_OR_RETURN(ForceSRGBFormats());

    xiiUInt32 uiNumChannelsUsed = 0;
    XII_SUCCEED_OR_RETURN(DetectNumChannels(m_Descriptor.m_ChannelMappings, uiNumChannelsUsed));

    xiiEnum<xiiImageFormat> OutputImageFormat;

    XII_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, m_Descriptor.m_Usage, uiNumChannelsUsed));

    xiiLog::Info("Output image format is '{}'", xiiImageFormat::GetName(OutputImageFormat));

    xiiUInt32 uiTargetResolutionX = 0;
    xiiUInt32 uiTargetResolutionY = 0;

    XII_SUCCEED_OR_RETURN(DetermineTargetResolution(m_Descriptor.m_InputImages[0], OutputImageFormat, uiTargetResolutionX, uiTargetResolutionY));

    xiiLog::Info("Target resolution is '{} x {}'", uiTargetResolutionX, uiTargetResolutionY);

    XII_SUCCEED_OR_RETURN(ConvertAndScaleInputImages(uiTargetResolutionX, uiTargetResolutionY, m_Descriptor.m_Usage));

    XII_SUCCEED_OR_RETURN(ClampInputValues(m_Descriptor.m_InputImages, m_Descriptor.m_fMaxValue));

    if (m_Descriptor.m_Usage == xiiTexConvUsage::BumpMap)
    {
      XII_SUCCEED_OR_RETURN(ConvertToNormalMap(m_Descriptor.m_InputImages));
      m_Descriptor.m_Usage = xiiTexConvUsage::NormalMap;
    }

    xiiImage assembledImg;
    if (m_Descriptor.m_OutputType == xiiTexConvOutputType::Texture2D || m_Descriptor.m_OutputType == xiiTexConvOutputType::None)
    {
      XII_SUCCEED_OR_RETURN(Assemble2DTexture(m_Descriptor.m_InputImages[0].GetHeader(), assembledImg));

      XII_SUCCEED_OR_RETURN(DilateColor2D(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == xiiTexConvOutputType::Cubemap)
    {
      XII_SUCCEED_OR_RETURN(AssembleCubemap(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == xiiTexConvOutputType::Volume)
    {
      XII_SUCCEED_OR_RETURN(Assemble3DTexture(assembledImg));
    }

    XII_SUCCEED_OR_RETURN(AdjustHdrExposure(assembledImg));

    XII_SUCCEED_OR_RETURN(GenerateMipmaps(assembledImg, 0, uiNumChannelsUsed == 1 ? MipmapChannelMode::SingleChannel : MipmapChannelMode::AllChannels));

    XII_SUCCEED_OR_RETURN(PremultiplyAlpha(assembledImg));

    XII_SUCCEED_OR_RETURN(GenerateOutput(std::move(assembledImg), m_OutputImage, OutputImageFormat));

    XII_SUCCEED_OR_RETURN(GenerateThumbnailOutput(m_OutputImage, m_ThumbnailOutputImage, m_Descriptor.m_uiThumbnailOutputResolution));

    XII_SUCCEED_OR_RETURN(GenerateLowResOutput(m_OutputImage, m_LowResOutputImage, m_Descriptor.m_uiLowResMipmaps));
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::DetectNumChannels(xiiArrayPtr<const xiiTexConvSliceChannelMapping> channelMapping, xiiUInt32& uiNumChannels)
{
  XII_PROFILE_SCOPE("DetectNumChannels");

  uiNumChannels = 0;

  for (const auto& mapping : channelMapping)
  {
    for (xiiUInt32 i = 0; i < 4; ++i)
    {
      if (mapping.m_Channel[i].m_iInputImageIndex != -1 || mapping.m_Channel[i].m_ChannelValue == xiiTexConvChannelValue::Black)
      {
        uiNumChannels = xiiMath::Max(uiNumChannels, i + 1);
      }
    }
  }

  if (uiNumChannels == 0)
  {
    xiiLog::Error("No proper channel mapping provided.");
    return XII_FAILURE;
  }

  // special case handling to detect when the alpha channel will end up white anyway and thus uiNumChannels could be 3 instead of 4
  // which enables us to use more optimized output formats
  if (uiNumChannels == 4)
  {
    uiNumChannels = 3;

    for (const auto& mapping : channelMapping)
    {
      if (mapping.m_Channel[3].m_ChannelValue == xiiTexConvChannelValue::Black)
      {
        // sampling a texture without an alpha channel always returns 1, so to use all 0, we do need the channel
        uiNumChannels = 4;
        return XII_SUCCESS;
      }

      if (mapping.m_Channel[3].m_iInputImageIndex == -1)
      {
        // no fourth channel is needed for this
        continue;
      }

      xiiImage& img = m_Descriptor.m_InputImages[mapping.m_Channel[3].m_iInputImageIndex];

      const xiiUInt32 uiNumRequiredChannels = (xiiUInt32)mapping.m_Channel[3].m_ChannelValue + 1;
      const xiiUInt32 uiNumActualChannels   = xiiImageFormat::GetNumChannels(img.GetImageFormat());

      if (uiNumActualChannels < uiNumRequiredChannels)
      {
        // channel not available -> not needed
        continue;
      }

      if (img.Convert(xiiImageFormat::R32G32B32A32_FLOAT).Failed())
      {
        // can't convert -> will fail later anyway
        continue;
      }

      const float* pColors = img.GetPixelPointer<float>();
      pColors += (uiNumRequiredChannels - 1); // offset by 0 to 3 to read red, green, blue or alpha

      XII_ASSERT_DEV(img.GetRowPitch() == img.GetWidth() * sizeof(float) * 4, "Unexpected row pitch");

      for (xiiUInt32 i = 0; i < img.GetWidth() * img.GetHeight(); ++i)
      {
        if (!xiiMath::IsEqual(*pColors, 1.0f, 1.0f / 255.0f))
        {
          // value is not 1.0f -> the channel is needed
          uiNumChannels = 4;
          return XII_SUCCESS;
        }

        pColors += 4;
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::GenerateOutput(xiiImage&& src, xiiImage& dst, xiiEnum<xiiImageFormat> format)
{
  XII_PROFILE_SCOPE("GenerateOutput");

  dst.ResetAndMove(std::move(src));

  if (dst.Convert(format).Failed())
  {
    xiiLog::Error("Failed to convert result image to output format '{}'", xiiImageFormat::GetName(format));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::GenerateThumbnailOutput(const xiiImage& srcImg, xiiImage& dstImg, xiiUInt32 uiTargetRes)
{
  if (uiTargetRes == 0)
    return XII_SUCCESS;

  XII_PROFILE_SCOPE("GenerateThumbnailOutput");

  xiiUInt32 uiBestMip = 0;

  for (xiiUInt32 m = 0; m < srcImg.GetNumMipLevels(); ++m)
  {
    if (srcImg.GetWidth(m) <= uiTargetRes && srcImg.GetHeight(m) <= uiTargetRes)
    {
      uiBestMip = m;
      break;
    }

    uiBestMip = m;
  }

  xiiImage  scratch1, scratch2;
  xiiImage* pCurrentScratch = &scratch1;
  xiiImage* pOtherScratch   = &scratch2;

  pCurrentScratch->ResetAndCopy(srcImg.GetSubImageView(uiBestMip, 0));

  if (pCurrentScratch->GetWidth() > uiTargetRes || pCurrentScratch->GetHeight() > uiTargetRes)
  {
    if (pCurrentScratch->GetWidth() > pCurrentScratch->GetHeight())
    {
      const float fAspectRatio   = (float)pCurrentScratch->GetWidth() / (float)uiTargetRes;
      xiiUInt32   uiTargetHeight = (xiiUInt32)(pCurrentScratch->GetHeight() / fAspectRatio);

      uiTargetHeight = xiiMath::Max(uiTargetHeight, 4U);

      if (xiiImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetRes, uiTargetHeight).Failed())
      {
        xiiLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetRes,
                      uiTargetHeight);
        return XII_FAILURE;
      }
    }
    else
    {
      const float fAspectRatio  = (float)pCurrentScratch->GetHeight() / (float)uiTargetRes;
      xiiUInt32   uiTargetWidth = (xiiUInt32)(pCurrentScratch->GetWidth() / fAspectRatio);

      uiTargetWidth = xiiMath::Max(uiTargetWidth, 4U);

      if (xiiImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetWidth, uiTargetRes).Failed())
      {
        xiiLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetWidth,
                      uiTargetRes);
        return XII_FAILURE;
      }
    }

    xiiMath::Swap(pCurrentScratch, pOtherScratch);
  }

  dstImg.ResetAndMove(std::move(*pCurrentScratch));

  // we want to write out the thumbnail unchanged, so make sure it has a non-sRGB format
  dstImg.ReinterpretAs(xiiImageFormat::AsLinear(dstImg.GetImageFormat()));

  if (dstImg.Convert(xiiImageFormat::R8G8B8A8_UNORM).Failed())
  {
    xiiLog::Error("Failed to convert thumbnail image to RGBA8.");
    return XII_FAILURE;
  }

  // generate alpha checkerboard pattern
  {
    const float fTileSize = 16.0f;

    xiiColorLinearUB* pPixels  = dstImg.GetPixelPointer<xiiColorLinearUB>();
    const xiiUInt64   rowPitch = dstImg.GetRowPitch();

    xiiInt32 checkCounter = 0;
    xiiColor tiles[2]{xiiColor::LightGray, xiiColor::DarkGray};


    for (xiiUInt32 y = 0; y < dstImg.GetHeight(); ++y)
    {
      checkCounter = (xiiInt32)xiiMath::Floor(y / fTileSize);

      for (xiiUInt32 x = 0; x < dstImg.GetWidth(); ++x)
      {
        xiiColorLinearUB& col = pPixels[x];

        if (col.a < 255)
        {
          const xiiColor colF    = col;
          const xiiInt32 tileIdx = (checkCounter + (xiiInt32)xiiMath::Floor(x / fTileSize)) % 2;

          col = xiiMath::Lerp(tiles[tileIdx], colF, xiiMath::Sqrt(colF.a)).WithAlpha(colF.a);
        }
      }

      pPixels = xiiMemoryUtils::AddByteOffset(pPixels, static_cast<ptrdiff_t>(rowPitch));
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::GenerateLowResOutput(const xiiImage& srcImg, xiiImage& dstImg, xiiUInt32 uiLowResMip)
{
  if (uiLowResMip == 0)
    return XII_SUCCESS;

  XII_PROFILE_SCOPE("GenerateLowResOutput");

  // Do not early out here in this case, otherwise external processes may consider the output to be incomplete.
#if 0
  if (srcImg.GetNumMipLevels() <= uiLowResMip)
  {
    // probably just a low-resolution input image, do not generate output, but also do not fail
    xiiLog::Warning("LowRes image not generated, original resolution is already below threshold.");
    return XII_SUCCESS;
  }
#endif

  if (xiiImageUtils::ExtractLowerMipChain(srcImg, dstImg, uiLowResMip).Failed())
  {
    xiiLog::Error("Failed to extract low-res mipmap chain from output image.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}



XII_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Processor);
