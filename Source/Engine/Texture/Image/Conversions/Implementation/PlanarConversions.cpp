/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageConversion.h>

namespace
{
  // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
  xiiVec3I32 RGB2YUV(xiiVec3I32 vRgb)
  {
    xiiVec3I32 yuv;
    yuv.x = ((66 * vRgb.x + 129 * vRgb.y + 25 * vRgb.z + 128) >> 8) + 16;
    yuv.y = ((-38 * vRgb.x - 74 * vRgb.y + 112 * vRgb.z + 128) >> 8) + 128;
    yuv.z = ((112 * vRgb.x - 94 * vRgb.y - 18 * vRgb.z + 128) >> 8) + 128;
    return yuv;
  }

  xiiVec3I32 YUV2RGB(xiiVec3I32 vYuv)
  {
    xiiVec3I32 rgb;

    xiiInt32 C = vYuv.x - 16;
    xiiInt32 D = vYuv.y - 128;
    xiiInt32 E = vYuv.z - 128;

    rgb.x = xiiMath::Clamp((298 * C + 409 * E + 128) >> 8, 0, 255);
    rgb.y = xiiMath::Clamp((298 * C - 100 * D - 208 * E + 128) >> 8, 0, 255);
    rgb.z = xiiMath::Clamp((298 * C + 516 * D + 128) >> 8, 0, 255);
    return rgb;
  }
} // namespace

struct xiiImageConversion_NV12_sRGB : public xiiImageConversionStepDeplanarize
{
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::NV12, xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiArrayPtr<xiiImageView> source, xiiImage target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    for (xiiUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const xiiUInt8* pLuma0  = source[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      const xiiUInt8* pLuma1  = source[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);
      const xiiUInt8* pChroma = source[1].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y / 2);

      xiiUInt8* pRGBA0 = target.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      xiiUInt8* pRGBA1 = target.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);

      for (xiiUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        xiiVec3I32 p00 = YUV2RGB(xiiVec3I32(pLuma0[0], pChroma[0], pChroma[1]));
        xiiVec3I32 p01 = YUV2RGB(xiiVec3I32(pLuma0[1], pChroma[0], pChroma[1]));
        xiiVec3I32 p10 = YUV2RGB(xiiVec3I32(pLuma1[0], pChroma[0], pChroma[1]));
        xiiVec3I32 p11 = YUV2RGB(xiiVec3I32(pLuma1[1], pChroma[0], pChroma[1]));

        pRGBA0[0] = static_cast<xiiUInt8>(p00.x);
        pRGBA0[1] = static_cast<xiiUInt8>(p00.y);
        pRGBA0[2] = static_cast<xiiUInt8>(p00.z);
        pRGBA0[3] = static_cast<xiiUInt8>(0xff);
        pRGBA0[4] = static_cast<xiiUInt8>(p01.x);
        pRGBA0[5] = static_cast<xiiUInt8>(p01.y);
        pRGBA0[6] = static_cast<xiiUInt8>(p01.z);
        pRGBA0[7] = static_cast<xiiUInt8>(0xff);

        pRGBA1[0] = static_cast<xiiUInt8>(p10.x);
        pRGBA1[1] = static_cast<xiiUInt8>(p10.y);
        pRGBA1[2] = static_cast<xiiUInt8>(p10.z);
        pRGBA1[3] = static_cast<xiiUInt8>(0xff);
        pRGBA1[4] = static_cast<xiiUInt8>(p11.x);
        pRGBA1[5] = static_cast<xiiUInt8>(p11.y);
        pRGBA1[6] = static_cast<xiiUInt8>(p11.z);
        pRGBA1[7] = static_cast<xiiUInt8>(0xff);

        pLuma0 += 2;
        pLuma1 += 2;
        pChroma += 2;

        pRGBA0 += 8;
        pRGBA1 += 8;
      }
    }

    return XII_SUCCESS;
  }
};

struct xiiImageConversion_sRGB_NV12 : public xiiImageConversionStepPlanarize
{
  virtual xiiArrayPtr<const xiiImageConversionEntry> GetSupportedConversions() const override
  {
    static xiiImageConversionEntry supportedConversions[] = {
      xiiImageConversionEntry(xiiGALResourceFormat::RGBA8UNormalizedSRGB, xiiGALResourceFormat::NV12, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(const xiiImageView& source, xiiArrayPtr<xiiImage> target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    for (xiiUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const xiiUInt8* pRGBA0 = source.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      const xiiUInt8* pRGBA1 = source.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);

      xiiUInt8* pLuma0  = target[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      xiiUInt8* pLuma1  = target[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);
      xiiUInt8* pChroma = target[1].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y / 2);

      for (xiiUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        xiiVec3I32 p00 = RGB2YUV(xiiVec3I32(pRGBA0[0], pRGBA0[1], pRGBA0[2]));
        xiiVec3I32 p01 = RGB2YUV(xiiVec3I32(pRGBA0[4], pRGBA0[5], pRGBA0[6]));
        xiiVec3I32 p10 = RGB2YUV(xiiVec3I32(pRGBA1[0], pRGBA1[1], pRGBA1[2]));
        xiiVec3I32 p11 = RGB2YUV(xiiVec3I32(pRGBA1[4], pRGBA1[5], pRGBA1[6]));

        pLuma0[0] = static_cast<xiiUInt8>(p00.x);
        pLuma0[1] = static_cast<xiiUInt8>(p01.x);
        pLuma1[0] = static_cast<xiiUInt8>(p10.x);
        pLuma1[1] = static_cast<xiiUInt8>(p11.x);

        xiiVec3I32 c = (p00 + p01 + p10 + p11);

        pChroma[0] = static_cast<xiiUInt8>(c.y >> 2);
        pChroma[1] = static_cast<xiiUInt8>(c.z >> 2);

        pLuma0 += 2;
        pLuma1 += 2;
        pChroma += 2;

        pRGBA0 += 8;
        pRGBA1 += 8;
      }
    }

    return XII_SUCCESS;
  }
};

XII_STATICLINK_FORCE static xiiImageConversion_NV12_sRGB s_conversion_NV12_sRGB;
XII_STATICLINK_FORCE static xiiImageConversion_sRGB_NV12 s_conversion_sRGB_NV12;

XII_STATICLINK_FILE(Texture, Texture_Image_Conversions_PlanarConversions);
