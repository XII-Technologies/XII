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
      xiiImageConversionEntry(xiiImageFormat::NV12, xiiImageFormat::R8G8B8A8_UNORM_SRGB, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(xiiArrayPtr<xiiImageView> source, xiiImage target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    for (xiiUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const xiiUInt8* luma0  = source[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      const xiiUInt8* luma1  = source[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);
      const xiiUInt8* chroma = source[1].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y / 2);

      xiiUInt8* rgba0 = target.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      xiiUInt8* rgba1 = target.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);

      for (xiiUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        xiiVec3I32 p00 = YUV2RGB(xiiVec3I32(luma0[0], chroma[0], chroma[1]));
        xiiVec3I32 p01 = YUV2RGB(xiiVec3I32(luma0[1], chroma[0], chroma[1]));
        xiiVec3I32 p10 = YUV2RGB(xiiVec3I32(luma1[0], chroma[0], chroma[1]));
        xiiVec3I32 p11 = YUV2RGB(xiiVec3I32(luma1[1], chroma[0], chroma[1]));

        rgba0[0] = static_cast<xiiUInt8>(p00.x);
        rgba0[1] = static_cast<xiiUInt8>(p00.y);
        rgba0[2] = static_cast<xiiUInt8>(p00.z);
        rgba0[3] = static_cast<xiiUInt8>(0xff);
        rgba0[4] = static_cast<xiiUInt8>(p01.x);
        rgba0[5] = static_cast<xiiUInt8>(p01.y);
        rgba0[6] = static_cast<xiiUInt8>(p01.z);
        rgba0[7] = static_cast<xiiUInt8>(0xff);

        rgba1[0] = static_cast<xiiUInt8>(p10.x);
        rgba1[1] = static_cast<xiiUInt8>(p10.y);
        rgba1[2] = static_cast<xiiUInt8>(p10.z);
        rgba1[3] = static_cast<xiiUInt8>(0xff);
        rgba1[4] = static_cast<xiiUInt8>(p11.x);
        rgba1[5] = static_cast<xiiUInt8>(p11.y);
        rgba1[6] = static_cast<xiiUInt8>(p11.z);
        rgba1[7] = static_cast<xiiUInt8>(0xff);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
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
      xiiImageConversionEntry(xiiImageFormat::R8G8B8A8_UNORM_SRGB, xiiImageFormat::NV12, xiiImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual xiiResult ConvertPixels(const xiiImageView& source, xiiArrayPtr<xiiImage> target, xiiUInt32 uiNumPixelsX, xiiUInt32 uiNumPixelsY, xiiEnum<xiiGALResourceFormat> sourceFormat, xiiEnum<xiiGALResourceFormat> targetFormat) const override
  {
    XII_IGNORE_UNUSED(sourceFormat);
    XII_IGNORE_UNUSED(targetFormat);

    for (xiiUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const xiiUInt8* rgba0 = source.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      const xiiUInt8* rgba1 = source.GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);

      xiiUInt8* luma0  = target[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y);
      xiiUInt8* luma1  = target[0].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y + 1);
      xiiUInt8* chroma = target[1].GetPixelPointer<xiiUInt8>(0, 0, 0, 0, y / 2);

      for (xiiUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        xiiVec3I32 p00 = RGB2YUV(xiiVec3I32(rgba0[0], rgba0[1], rgba0[2]));
        xiiVec3I32 p01 = RGB2YUV(xiiVec3I32(rgba0[4], rgba0[5], rgba0[6]));
        xiiVec3I32 p10 = RGB2YUV(xiiVec3I32(rgba1[0], rgba1[1], rgba1[2]));
        xiiVec3I32 p11 = RGB2YUV(xiiVec3I32(rgba1[4], rgba1[5], rgba1[6]));

        luma0[0] = static_cast<xiiUInt8>(p00.x);
        luma0[1] = static_cast<xiiUInt8>(p01.x);
        luma1[0] = static_cast<xiiUInt8>(p10.x);
        luma1[1] = static_cast<xiiUInt8>(p11.x);

        xiiVec3I32 c = (p00 + p01 + p10 + p11);

        chroma[0] = static_cast<xiiUInt8>(c.y >> 2);
        chroma[1] = static_cast<xiiUInt8>(c.z >> 2);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return XII_SUCCESS;
  }
};

XII_STATICLINK_FORCE static xiiImageConversion_NV12_sRGB s_conversion_NV12_sRGB;
XII_STATICLINK_FORCE static xiiImageConversion_sRGB_NV12 s_conversion_sRGB_NV12;

XII_STATICLINK_FILE(Texture, Texture_Image_Conversions_PlanarConversions);
