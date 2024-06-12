#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

xiiResult xiiTexConvProcessor::Assemble2DTexture(const xiiImageHeader& refImg, xiiImage& dst) const
{
  XII_PROFILE_SCOPE("Assemble2DTexture");

  dst.ResetAndAlloc(refImg);

  xiiColor* pPixelOut = dst.GetPixelPointer<xiiColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.GetWidth(), refImg.GetHeight(), pPixelOut);
}

xiiResult xiiTexConvProcessor::Assemble2DSlice(const xiiTexConvSliceChannelMapping& mapping, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiColor* pPixelOut) const
{
  xiiHybridArray<const xiiColor*, 16> pSource;
  for (xiiUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<xiiColor>();
  }

  const float  fZero              = 0.0f;
  const float  fOne               = 1.0f;
  const float* pSourceValues[4]   = {nullptr, nullptr, nullptr, nullptr};
  xiiUInt32    uiSourceStrides[4] = {0, 0, 0, 0};

  for (xiiUInt32 channel = 0; channel < 4; ++channel)
  {
    const auto&    cm         = mapping.m_Channel[channel];
    const xiiInt32 inputIndex = cm.m_iInputImageIndex;

    if (inputIndex != -1)
    {
      const xiiColor* pSourcePixel = pSource[inputIndex];
      uiSourceStrides[channel]     = 4;

      switch (cm.m_ChannelValue)
      {
        case xiiTexConvChannelValue::Red:
          pSourceValues[channel] = &pSourcePixel->r;
          break;
        case xiiTexConvChannelValue::Green:
          pSourceValues[channel] = &pSourcePixel->g;
          break;
        case xiiTexConvChannelValue::Blue:
          pSourceValues[channel] = &pSourcePixel->b;
          break;
        case xiiTexConvChannelValue::Alpha:
          pSourceValues[channel] = &pSourcePixel->a;
          break;

        default:
          XII_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
    else
    {
      uiSourceStrides[channel] = 0; // because of the constant value

      switch (cm.m_ChannelValue)
      {
        case xiiTexConvChannelValue::Black:
          pSourceValues[channel] = &fZero;
          break;

        case xiiTexConvChannelValue::White:
          pSourceValues[channel] = &fOne;
          break;

        default:
          if (channel == 3)
            pSourceValues[channel] = &fOne;
          else
            pSourceValues[channel] = &fZero;
          break;
      }
    }
  }

  const bool bFlip = m_Descriptor.m_bFlipHorizontal;

  if (!bFlip && (pSourceValues[0] + 1 == pSourceValues[1]) && (pSourceValues[1] + 1 == pSourceValues[2]) &&
      (pSourceValues[2] + 1 == pSourceValues[3]))
  {
    XII_PROFILE_SCOPE("Assemble2DSlice(memcpy)");

    xiiMemoryUtils::Copy<xiiColor>(pPixelOut, reinterpret_cast<const xiiColor*>(pSourceValues[0]), uiResolutionX * uiResolutionY);
  }
  else
  {
    XII_PROFILE_SCOPE("Assemble2DSlice(gather)");

    for (xiiUInt32 y = 0; y < uiResolutionY; ++y)
    {
      const xiiUInt32 pixelWriteRowOffset = uiResolutionX * (bFlip ? (uiResolutionY - y - 1) : y);

      for (xiiUInt32 x = 0; x < uiResolutionX; ++x)
      {
        float* dst = &pPixelOut[pixelWriteRowOffset + x].r;

        for (xiiUInt32 c = 0; c < 4; ++c)
        {
          dst[c] = *pSourceValues[c];
          pSourceValues[c] += uiSourceStrides[c];
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTexConvProcessor::DetermineTargetResolution(const xiiImage& image, xiiEnum<xiiImageFormat> OutputImageFormat, xiiUInt32& out_uiTargetResolutionX, xiiUInt32& out_uiTargetResolutionY) const
{
  XII_PROFILE_SCOPE("DetermineResolution");

  XII_ASSERT_DEV(out_uiTargetResolutionX == 0 && out_uiTargetResolutionY == 0, "Target resolution already determined");

  const xiiUInt32 uiOrgResX = image.GetWidth();
  const xiiUInt32 uiOrgResY = image.GetHeight();

  out_uiTargetResolutionX = uiOrgResX;
  out_uiTargetResolutionY = uiOrgResY;

  out_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  out_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  out_uiTargetResolutionX = xiiMath::Clamp(out_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  out_uiTargetResolutionY = xiiMath::Clamp(out_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  // keep original aspect ratio
  if (uiOrgResX > uiOrgResY)
  {
    out_uiTargetResolutionY = (out_uiTargetResolutionX * uiOrgResY) / uiOrgResX;
  }
  else if (uiOrgResX < uiOrgResY)
  {
    out_uiTargetResolutionX = (out_uiTargetResolutionY * uiOrgResX) / uiOrgResY;
  }

  if (m_Descriptor.m_OutputType == xiiTexConvOutputType::Volume)
  {
    xiiUInt32 uiScaleFactor = uiOrgResY / out_uiTargetResolutionY;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != xiiImageFormat::UNKNOWN && xiiImageFormat::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    const xiiUInt32 blockWidth = xiiImageFormat::GetBlockWidth(OutputImageFormat);

    xiiUInt32 currentWidth  = out_uiTargetResolutionX;
    xiiUInt32 currentHeight = out_uiTargetResolutionY;
    bool      issueWarning  = false;

    if (out_uiTargetResolutionX % blockWidth != 0)
    {
      out_uiTargetResolutionX = xiiMath::RoundUp(out_uiTargetResolutionX, static_cast<xiiUInt16>(blockWidth));
      issueWarning            = true;
    }

    xiiUInt32 blockHeight = xiiImageFormat::GetBlockHeight(OutputImageFormat);
    if (out_uiTargetResolutionY % blockHeight != 0)
    {
      out_uiTargetResolutionY = xiiMath::RoundUp(out_uiTargetResolutionY, static_cast<xiiUInt16>(blockHeight));
      issueWarning            = true;
    }

    if (issueWarning)
    {
      xiiLog::Warning("Chosen output image format is compressed, but target resolution does not fulfill block size requirements. {}x{} -> downscale {} / "
                      "clamp({}, {}) -> {}x{}, adjusted to {}x{}",
                      uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution, currentWidth, currentHeight, out_uiTargetResolutionX, out_uiTargetResolutionY);
    }
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture2D);
