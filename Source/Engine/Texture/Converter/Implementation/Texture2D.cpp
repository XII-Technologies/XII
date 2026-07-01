/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Converter/TextureConverterProcessor.h>

xiiResult xiiTextureConverterProcessor::Assemble2DTexture(const xiiGALTextureCreationDescription& refImg, xiiImage& dst) const
{
  XII_PROFILE_SCOPE("Assemble2DTexture");

  dst.ResetAndAlloc(refImg);

  xiiColor* pPixelOut = dst.GetPixelPointer<xiiColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.m_Size.width, refImg.m_Size.height, pPixelOut);
}

xiiResult xiiTextureConverterProcessor::Assemble2DSlice(const xiiTextureConverterSliceChannelMapping& mapping, xiiUInt32 uiResolutionX, xiiUInt32 uiResolutionY, xiiColor* pPixelOut) const
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
        case xiiTextureConverterChannelValue::Red:
          pSourceValues[channel] = &pSourcePixel->r;
          break;
        case xiiTextureConverterChannelValue::Green:
          pSourceValues[channel] = &pSourcePixel->g;
          break;
        case xiiTextureConverterChannelValue::Blue:
          pSourceValues[channel] = &pSourcePixel->b;
          break;
        case xiiTextureConverterChannelValue::Alpha:
          pSourceValues[channel] = &pSourcePixel->a;
          break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }
    }
    else
    {
      uiSourceStrides[channel] = 0; // because of the constant value

      switch (cm.m_ChannelValue)
      {
        case xiiTextureConverterChannelValue::Black:
          pSourceValues[channel] = &fZero;
          break;

        case xiiTextureConverterChannelValue::White:
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

xiiResult xiiTextureConverterProcessor::DetermineTargetResolution(const xiiImage& image, xiiEnum<xiiGALResourceFormat> OutputImageFormat, xiiUInt32& out_uiTargetResolutionX, xiiUInt32& out_uiTargetResolutionY) const
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

  if (m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Volume)
  {
    xiiUInt32 uiScaleFactor = uiOrgResY / out_uiTargetResolutionY;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != xiiGALResourceFormat::Unknown && xiiGALTextureUtilities::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    const xiiUInt32 blockWidth = xiiGALTextureUtilities::GetBlockWidth(OutputImageFormat);

    xiiUInt32 currentWidth  = out_uiTargetResolutionX;
    xiiUInt32 currentHeight = out_uiTargetResolutionY;
    bool      issueWarning  = false;

    if (out_uiTargetResolutionX % blockWidth != 0)
    {
      out_uiTargetResolutionX = xiiMath::RoundUp(out_uiTargetResolutionX, static_cast<xiiUInt16>(blockWidth));
      issueWarning            = true;
    }

    xiiUInt32 blockHeight = xiiGALTextureUtilities::GetBlockHeight(OutputImageFormat);
    if (out_uiTargetResolutionY % blockHeight != 0)
    {
      out_uiTargetResolutionY = xiiMath::RoundUp(out_uiTargetResolutionY, static_cast<xiiUInt16>(blockHeight));
      issueWarning            = true;
    }

    if (issueWarning)
    {
      xiiLog::Warning("Chosen output image format is compressed, but target resolution does not fulfill block size requirements. {}x{} -> downscale {} / clamp({}, {}) -> {}x{}, adjusted to {}x{}",
                      uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution, currentWidth, currentHeight, out_uiTargetResolutionX, out_uiTargetResolutionY);
    }
  }

  return XII_SUCCESS;
}
