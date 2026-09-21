/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Image.h>

XII_STATICLINK_FORCE static xiiImageFileFormatRegistrator<xiiDdsFileFormat> g_DdsFormat;

namespace
{
  struct xiiDdsPixelFormat
  {
    xiiUInt32 m_uiSize;
    xiiUInt32 m_uiFlags;
    xiiUInt32 m_uiFourCC;
    xiiUInt32 m_uiRGBBitCount;
    xiiUInt32 m_uiRBitMask;
    xiiUInt32 m_uiGBitMask;
    xiiUInt32 m_uiBBitMask;
    xiiUInt32 m_uiABitMask;
  };

  struct xiiDdsHeader
  {
    xiiUInt32         m_uiMagic;
    xiiUInt32         m_uiSize;
    xiiUInt32         m_uiFlags;
    xiiUInt32         m_uiHeight;
    xiiUInt32         m_uiWidth;
    xiiUInt32         m_uiPitchOrLinearSize;
    xiiUInt32         m_uiDepth;
    xiiUInt32         m_uiMipMapCount;
    xiiUInt32         m_uiReserved1[11];
    xiiDdsPixelFormat m_ddspf;
    xiiUInt32         m_uiCaps;
    xiiUInt32         m_uiCaps2;
    xiiUInt32         m_uiCaps3;
    xiiUInt32         m_uiCaps4;
    xiiUInt32         m_uiReserved2;
  };

  struct xiiDdsResourceDimension
  {
    enum Enum
    {
      TEXTURE1D = 2,
      TEXTURE2D = 3,
      TEXTURE3D = 4,
    };
  };

  struct xiiDdsResourceMiscFlags
  {
    enum Enum
    {
      TEXTURECUBE = 0x4,
    };
  };

  struct xiiDdsHeaderDxt10
  {
    xiiUInt32 m_uiDxgiFormat;
    xiiUInt32 m_uiResourceDimension;
    xiiUInt32 m_uiMiscFlag;
    xiiUInt32 m_uiArraySize;
    xiiUInt32 m_uiMiscFlags2;
  };

  struct xiiDdsdFlags
  {
    enum Enum
    {
      CAPS        = 0x000001,
      HEIGHT      = 0x000002,
      WIDTH       = 0x000004,
      PITCH       = 0x000008,
      PIXELFORMAT = 0x001000,
      MIPMAPCOUNT = 0x020000,
      LINEARSIZE  = 0x080000,
      DEPTH       = 0x800000,
    };
  };

  struct xiiDdpfFlags
  {
    enum Enum
    {
      ALPHAPIXELS = 0x00001,
      ALPHA       = 0x00002,
      FOURCC      = 0x00004,
      RGB         = 0x00040,
      YUV         = 0x00200,
      LUMINANCE   = 0x20000,
    };
  };

  struct xiiDdsCaps
  {
    enum Enum
    {
      COMPLEX = 0x000008,
      MIPMAP  = 0x400000,
      TEXTURE = 0x001000,
    };
  };

  struct xiiDdsCaps2
  {
    enum Enum
    {
      CUBEMAP           = 0x000200,
      CUBEMAP_POSITIVEX = 0x000400,
      CUBEMAP_NEGATIVEX = 0x000800,
      CUBEMAP_POSITIVEY = 0x001000,
      CUBEMAP_NEGATIVEY = 0x002000,
      CUBEMAP_POSITIVEZ = 0x004000,
      CUBEMAP_NEGATIVEZ = 0x008000,
      VOLUME            = 0x200000,
    };
  };

  /// Returns the red mask for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetRedMask(xiiEnum<xiiGALResourceFormat> format)
  {
    switch (format)
    {
      case xiiGALResourceFormat::RGBA8UNormalized:
      case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
        return 0x000000FFU;
      case xiiGALResourceFormat::BGRA8UNormalized:
      case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      case xiiGALResourceFormat::BGRX8UNormalized:
      case xiiGALResourceFormat::BGRX8UNormalizedSRGB:
        return 0x00FF0000U;
      case xiiGALResourceFormat::B5G6R5UNormalized:
        return 0x0000F800U;
      case xiiGALResourceFormat::B5G5R5A1UNormalized:
        return 0x00007C00U;
      default:
        return 0U;
    }
  }

  /// Returns the green mask for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetGreenMask(xiiEnum<xiiGALResourceFormat> format)
  {
    switch (format)
    {
      case xiiGALResourceFormat::RGBA8UNormalized:
      case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      case xiiGALResourceFormat::BGRA8UNormalized:
      case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      case xiiGALResourceFormat::BGRX8UNormalized:
      case xiiGALResourceFormat::BGRX8UNormalizedSRGB:
        return 0x0000FF00U;
      case xiiGALResourceFormat::B5G6R5UNormalized:
        return 0x000007E0U;
      case xiiGALResourceFormat::B5G5R5A1UNormalized:
        return 0x000003E0U;
      default:
        return 0U;
    }
  }

  /// Returns the blue mask for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetBlueMask(xiiEnum<xiiGALResourceFormat> format)
  {
    switch (format)
    {
      case xiiGALResourceFormat::RGBA8UNormalized:
      case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
        return 0x00FF0000U;
      case xiiGALResourceFormat::BGRA8UNormalized:
      case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
      case xiiGALResourceFormat::BGRX8UNormalized:
      case xiiGALResourceFormat::BGRX8UNormalizedSRGB:
        return 0x000000FFU;
      case xiiGALResourceFormat::B5G6R5UNormalized:
      case xiiGALResourceFormat::B5G5R5A1UNormalized:
        return 0x0000001FU;
      default:
        return 0U;
    }
  }

  /// Returns the alpha mask for a given texture format.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiUInt32 GetAlphaMask(xiiEnum<xiiGALResourceFormat> format)
  {
    switch (format)
    {
      case xiiGALResourceFormat::RGBA8UNormalized:
      case xiiGALResourceFormat::RGBA8UNormalizedSRGB:
      case xiiGALResourceFormat::BGRA8UNormalized:
      case xiiGALResourceFormat::BGRA8UNormalizedSRGB:
        return 0xFF000000U;
      case xiiGALResourceFormat::B5G5R5A1UNormalized:
        return 0x00008000U;
      default:
        return 0U;
    }
  }

  /// This returns the texture format for a given pixel mask and bits per pixel.
  [[nodiscard]] static XII_ALWAYS_INLINE xiiEnum<xiiGALResourceFormat> FromPixelMask(xiiUInt32 uiRedMask, xiiUInt32 uiGreenMask, xiiUInt32 uiBlueMask, xiiUInt32 uiAlphaMask, xiiUInt32 uiBitsPerPixel)
  {
    if (uiBitsPerPixel == 32U)
    {
      if (uiRedMask == 0x000000FFU && uiGreenMask == 0x0000FF00U && uiBlueMask == 0x00FF0000U && uiAlphaMask == 0xFF000000U)
        return xiiGALResourceFormat::RGBA8UNormalized;

      if (uiRedMask == 0x00FF0000U && uiGreenMask == 0x0000FF00U && uiBlueMask == 0x000000FFU && uiAlphaMask == 0xFF000000U)
        return xiiGALResourceFormat::BGRA8UNormalized;

      if (uiRedMask == 0x00FF0000U && uiGreenMask == 0x0000FF00U && uiBlueMask == 0x000000FFU && uiAlphaMask == 0x00000000U)
        return xiiGALResourceFormat::BGRX8UNormalized;
    }

    if (uiBitsPerPixel == 16U)
    {
      if (uiRedMask == 0x0000F800U && uiGreenMask == 0x000007E0U && uiBlueMask == 0x0000001FU)
        return xiiGALResourceFormat::B5G6R5UNormalized;

      if (uiRedMask == 0x00007C00U && uiGreenMask == 0x000003E0U && uiBlueMask == 0x0000001FU)
        return xiiGALResourceFormat::B5G5R5A1UNormalized;
    }

    return xiiGALResourceFormat::Unknown;
  }
} // namespace

static const xiiUInt32 xiiDdsMagic       = 0x20534444;
static const xiiUInt32 xiiDdsDxt10FourCc = 0x30315844;

static xiiResult ReadImageData(xiiStreamReader& inout_stream, xiiGALTextureCreationDescription& ref_imageHeader, xiiDdsHeader& ref_ddsHeader)
{
  if (inout_stream.ReadBytes(&ref_ddsHeader, sizeof(xiiDdsHeader)) != sizeof(xiiDdsHeader))
  {
    xiiLog::Error("Failed to read file header.");
    return XII_FAILURE;
  }

  if (ref_ddsHeader.m_uiMagic != xiiDdsMagic)
  {
    xiiLog::Error("The file is not a recognized DDS file.");
    return XII_FAILURE;
  }

  if (ref_ddsHeader.m_uiSize != 124)
  {
    xiiLog::Error("The file header size {0} doesn't match the expected size of 124.", ref_ddsHeader.m_uiSize);
    return XII_FAILURE;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if ((ref_ddsHeader.m_uiFlags & xiiDdsdFlags::WIDTH) == 0 || (ref_ddsHeader.m_uiFlags & xiiDdsdFlags::HEIGHT) == 0)
  {
    xiiLog::Error("The file header doesn't specify the mandatory WIDTH or HEIGHT flag.");
    return XII_FAILURE;
  }

  if ((ref_ddsHeader.m_uiCaps & xiiDdsCaps::TEXTURE) == 0)
  {
    xiiLog::Error("The file header doesn't specify the mandatory TEXTURE flag.");
    return XII_FAILURE;
  }

  ref_imageHeader.m_Size.width  = ref_ddsHeader.m_uiWidth;
  ref_imageHeader.m_Size.height = ref_ddsHeader.m_uiHeight;

  if (ref_ddsHeader.m_ddspf.m_uiSize != 32)
  {
    xiiLog::Error("The pixel format size {0} doesn't match the expected value of 32.", ref_ddsHeader.m_ddspf.m_uiSize);
    return XII_FAILURE;
  }

  xiiDdsHeaderDxt10 headerDxt10;

  xiiEnum<xiiGALResourceFormat> format = xiiGALResourceFormat::Unknown;

  // Data format specified in RGBA masks
  if ((ref_ddsHeader.m_ddspf.m_uiFlags & xiiDdpfFlags::ALPHAPIXELS) != 0 || (ref_ddsHeader.m_ddspf.m_uiFlags & xiiDdpfFlags::RGB) != 0 || (ref_ddsHeader.m_ddspf.m_uiFlags & xiiDdpfFlags::ALPHA) != 0)
  {
    format = FromPixelMask(ref_ddsHeader.m_ddspf.m_uiRBitMask, ref_ddsHeader.m_ddspf.m_uiGBitMask, ref_ddsHeader.m_ddspf.m_uiBBitMask, ref_ddsHeader.m_ddspf.m_uiABitMask, ref_ddsHeader.m_ddspf.m_uiRGBBitCount);

    if (format == xiiGALResourceFormat::Unknown)
    {
      xiiLog::Error("The pixel mask specified was not recognized (R: {0}, G: {1}, B: {2}, A: {3}, Bpp: {4}).", xiiArgU(ref_ddsHeader.m_ddspf.m_uiRBitMask, 1, false, 16), xiiArgU(ref_ddsHeader.m_ddspf.m_uiGBitMask, 1, false, 16), xiiArgU(ref_ddsHeader.m_ddspf.m_uiBBitMask, 1, false, 16), xiiArgU(ref_ddsHeader.m_ddspf.m_uiABitMask, 1, false, 16), ref_ddsHeader.m_ddspf.m_uiRGBBitCount);

      return XII_FAILURE;
    }

    // Verify that the format we found is correct
    if (xiiGALTextureUtilities::GetBitsPerPixel(format) != ref_ddsHeader.m_ddspf.m_uiRGBBitCount)
    {
      xiiLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.", ref_ddsHeader.m_ddspf.m_uiRGBBitCount, xiiGALTextureUtilities::GetBitsPerPixel(format), xiiArgEnum(format));

      return XII_FAILURE;
    }
  }
  else if ((ref_ddsHeader.m_ddspf.m_uiFlags & xiiDdpfFlags::FOURCC) != 0)
  {
    if (ref_ddsHeader.m_ddspf.m_uiFourCC == xiiDdsDxt10FourCc)
    {
      if (inout_stream.ReadBytes(&headerDxt10, sizeof(xiiDdsHeaderDxt10)) != sizeof(xiiDdsHeaderDxt10))
      {
        xiiLog::Error("Failed to read file header.");
        return XII_FAILURE;
      }

      format = xiiImageFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);

      if (format == xiiGALResourceFormat::Unknown)
      {
        xiiLog::Error("The DXGI format {0} has no equivalent image format.", headerDxt10.m_uiDxgiFormat);
        return XII_FAILURE;
      }
    }
    else
    {
      format = xiiImageFormatMappings::FromFourCc(ref_ddsHeader.m_ddspf.m_uiFourCC);

      if (format == xiiGALResourceFormat::Unknown)
      {
        xiiLog::Error("The FourCC code '{0}{1}{2}{3}' was not recognized.", xiiArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 0)), xiiArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 8)), xiiArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 16)), xiiArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 24)));
        return XII_FAILURE;
      }
    }
  }
  else
  {
    xiiLog::Error("The image format is neither specified as a pixel mask nor as a FourCC code.");
    return XII_FAILURE;
  }

  ref_imageHeader.m_Format = format;

  const bool bHasMipMaps = (ref_ddsHeader.m_uiCaps & xiiDdsCaps::MIPMAP) != 0;
  const bool bCubeMap    = (ref_ddsHeader.m_uiCaps2 & xiiDdsCaps2::CUBEMAP) != 0;
  const bool bVolume     = (ref_ddsHeader.m_uiCaps2 & xiiDdsCaps2::VOLUME) != 0;

  if (bHasMipMaps)
  {
    ref_imageHeader.m_uiMipLevels = ref_ddsHeader.m_uiMipMapCount;
  }

  // Cubemap and volume texture are mutually exclusive
  if (bVolume && bCubeMap)
  {
    xiiLog::Error("The header specifies both the VOLUME and CUBEMAP flags.");
    return XII_FAILURE;
  }

  if (bCubeMap)
  {
    ref_imageHeader.m_uiArraySizeOrDepth = 6;
  }
  else if (bVolume)
  {
    ref_imageHeader.m_uiArraySizeOrDepth = ref_ddsHeader.m_uiDepth;
  }

  return XII_SUCCESS;
}

xiiResult xiiDdsFileFormat::ReadImageDescription(xiiStreamReader& inout_stream, xiiGALTextureCreationDescription& ref_description, xiiStringView sFileExtension) const
{
  XII_IGNORE_UNUSED(sFileExtension);

  XII_PROFILE_SCOPE("xiiDdsFileFormat::ReadImageDescription");

  xiiDdsHeader ddsHeader;
  return ReadImageData(inout_stream, ref_description, ddsHeader);
}

xiiResult xiiDdsFileFormat::ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const
{
  XII_IGNORE_UNUSED(sFileExtension);

  XII_PROFILE_SCOPE("xiiDdsFileFormat::ReadImage");

  xiiGALTextureCreationDescription imageHeader;
  xiiDdsHeader                     ddsHeader;
  XII_SUCCEED_OR_RETURN(ReadImageData(inout_stream, imageHeader, ddsHeader));

  ref_image.ResetAndAlloc(imageHeader);

  const bool bPitch = (ddsHeader.m_uiFlags & xiiDdsdFlags::PITCH) != 0;

  // If pitch is specified, it must match the computed value
  if (bPitch && ref_image.GetRowPitch(0) != ddsHeader.m_uiPitchOrLinearSize)
  {
    xiiLog::Error("The row pitch specified in the header doesn't match the expected pitch.");
    return XII_FAILURE;
  }

  xiiUInt64 uiDataSize = ref_image.GetByteBlobPtr().GetCount();

  if (inout_stream.ReadBytes(ref_image.GetByteBlobPtr().GetPtr(), uiDataSize) != uiDataSize)
  {
    xiiLog::Error("Failed to read image data.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiDdsFileFormat::WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const
{
  XII_IGNORE_UNUSED(sFileExtension);

  const xiiEnum<xiiGALResourceFormat> format = image.GetImageFormat();
  const xiiUInt32                     uiBpp  = xiiGALTextureUtilities::GetBitsPerPixel(format);

  const xiiUInt32 uiNumFaces        = image.GetNumFaces();
  const xiiUInt32 uiNumMipLevels    = image.GetMipLevelCount();
  const xiiUInt32 uiNumArrayIndices = image.GetNumArrayIndices();

  const xiiUInt32 uiWidth  = image.GetWidth(0);
  const xiiUInt32 uiHeight = image.GetHeight(0);
  const xiiUInt32 uiDepth  = image.GetDepth(0);

  bool bHasMipMaps = uiNumMipLevels > 1;
  bool bVolume     = uiDepth > 1;
  bool bCubeMap    = uiNumFaces > 1;
  bool bArray      = uiNumArrayIndices > 1;

  bool bDxt10 = false;

  xiiDdsHeader      fileHeader;
  xiiDdsHeaderDxt10 headerDxt10;

  xiiMemoryUtils::ZeroFill(&fileHeader, 1);
  xiiMemoryUtils::ZeroFill(&headerDxt10, 1);

  fileHeader.m_uiMagic  = xiiDdsMagic;
  fileHeader.m_uiSize   = 124;
  fileHeader.m_uiWidth  = uiWidth;
  fileHeader.m_uiHeight = uiHeight;

  // Required in every .dds file.
  fileHeader.m_uiFlags = xiiDdsdFlags::WIDTH | xiiDdsdFlags::HEIGHT | xiiDdsdFlags::CAPS | xiiDdsdFlags::PIXELFORMAT;

  if (bHasMipMaps)
  {
    fileHeader.m_uiFlags |= xiiDdsdFlags::MIPMAPCOUNT;
    fileHeader.m_uiMipMapCount = uiNumMipLevels;
  }

  if (bVolume)
  {
    // Volume and array are incompatible
    if (bArray)
    {
      xiiLog::Error("The image is both an array and volume texture. This is not supported.");
      return XII_FAILURE;
    }

    fileHeader.m_uiFlags |= xiiDdsdFlags::DEPTH;
    fileHeader.m_uiDepth = uiDepth;
  }

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(image.GetImageFormat());

  if (formatProperties.IsCompressed())
  {
    fileHeader.m_uiFlags |= xiiDdsdFlags::LINEARSIZE;
    fileHeader.m_uiPitchOrLinearSize = 0; /// \todo sub-image size
  }
  else
  {
    fileHeader.m_uiFlags |= xiiDdsdFlags::PITCH;
    fileHeader.m_uiPitchOrLinearSize = static_cast<xiiUInt32>(image.GetRowPitch(0));
  }

  fileHeader.m_uiCaps = xiiDdsCaps::TEXTURE;

  if (bCubeMap)
  {
    if (uiNumFaces != 6)
    {
      xiiLog::Error("The image is a cubemap, but has {0} faces instead of the expected 6.", uiNumFaces);
      return XII_FAILURE;
    }

    if (bVolume)
    {
      xiiLog::Error("The image is both a cubemap and volume texture. This is not supported.");
      return XII_FAILURE;
    }

    fileHeader.m_uiCaps |= xiiDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= xiiDdsCaps2::CUBEMAP | xiiDdsCaps2::CUBEMAP_POSITIVEX | xiiDdsCaps2::CUBEMAP_NEGATIVEX | xiiDdsCaps2::CUBEMAP_POSITIVEY | xiiDdsCaps2::CUBEMAP_NEGATIVEY | xiiDdsCaps2::CUBEMAP_POSITIVEZ | xiiDdsCaps2::CUBEMAP_NEGATIVEZ;
  }

  if (bArray)
  {
    fileHeader.m_uiCaps |= xiiDdsCaps::COMPLEX;

    // Must be written as DXT10
    bDxt10 = true;
  }

  if (bVolume)
  {
    fileHeader.m_uiCaps |= xiiDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= xiiDdsCaps2::VOLUME;
  }

  if (bHasMipMaps)
  {
    fileHeader.m_uiCaps |= xiiDdsCaps::MIPMAP | xiiDdsCaps::COMPLEX;
  }

  fileHeader.m_ddspf.m_uiSize = 32;

  xiiUInt32 uiRedMask   = GetRedMask(format);
  xiiUInt32 uiGreenMask = GetGreenMask(format);
  xiiUInt32 uiBlueMask  = GetBlueMask(format);
  xiiUInt32 uiAlphaMask = GetAlphaMask(format);

  xiiUInt32 uiFourCc     = xiiImageFormatMappings::ToFourCc(format);
  xiiUInt32 uiDxgiFormat = xiiImageFormatMappings::ToDxgiFormat(format);

  // When not required to use a DXT10 texture, try to write a legacy DDS by specifying FourCC or pixel masks
  if (!bDxt10)
  {
    // The format has a known mask and we would also recognize it as the same when reading back in, since multiple formats may have the same pixel masks
    if ((uiRedMask | uiGreenMask | uiBlueMask | uiAlphaMask) && format == FromPixelMask(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask, uiBpp))
    {
      fileHeader.m_ddspf.m_uiFlags       = xiiDdpfFlags::ALPHAPIXELS | xiiDdpfFlags::RGB;
      fileHeader.m_ddspf.m_uiRBitMask    = uiRedMask;
      fileHeader.m_ddspf.m_uiGBitMask    = uiGreenMask;
      fileHeader.m_ddspf.m_uiBBitMask    = uiBlueMask;
      fileHeader.m_ddspf.m_uiABitMask    = uiAlphaMask;
      fileHeader.m_ddspf.m_uiRGBBitCount = xiiGALTextureUtilities::GetBitsPerPixel(format);
    }
    // The format has a known FourCC
    else if (uiFourCc != 0)
    {
      fileHeader.m_ddspf.m_uiFlags  = xiiDdpfFlags::FOURCC;
      fileHeader.m_ddspf.m_uiFourCC = uiFourCc;
    }
    else
    {
      // Fallback to DXT10 path
      bDxt10 = true;
    }
  }

  if (bDxt10)
  {
    // We must write a DXT10 file, but there is no matching DXGI_FORMAT - we could also try converting, but that is rarely intended when writing .dds
    if (uiDxgiFormat == 0)
    {
      xiiLog::Error("The image needs to be written as a DXT10 file, but no matching DXGI format was found for '{0}'.", xiiArgEnum(format));
      return XII_FAILURE;
    }

    fileHeader.m_ddspf.m_uiFlags  = xiiDdpfFlags::FOURCC;
    fileHeader.m_ddspf.m_uiFourCC = xiiDdsDxt10FourCc;

    headerDxt10.m_uiDxgiFormat = uiDxgiFormat;

    if (bVolume)
    {
      headerDxt10.m_uiResourceDimension = xiiDdsResourceDimension::TEXTURE3D;
    }
    else if (uiHeight > 1)
    {
      headerDxt10.m_uiResourceDimension = xiiDdsResourceDimension::TEXTURE2D;
    }
    else
    {
      headerDxt10.m_uiResourceDimension = xiiDdsResourceDimension::TEXTURE1D;
    }

    if (bCubeMap)
    {
      headerDxt10.m_uiMiscFlag = xiiDdsResourceMiscFlags::TEXTURECUBE;
    }

    // NOT multiplied by number of cubemap faces
    headerDxt10.m_uiArraySize = uiNumArrayIndices;

    // Can be used to describe the alpha channel usage, but automatically makes it incompatible with the D3DX libraries if not 0.
    headerDxt10.m_uiMiscFlags2 = 0;
  }

  if (inout_stream.WriteBytes(&fileHeader, sizeof(fileHeader)) != XII_SUCCESS)
  {
    xiiLog::Error("Failed to write image header.");
    return XII_FAILURE;
  }

  if (bDxt10)
  {
    if (inout_stream.WriteBytes(&headerDxt10, sizeof(headerDxt10)) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write image DX10 header.");
      return XII_FAILURE;
    }
  }

  if (inout_stream.WriteBytes(image.GetByteBlobPtr().GetPtr(), image.GetByteBlobPtr().GetCount()) != XII_SUCCESS)
  {
    xiiLog::Error("Failed to write image data.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

bool xiiDdsFileFormat::CanReadFileType(xiiStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("dds");
}

bool xiiDdsFileFormat::CanWriteFileType(xiiStringView sExtension) const
{
  return CanReadFileType(sExtension);
}

XII_STATICLINK_FILE(Texture, Texture_Image_Formats_DdsFileFormat);
