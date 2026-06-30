/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Texture/Converter/TextureConverterProcessor.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct FileSuffixToUsage
{
  const char*                 m_szSuffix = nullptr;
  const xiiTextureConverterUsage::Enum m_Usage    = xiiTextureConverterUsage::Auto;
};

static FileSuffixToUsage suffixToUsageMap[] = {
  //
  {"_d", xiiTextureConverterUsage::Color},       //
  {"diff", xiiTextureConverterUsage::Color},     //
  {"diffuse", xiiTextureConverterUsage::Color},  //
  {"albedo", xiiTextureConverterUsage::Color},   //
  {"col", xiiTextureConverterUsage::Color},      //
  {"color", xiiTextureConverterUsage::Color},    //
  {"emissive", xiiTextureConverterUsage::Color}, //
  {"emit", xiiTextureConverterUsage::Color},     //

  {"_n", xiiTextureConverterUsage::NormalMap},      //
  {"nrm", xiiTextureConverterUsage::NormalMap},     //
  {"norm", xiiTextureConverterUsage::NormalMap},    //
  {"normal", xiiTextureConverterUsage::NormalMap},  //
  {"normals", xiiTextureConverterUsage::NormalMap}, //

  {"_r", xiiTextureConverterUsage::Linear},        //
  {"_rgh", xiiTextureConverterUsage::Linear},      //
  {"_rough", xiiTextureConverterUsage::Linear},    //
  {"roughness", xiiTextureConverterUsage::Linear}, //

  {"_m", xiiTextureConverterUsage::Linear},       //
  {"_met", xiiTextureConverterUsage::Linear},     //
  {"_metal", xiiTextureConverterUsage::Linear},   //
  {"metallic", xiiTextureConverterUsage::Linear}, //

  {"_h", xiiTextureConverterUsage::Linear},     //
  {"height", xiiTextureConverterUsage::Linear}, //
  {"_disp", xiiTextureConverterUsage::Linear},  //

  {"_ao", xiiTextureConverterUsage::Linear},       //
  {"occlusion", xiiTextureConverterUsage::Linear}, //

  {"_alpha", xiiTextureConverterUsage::Linear}, //
};


static xiiTextureConverterUsage::Enum DetectUsageFromFilename(xiiStringView sFile)
{
  xiiStringBuilder name = xiiPathUtils::GetFileName(sFile);
  name.ToLower();

  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(suffixToUsageMap); ++i)
  {
    if (name.EndsWith_NoCase(suffixToUsageMap[i].m_szSuffix))
    {
      return suffixToUsageMap[i].m_Usage;
    }
  }

  return xiiTextureConverterUsage::Auto;
}

static xiiTextureConverterUsage::Enum DetectUsageFromImage(const xiiImage& image)
{
  const xiiGALTextureCreationDescription&      header = image.GetDescription();
  const xiiEnum<xiiGALResourceFormat> format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return xiiTextureConverterUsage::Auto;
  }

  if (xiiGALResourceFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return xiiTextureConverterUsage::Color;
  }

  if (format == xiiGALResourceFormat::BC5UNormalized)
  {
    return xiiTextureConverterUsage::NormalMap;
  }

  if (xiiGALTextureUtilities::GetBitsPerComponent(format, 0) > 8 || format == xiiGALResourceFormat::BC6HSF16 ||
      format == xiiGALResourceFormat::BC6HUF16)
  {
    return xiiTextureConverterUsage::Hdr;
  }

  if (xiiGALTextureUtilities::GetComponentCount(format) <= 2)
  {
    return xiiTextureConverterUsage::Linear;
  }

  const xiiImage* pImgRGBA = &image;
  xiiImage        convertedRGBA;

  if (image.GetImageFormat() != xiiGALResourceFormat::RGBA8UNormalized)
  {
    pImgRGBA = &convertedRGBA;
    if (xiiImageConversion::Convert(image, convertedRGBA, xiiGALResourceFormat::RGBA8UNormalized).Failed())
    {
      // cannot convert to RGBA -> maybe some weird lookup table format
      return xiiTextureConverterUsage::Auto;
    }
  }

  // analyze the image content
  {
    xiiUInt32 sr = 0;
    xiiUInt32 sg = 0;
    xiiUInt32 sb = 0;

    xiiUInt32 uiExtremeNormals = 0;

    xiiUInt32 uiNumPixels = header.GetWidth() * header.GetHeight();
    XII_ASSERT_DEBUG(uiNumPixels > 0, "Unexpected empty image.");

    // Sample no more than 10000 pixels
    xiiUInt32 uiStride = xiiMath::Max(1U, uiNumPixels / 10000);
    uiNumPixels /= uiStride;

    const xiiUInt8* pPixel = pImgRGBA->GetPixelPointer<xiiUInt8>();

    for (xiiUInt32 uiPixel = 0; uiPixel < uiNumPixels; ++uiPixel)
    {
      // definitely not a normal map, if any Z vector points that much backwards
      uiExtremeNormals += (pPixel[2] < 90) ? 1 : 0;

      sr += pPixel[0];
      sg += pPixel[1];
      sb += pPixel[2];

      pPixel += 4 * uiStride;
    }

    // the average color in the image
    sr /= uiNumPixels; // NOLINT: Not a division by zero.
    sg /= uiNumPixels; // NOLINT: Not a division by zero.
    sb /= uiNumPixels; // NOLINT: Not a division by zero.

    if (sb < 230 || sr < 128 - 60 || sr > 128 + 60 || sg < 128 - 60 || sg > 128 + 60)
    {
      // if the average color is not a proper hue of blue, it cannot be a normal map
      return xiiTextureConverterUsage::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return xiiTextureConverterUsage::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return xiiTextureConverterUsage::NormalMap;
  }
}

xiiResult xiiTextureConverterProcessor::AdjustUsage(xiiStringView sFilename, const xiiImage& srcImg, xiiEnum<xiiTextureConverterUsage>& inout_Usage)
{
  XII_PROFILE_SCOPE("AdjustUsage");

  if (inout_Usage == xiiTextureConverterUsage::Auto)
  {
    inout_Usage = DetectUsageFromFilename(sFilename);
  }

  if (inout_Usage == xiiTextureConverterUsage::Auto)
  {
    inout_Usage = DetectUsageFromImage(srcImg);
  }

  if (inout_Usage == xiiTextureConverterUsage::Auto)
  {
    xiiLog::Error("Failed to deduce target format.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}
