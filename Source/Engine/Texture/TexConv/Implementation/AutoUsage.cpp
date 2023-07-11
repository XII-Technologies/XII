#include <Texture/TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct FileSuffixToUsage
{
  const char*                 m_szSuffix = nullptr;
  const xiiTexConvUsage::Enum m_Usage    = xiiTexConvUsage::Auto;
};

static FileSuffixToUsage suffixToUsageMap[] = {
  //
  {"_d", xiiTexConvUsage::Color},       //
  {"diff", xiiTexConvUsage::Color},     //
  {"diffuse", xiiTexConvUsage::Color},  //
  {"albedo", xiiTexConvUsage::Color},   //
  {"col", xiiTexConvUsage::Color},      //
  {"color", xiiTexConvUsage::Color},    //
  {"emissive", xiiTexConvUsage::Color}, //
  {"emit", xiiTexConvUsage::Color},     //

  {"_n", xiiTexConvUsage::NormalMap},      //
  {"nrm", xiiTexConvUsage::NormalMap},     //
  {"norm", xiiTexConvUsage::NormalMap},    //
  {"normal", xiiTexConvUsage::NormalMap},  //
  {"normals", xiiTexConvUsage::NormalMap}, //

  {"_r", xiiTexConvUsage::Linear},        //
  {"_rgh", xiiTexConvUsage::Linear},      //
  {"_rough", xiiTexConvUsage::Linear},    //
  {"roughness", xiiTexConvUsage::Linear}, //

  {"_m", xiiTexConvUsage::Linear},       //
  {"_met", xiiTexConvUsage::Linear},     //
  {"_metal", xiiTexConvUsage::Linear},   //
  {"metallic", xiiTexConvUsage::Linear}, //

  {"_h", xiiTexConvUsage::Linear},     //
  {"height", xiiTexConvUsage::Linear}, //
  {"_disp", xiiTexConvUsage::Linear},  //

  {"_ao", xiiTexConvUsage::Linear},       //
  {"occlusion", xiiTexConvUsage::Linear}, //

  {"_alpha", xiiTexConvUsage::Linear}, //
};


static xiiTexConvUsage::Enum DetectUsageFromFilename(xiiStringView sFile)
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

  return xiiTexConvUsage::Auto;
}

static xiiTexConvUsage::Enum DetectUsageFromImage(const xiiImage& image)
{
  const xiiImageHeader&      header = image.GetHeader();
  const xiiImageFormat::Enum format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return xiiTexConvUsage::Auto;
  }

  if (xiiImageFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return xiiTexConvUsage::Color;
  }

  if (format == xiiImageFormat::BC5_UNORM)
  {
    return xiiTexConvUsage::NormalMap;
  }

  if (xiiImageFormat::GetBitsPerChannel(format, xiiImageFormatChannel::R) > 8 || format == xiiImageFormat::BC6H_SF16 ||
      format == xiiImageFormat::BC6H_UF16)
  {
    return xiiTexConvUsage::Hdr;
  }

  if (xiiImageFormat::GetNumChannels(format) <= 2)
  {
    return xiiTexConvUsage::Linear;
  }

  const xiiImage* pImgRGBA = &image;
  xiiImage        convertedRGBA;

  if (image.GetImageFormat() != xiiImageFormat::R8G8B8A8_UNORM)
  {
    pImgRGBA = &convertedRGBA;
    if (xiiImageConversion::Convert(image, convertedRGBA, xiiImageFormat::R8G8B8A8_UNORM).Failed())
    {
      // cannot convert to RGBA -> maybe some weird lookup table format
      return xiiTexConvUsage::Auto;
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
      return xiiTexConvUsage::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return xiiTexConvUsage::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return xiiTexConvUsage::NormalMap;
  }
}

xiiResult xiiTexConvProcessor::AdjustUsage(xiiStringView sFilename, const xiiImage& srcImg, xiiEnum<xiiTexConvUsage>& inout_Usage)
{
  XII_PROFILE_SCOPE("AdjustUsage");

  if (inout_Usage == xiiTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromFilename(sFilename);
  }

  if (inout_Usage == xiiTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromImage(srcImg);
  }

  if (inout_Usage == xiiTexConvUsage::Auto)
  {
    xiiLog::Error("Failed to deduce target format.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_AutoUsage);
