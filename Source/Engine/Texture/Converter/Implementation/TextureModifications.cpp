/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Converter/TextureConverterProcessor.h>
#include <Texture/Image/ImageUtils.h>

xiiResult xiiTextureConverterProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == xiiTextureConverterUsage::Color)
  {
    for (const auto& mapping : m_Descriptor.m_ChannelMappings)
    {
      // do not enforce sRGB conversion for textures that are mapped to the alpha channel
      for (xiiUInt32 i = 0; i < 3; ++i)
      {
        const xiiInt32 iTex = mapping.m_Channel[i].m_iInputImageIndex;
        if (iTex != -1)
        {
          auto& img = m_Descriptor.m_InputImages[iTex];
          img.ReinterpretAs(xiiGALResourceFormat::AsSrgb(img.GetImageFormat()));
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::GenerateMipmaps(xiiImage& img, xiiUInt32 uiNumMips, MipmapChannelMode channelMode /*= MipmapChannelMode::AllChannels*/) const
{
  XII_PROFILE_SCOPE("GenerateMipmaps");

  xiiImageUtils::MipMapOptions opt;
  opt.m_numMipMaps = uiNumMips;

  xiiImageFilterBox                  filterLinear;
  xiiImageFilterSincWithKaiserWindow filterKaiser;

  switch (m_Descriptor.m_MipmapMode)
  {
    case xiiTextureConverterMipmapMode::None:
      return XII_SUCCESS;

    case xiiTextureConverterMipmapMode::Linear:
      opt.m_filter = &filterLinear;
      break;

    case xiiTextureConverterMipmapMode::Kaiser:
      opt.m_filter = &filterKaiser;
      break;
  }

  opt.m_addressModeU = m_Descriptor.m_AddressModeU;
  opt.m_addressModeV = m_Descriptor.m_AddressModeV;
  opt.m_addressModeW = m_Descriptor.m_AddressModeW;

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold   = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals = m_Descriptor.m_Usage == xiiTextureConverterUsage::NormalMap || m_Descriptor.m_Usage == xiiTextureConverterUsage::NormalMap_Inverted || m_Descriptor.m_Usage == xiiTextureConverterUsage::BumpMap;

  // Copy red to alpha channel if we only have a single channel input texture
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<xiiColor>();
    auto pData   = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->a = pData->r;
      ++pData;
    }
  }

  xiiImage scratch;
  xiiImageUtils::GenerateMipMaps(img, scratch, opt);
  img.ResetAndMove(std::move(scratch));

  if (img.GetMipLevelCount() <= 1)
  {
    xiiLog::Error("Mipmap generation failed.");
    return XII_FAILURE;
  }

  // Copy alpha channel back to red
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<xiiColor>();
    auto pData   = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->r = pData->a;
      ++pData;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::PremultiplyAlpha(xiiImage& image) const
{
  XII_PROFILE_SCOPE("PremultiplyAlpha");

  if (!m_Descriptor.m_bPremultiplyAlpha)
    return XII_SUCCESS;

  for (xiiColor& col : image.GetBlobPtr<xiiColor>())
  {
    col.r *= col.a;
    col.g *= col.a;
    col.b *= col.a;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::AdjustHdrExposure(xiiImage& img) const
{
  XII_PROFILE_SCOPE("AdjustHdrExposure");

  xiiImageUtils::ChangeExposure(img, m_Descriptor.m_fHdrExposureBias);
  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::ConvertToNormalMap(xiiArrayPtr<xiiImage> imgs) const
{
  XII_PROFILE_SCOPE("ConvertToNormalMap");

  for (xiiImage& img : imgs)
  {
    XII_SUCCEED_OR_RETURN(ConvertToNormalMap(img));
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::ConvertToNormalMap(xiiImage& bumpMap) const
{
  xiiGALTextureCreationDescription newImageHeader = bumpMap.GetDescription();
  newImageHeader.SetMipLevelCount(1);
  xiiImage newImage;
  newImage.ResetAndAlloc(newImageHeader);

  struct Accum
  {
    float x = 0.f;
    float y = 0.f;
  };
  xiiDelegate<Accum(xiiUInt32, xiiUInt32)> filterKernel;

  // we'll assume that both the input bump map and the new image are using
  // RGBA 32 bit floating point as an internal format which should be tightly packed
  XII_ASSERT_DEV(bumpMap.GetImageFormat() == xiiGALResourceFormat::RGBA32Float && bumpMap.GetRowPitch() % sizeof(xiiColor) == 0, "");

  const xiiColor* bumpPixels   = bumpMap.GetPixelPointer<xiiColor>(0, 0, 0, 0, 0, 0);
  const auto      getBumpPixel = [&](xiiUInt32 x, xiiUInt32 y) -> float {
    const xiiColor* ptr = bumpPixels + y * bumpMap.GetWidth() + x;
    return ptr->r;
  };

  xiiColor* newPixels   = newImage.GetPixelPointer<xiiColor>(0, 0, 0, 0, 0, 0);
  auto      getNewPixel = [&](xiiUInt32 x, xiiUInt32 y) -> xiiColor& {
    xiiColor* ptr = newPixels + y * newImage.GetWidth() + x;
    return *ptr;
  };

  switch (m_Descriptor.m_BumpMapFilter)
  {
    case xiiTextureConverterBumpMapFilter::Finite:
      filterKernel = [&](xiiUInt32 x, xiiUInt32 y) {
        constexpr float linearKernel[3] = {-1, 0, 1};

        Accum accum;
        for (int i = -1; i <= 1; ++i)
        {
          const xiiInt32 rx = xiiMath::Clamp(i + static_cast<xiiInt32>(x), 0, static_cast<xiiInt32>(newImage.GetWidth()) - 1);
          const xiiInt32 ry = xiiMath::Clamp(i + static_cast<xiiInt32>(y), 0, static_cast<xiiInt32>(newImage.GetHeight()) - 1);

          const float depthX = getBumpPixel(rx, y);
          const float depthY = getBumpPixel(x, ry);

          accum.x += depthX * linearKernel[i + 1];
          accum.y += depthY * linearKernel[i + 1];
        }

        return accum;
      };
      break;
    case xiiTextureConverterBumpMapFilter::Sobel:
      filterKernel = [&](xiiUInt32 x, xiiUInt32 y) {
        constexpr float kernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
        constexpr float weight       = 1.f / 4.f;

        Accum accum;
        for (xiiInt32 i = -1; i <= 1; ++i)
        {
          for (xiiInt32 j = -1; j <= 1; ++j)
          {
            const xiiInt32 rx = xiiMath::Clamp(j + static_cast<xiiInt32>(x), 0, static_cast<xiiInt32>(newImage.GetWidth()) - 1);
            const xiiInt32 ry = xiiMath::Clamp(i + static_cast<xiiInt32>(y), 0, static_cast<xiiInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
    case xiiTextureConverterBumpMapFilter::Scharr:
      filterKernel = [&](xiiUInt32 x, xiiUInt32 y) {
        constexpr float kernel[3][3] = {{-3, 0, 3}, {-10, 0, 10}, {-3, 0, 3}};
        constexpr float weight       = 1.f / 16.f;

        Accum accum;
        for (xiiInt32 i = -1; i <= 1; ++i)
        {
          for (xiiInt32 j = -1; j <= 1; ++j)
          {
            const xiiInt32 rx = xiiMath::Clamp(j + static_cast<xiiInt32>(x), 0, static_cast<xiiInt32>(newImage.GetWidth()) - 1);
            const xiiInt32 ry = xiiMath::Clamp(i + static_cast<xiiInt32>(y), 0, static_cast<xiiInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
  };

  for (xiiUInt32 y = 0; y < bumpMap.GetHeight(); ++y)
  {
    for (xiiUInt32 x = 0; x < bumpMap.GetWidth(); ++x)
    {
      Accum accum = filterKernel(x, y);

      xiiVec3 normal = xiiVec3(1.f, 0.f, accum.x).CrossRH(xiiVec3(0.f, 1.f, accum.y));
      normal.NormalizeIfNotZero(xiiVec3(0, 0, 1), 0.001f).IgnoreResult();
      normal.y = -normal.y;

      normal = normal * 0.5f + xiiVec3(0.5f);

      xiiColor& newPixel = getNewPixel(x, y);
      newPixel.SetRGBA(normal.x, normal.y, normal.z, 0.f);
    }
  }

  bumpMap.ResetAndMove(std::move(newImage));

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::ClampInputValues(xiiArrayPtr<xiiImage> images, float maxValue) const
{
  for (xiiImage& image : images)
  {
    XII_SUCCEED_OR_RETURN(ClampInputValues(image, maxValue));
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::ClampInputValues(xiiImage& image, float maxValue) const
{
  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  XII_ASSERT_DEV(image.GetImageFormat() == xiiGALResourceFormat::RGBA32Float && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<float>())
  {
    if (xiiMath::IsNaN(value))
    {
      value = 0.f;
    }
    else
    {
      value = xiiMath::Clamp(value, -maxValue, maxValue);
    }
  }

  return XII_SUCCESS;
}

static bool FillAvgImageColor(xiiImage& ref_img)
{
  xiiColor  avg          = xiiColor::MakeZero();
  xiiUInt32 uiValidCount = 0;

  for (const xiiColor& col : ref_img.GetBlobPtr<xiiColor>())
  {
    if (col.a > 0.0f)
    {
      avg += col;
      ++uiValidCount;
    }
  }

  if (uiValidCount == 0 || uiValidCount == ref_img.GetBlobPtr<xiiColor>().GetCount())
  {
    // nothing to do
    return false;
  }

  avg /= static_cast<float>(uiValidCount);
  avg.NormalizeToLdrRange();
  avg.a = 0.0f;

  for (xiiColor& col : ref_img.GetBlobPtr<xiiColor>())
  {
    if (col.a == 0.0f)
    {
      col = avg;
    }
  }

  return true;
}

static void ClearAlpha(xiiImage& ref_img, float fAlphaThreshold)
{
  for (xiiColor& col : ref_img.GetBlobPtr<xiiColor>())
  {
    if (col.a <= fAlphaThreshold)
    {
      col.a = 0.0f;
    }
  }
}

inline static xiiColor GetPixelValue(const xiiColor* pPixels, xiiInt32 iWidth, xiiInt32 x, xiiInt32 y)
{
  return pPixels[y * iWidth + x];
}

inline static void SetPixelValue(xiiColor* pPixels, xiiInt32 iWidth, xiiInt32 x, xiiInt32 y, const xiiColor& col)
{
  pPixels[y * iWidth + x] = col;
}

static xiiColor GetAvgColor(xiiColor* pPixels, xiiInt32 iWidth, xiiInt32 iHeight, xiiInt32 x, xiiInt32 y, float fMarkAlpha)
{
  xiiColor colAt = GetPixelValue(pPixels, iWidth, x, y);

  if (colAt.a > 0)
    return colAt;

  xiiColor  avg          = xiiColor::MakeZero();
  xiiUInt32 uiValidCount = 0;

  const xiiInt32 iRadius = 1;

  for (xiiInt32 cy = xiiMath::Max<xiiInt32>(0, y - iRadius); cy <= xiiMath::Min<xiiInt32>(y + iRadius, iHeight - 1); ++cy)
  {
    for (xiiInt32 cx = xiiMath::Max<xiiInt32>(0, x - iRadius); cx <= xiiMath::Min<xiiInt32>(x + iRadius, iWidth - 1); ++cx)
    {
      const xiiColor col = GetPixelValue(pPixels, iWidth, cx, cy);

      if (col.a > fMarkAlpha)
      {
        avg += col;
        ++uiValidCount;
      }
    }
  }

  if (uiValidCount == 0)
    return colAt;

  avg /= static_cast<float>(uiValidCount);
  avg.a = fMarkAlpha;

  return avg;
}

static void DilateColors(xiiColor* pPixels, xiiInt32 iWidth, xiiInt32 iHeight, float fMarkAlpha)
{
  for (xiiInt32 y = 0; y < iHeight; ++y)
  {
    for (xiiInt32 x = 0; x < iWidth; ++x)
    {
      const xiiColor avg = GetAvgColor(pPixels, iWidth, iHeight, x, y, fMarkAlpha);

      SetPixelValue(pPixels, iWidth, x, y, avg);
    }
  }
}

xiiResult xiiTextureConverterProcessor::DilateColor2D(xiiImage& img) const
{
  if (m_Descriptor.m_uiDilateColor == 0)
    return XII_SUCCESS;

  XII_PROFILE_SCOPE("DilateColor2D");

  if (!FillAvgImageColor(img))
    return XII_SUCCESS;

  const xiiUInt32 uiNumPasses = m_Descriptor.m_uiDilateColor;

  xiiColor*      pPixels = img.GetPixelPointer<xiiColor>();
  const xiiInt32 iWidth  = static_cast<xiiInt32>(img.GetWidth());
  const xiiInt32 iHeight = static_cast<xiiInt32>(img.GetHeight());

  for (xiiUInt32 pass = uiNumPasses; pass > 0; --pass)
  {
    const float fAlphaThreshold = (static_cast<float>(pass) / uiNumPasses) / 256.0f; // between 0 and 1/256
    DilateColors(pPixels, iWidth, iHeight, fAlphaThreshold);
  }

  ClearAlpha(img, 1.0f / 256.0f);

  return XII_SUCCESS;
}

xiiResult xiiTextureConverterProcessor::InvertNormalMap(xiiImage& image)
{
  if (m_Descriptor.m_Usage != xiiTextureConverterUsage::NormalMap_Inverted)
    return XII_SUCCESS;

  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  XII_ASSERT_DEV(image.GetImageFormat() == xiiGALResourceFormat::RGBA32Float && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<xiiColor>())
  {
    value.g = 1.0f - value.g;
  }

  return XII_SUCCESS;
}
