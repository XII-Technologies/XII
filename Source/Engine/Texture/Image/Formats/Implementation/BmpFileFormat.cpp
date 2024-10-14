#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/ImageConversion.h>

xiiBmpFileFormat g_bmpFormat;

enum xiiBmpCompression
{
  RGB       = 0L,
  RLE8      = 1L,
  RLE4      = 2L,
  BITFIELDS = 3L,
  JPEG      = 4L,
  PNG       = 5L,
};


#pragma pack(push, 1)
struct xiiBmpFileHeader
{
  xiiUInt16 m_type      = 0;
  xiiUInt32 m_size      = 0;
  xiiUInt16 m_reserved1 = 0;
  xiiUInt16 m_reserved2 = 0;
  xiiUInt32 m_offBits   = 0;
};
#pragma pack(pop)

struct xiiBmpFileInfoHeader
{
  xiiUInt32         m_size          = 0;
  xiiUInt32         m_width         = 0;
  xiiUInt32         m_height        = 0;
  xiiUInt16         m_planes        = 0;
  xiiUInt16         m_bitCount      = 0;
  xiiBmpCompression m_compression   = xiiBmpCompression::RGB;
  xiiUInt32         m_sizeImage     = 0;
  xiiUInt32         m_xPelsPerMeter = 0;
  xiiUInt32         m_yPelsPerMeter = 0;
  xiiUInt32         m_clrUsed       = 0;
  xiiUInt32         m_clrImportant  = 0;
};

struct xiiCIEXYZ
{
  int ciexyzX = 0;
  int ciexyzY = 0;
  int ciexyzZ = 0;
};

struct xiiCIEXYZTRIPLE
{
  xiiCIEXYZ ciexyzRed;
  xiiCIEXYZ ciexyzGreen;
  xiiCIEXYZ ciexyzBlue;
};

struct xiiBmpFileInfoHeaderV4
{
  xiiUInt32       m_redMask   = 0;
  xiiUInt32       m_greenMask = 0;
  xiiUInt32       m_blueMask  = 0;
  xiiUInt32       m_alphaMask = 0;
  xiiUInt32       m_csType    = 0;
  xiiCIEXYZTRIPLE m_endpoints;
  xiiUInt32       m_gammaRed   = 0;
  xiiUInt32       m_gammaGreen = 0;
  xiiUInt32       m_gammaBlue  = 0;
};

static_assert(sizeof(xiiCIEXYZTRIPLE) == 3 * 3 * 4);

// just to be on the safe side
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
static_assert(sizeof(xiiCIEXYZTRIPLE) == sizeof(CIEXYZTRIPLE));
#endif

struct xiiBmpFileInfoHeaderV5
{
  xiiUInt32 m_intent;
  xiiUInt32 m_profileData;
  xiiUInt32 m_profileSize;
  xiiUInt32 m_reserved;
};

static const xiiUInt16 xiiBmpFileMagic = 0x4D42u;

struct xiiBmpBgrxQuad
{
  XII_DECLARE_POD_TYPE();

  xiiBmpBgrxQuad() = default;

  xiiBmpBgrxQuad(xiiUInt8 uiRed, xiiUInt8 uiGreen, xiiUInt8 uiBlue) :
    m_blue(uiBlue), m_green(uiGreen), m_red(uiRed), m_reserved(0)
  {
  }

  xiiUInt8 m_blue;
  xiiUInt8 m_green;
  xiiUInt8 m_red;
  xiiUInt8 m_reserved;
};

xiiResult xiiBmpFileFormat::WriteImage(xiiStreamWriter& ref_stream, const xiiImageView& image, xiiStringView sFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  xiiImageFormat::Enum compatibleFormats[] = {
    xiiImageFormat::B8G8R8X8_UNORM,
    xiiImageFormat::B8G8R8A8_UNORM,
    xiiImageFormat::B8G8R8_UNORM,
    xiiImageFormat::B5G5R5X1_UNORM,
    xiiImageFormat::B5G6R5_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  xiiImageFormat::Enum format = xiiImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("No conversion from format '{0}' to a format suitable for BMP files known.", xiiImageFormat::GetName(image.GetImageFormat()));
    return XII_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    xiiImage convertedImage;
    if (xiiImageConversion::Convert(image, convertedImage, format) != XII_SUCCESS)
    {
      // This should never happen
      XII_ASSERT_DEV(false, "xiiImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return XII_FAILURE;
    }

    return WriteImage(ref_stream, convertedImage, sFileExtension);
  }

  xiiUInt64 uiRowPitch = image.GetRowPitch(0);

  xiiUInt32 uiHeight = image.GetHeight(0);

  xiiUInt64 dataSize = uiRowPitch * uiHeight;
  if (dataSize >= xiiMath::MaxValue<xiiUInt32>())
  {
    XII_ASSERT_DEV(false, "Size overflow in BMP file format.");
    return XII_FAILURE;
  }

  xiiBmpFileInfoHeader fileInfoHeader;
  fileInfoHeader.m_width    = image.GetWidth(0);
  fileInfoHeader.m_height   = uiHeight;
  fileInfoHeader.m_planes   = 1;
  fileInfoHeader.m_bitCount = static_cast<xiiUInt16>(xiiImageFormat::GetBitsPerPixel(format));

  fileInfoHeader.m_sizeImage = 0; // Can be zero unless we store the data compressed

  fileInfoHeader.m_xPelsPerMeter = 0;
  fileInfoHeader.m_yPelsPerMeter = 0;
  fileInfoHeader.m_clrUsed       = 0;
  fileInfoHeader.m_clrImportant  = 0;

  bool bWriteColorMask = false;

  // Prefer to write a V3 header
  xiiUInt32 uiHeaderVersion = 3;

  switch (format)
  {
    case xiiImageFormat::B8G8R8X8_UNORM:
    case xiiImageFormat::B5G5R5X1_UNORM:
    case xiiImageFormat::B8G8R8_UNORM:
      fileInfoHeader.m_compression = RGB;
      break;

    case xiiImageFormat::B8G8R8A8_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      uiHeaderVersion              = 4;
      break;

    case xiiImageFormat::B5G6R5_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      bWriteColorMask              = true;
      break;

    default:
      return XII_FAILURE;
  }

  XII_ASSERT_DEV(!bWriteColorMask || uiHeaderVersion <= 3, "Internal bug");

  xiiUInt32 uiFileInfoHeaderSize = sizeof(xiiBmpFileInfoHeader);
  xiiUInt32 uiHeaderSize         = sizeof(xiiBmpFileHeader);

  if (uiHeaderVersion >= 4)
  {
    uiFileInfoHeaderSize += sizeof(xiiBmpFileInfoHeaderV4);
  }
  else if (bWriteColorMask)
  {
    uiHeaderSize += 3 * sizeof(xiiUInt32);
  }

  uiHeaderSize += uiFileInfoHeaderSize;

  fileInfoHeader.m_size = uiFileInfoHeaderSize;

  xiiBmpFileHeader header;
  header.m_type      = xiiBmpFileMagic;
  header.m_size      = uiHeaderSize + static_cast<xiiUInt32>(dataSize);
  header.m_reserved1 = 0;
  header.m_reserved2 = 0;
  header.m_offBits   = uiHeaderSize;

  // Write all data
  if (ref_stream.WriteBytes(&header, sizeof(header)) != XII_SUCCESS)
  {
    xiiLog::Error("Failed to write header.");
    return XII_FAILURE;
  }

  if (ref_stream.WriteBytes(&fileInfoHeader, sizeof(fileInfoHeader)) != XII_SUCCESS)
  {
    xiiLog::Error("Failed to write fileInfoHeader.");
    return XII_FAILURE;
  }

  if (uiHeaderVersion >= 4)
  {
    xiiBmpFileInfoHeaderV4 fileInfoHeaderV4;
    memset(&fileInfoHeaderV4, 0, sizeof(fileInfoHeaderV4));

    fileInfoHeaderV4.m_redMask   = xiiImageFormat::GetRedMask(format);
    fileInfoHeaderV4.m_greenMask = xiiImageFormat::GetGreenMask(format);
    fileInfoHeaderV4.m_blueMask  = xiiImageFormat::GetBlueMask(format);
    fileInfoHeaderV4.m_alphaMask = xiiImageFormat::GetAlphaMask(format);

    if (ref_stream.WriteBytes(&fileInfoHeaderV4, sizeof(fileInfoHeaderV4)) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write fileInfoHeaderV4.");
      return XII_FAILURE;
    }
  }
  else if (bWriteColorMask)
  {
    struct
    {
      xiiUInt32 m_red;
      xiiUInt32 m_green;
      xiiUInt32 m_blue;
    } colorMask;


    colorMask.m_red   = xiiImageFormat::GetRedMask(format);
    colorMask.m_green = xiiImageFormat::GetGreenMask(format);
    colorMask.m_blue  = xiiImageFormat::GetBlueMask(format);

    if (ref_stream.WriteBytes(&colorMask, sizeof(colorMask)) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write colorMask.");
      return XII_FAILURE;
    }
  }

  const xiiUInt64 uiPaddedRowPitch = ((uiRowPitch - 1) / 4 + 1) * 4;
  // Write rows in reverse order
  for (xiiInt32 iRow = uiHeight - 1; iRow >= 0; iRow--)
  {
    if (ref_stream.WriteBytes(image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write data.");
      return XII_FAILURE;
    }

    xiiUInt8 zeroes[4] = {0, 0, 0, 0};
    if (ref_stream.WriteBytes(zeroes, uiPaddedRowPitch - uiRowPitch) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write data.");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

namespace
{
  xiiUInt32 ExtractBits(const void* pData, xiiUInt32 uiBitAddress, xiiUInt32 uiNumBits)
  {
    xiiUInt32 uiMask        = (1U << uiNumBits) - 1;
    xiiUInt32 uiByteAddress = uiBitAddress / 8;
    xiiUInt32 uiShiftAmount = 7 - (uiBitAddress % 8 + uiNumBits - 1);

    return (reinterpret_cast<const xiiUInt8*>(pData)[uiByteAddress] >> uiShiftAmount) & uiMask;
  }

  xiiResult ReadImageInfo(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, xiiBmpFileHeader& ref_fileHeader, xiiBmpFileInfoHeader& ref_fileInfoHeader, bool& ref_bIndexed, bool& ref_bCompressed, xiiUInt32& ref_uiBpp, xiiUInt32& ref_uiDataSize)
  {
    if (ref_stream.ReadBytes(&ref_fileHeader, sizeof(xiiBmpFileHeader)) != sizeof(xiiBmpFileHeader))
    {
      xiiLog::Error("Failed to read header data.");
      return XII_FAILURE;
    }

    // Some very old BMP variants may have different magic numbers, but we don't support them.
    if (ref_fileHeader.m_type != xiiBmpFileMagic)
    {
      xiiLog::Error("The file is not a recognized BMP file.");
      return XII_FAILURE;
    }

    // We expect at least header version 3
    xiiUInt32 uiHeaderVersion = 3;
    if (ref_stream.ReadBytes(&ref_fileInfoHeader, sizeof(xiiBmpFileInfoHeader)) != sizeof(xiiBmpFileInfoHeader))
    {
      xiiLog::Error("Failed to read header data (V3).");
      return XII_FAILURE;
    }

    int remainingHeaderBytes = ref_fileInfoHeader.m_size - sizeof(ref_fileInfoHeader);

    // File header shorter than expected - happens with corrupt files or e.g. with OS/2 BMP files which may have shorter headers
    if (remainingHeaderBytes < 0)
    {
      xiiLog::Error("The file header was shorter than expected.");
      return XII_FAILURE;
    }

    // Newer files may have a header version 4 (required for transparency)
    xiiBmpFileInfoHeaderV4 fileInfoHeaderV4;
    if (remainingHeaderBytes >= sizeof(xiiBmpFileInfoHeaderV4))
    {
      uiHeaderVersion = 4;
      if (ref_stream.ReadBytes(&fileInfoHeaderV4, sizeof(xiiBmpFileInfoHeaderV4)) != sizeof(xiiBmpFileInfoHeaderV4))
      {
        xiiLog::Error("Failed to read header data (V4).");
        return XII_FAILURE;
      }
      remainingHeaderBytes -= sizeof(xiiBmpFileInfoHeaderV4);
    }

    // Skip rest of header
    if (ref_stream.SkipBytes(remainingHeaderBytes) != remainingHeaderBytes)
    {
      xiiLog::Error("Failed to skip remaining header data.");
      return XII_FAILURE;
    }

    ref_uiBpp = ref_fileInfoHeader.m_bitCount;

    // Find target format to load the image
    xiiImageFormat::Enum format = xiiImageFormat::UNKNOWN;

    switch (ref_fileInfoHeader.m_compression)
    {
        // RGB or indexed data
      case RGB:
        switch (ref_uiBpp)
        {
          case 1:
          case 4:
          case 8:
            ref_bIndexed = true;

            // We always decompress indexed to BGRX, since the palette is specified in this format
            format = xiiImageFormat::B8G8R8X8_UNORM;
            break;

          case 16:
            format = xiiImageFormat::B5G5R5X1_UNORM;
            break;

          case 24:
            format = xiiImageFormat::B8G8R8_UNORM;
            break;

          case 32:
            format = xiiImageFormat::B8G8R8X8_UNORM;
        }
        break;

        // RGB data, but with the color masks specified in place of the palette
      case BITFIELDS:
        switch (ref_uiBpp)
        {
          case 16:
          case 32:
            // In case of old headers, the color masks appear after the header (and aren't counted as part of it)
            if (uiHeaderVersion < 4)
            {
              // Color masks (w/o alpha channel)
              struct
              {
                xiiUInt32 m_red;
                xiiUInt32 m_green;
                xiiUInt32 m_blue;
              } colorMask;

              if (ref_stream.ReadBytes(&colorMask, sizeof(colorMask)) != sizeof(colorMask))
              {
                return XII_FAILURE;
              }

              format = xiiImageFormat::FromPixelMask(colorMask.m_red, colorMask.m_green, colorMask.m_blue, 0, ref_uiBpp);
            }
            else
            {
              // For header version four and higher, the color masks are part of the header
              format = xiiImageFormat::FromPixelMask(fileInfoHeaderV4.m_redMask, fileInfoHeaderV4.m_greenMask, fileInfoHeaderV4.m_blueMask, fileInfoHeaderV4.m_alphaMask, ref_uiBpp);
            }

            break;
        }
        break;

      case RLE4:
        if (ref_uiBpp == 4)
        {
          ref_bIndexed    = true;
          ref_bCompressed = true;
          format          = xiiImageFormat::B8G8R8X8_UNORM;
        }
        break;

      case RLE8:
        if (ref_uiBpp == 8)
        {
          ref_bIndexed    = true;
          ref_bCompressed = true;
          format          = xiiImageFormat::B8G8R8X8_UNORM;
        }
        break;

      default:
        XII_ASSERT_NOT_IMPLEMENTED;
    }

    if (format == xiiImageFormat::UNKNOWN)
    {
      xiiLog::Error("Unknown or unsupported BMP encoding.");
      return XII_FAILURE;
    }

    const xiiUInt32 uiWidth = ref_fileInfoHeader.m_width;

    if (uiWidth > 65536)
    {
      xiiLog::Error("Image specifies width > 65536. Header corrupted?");
      return XII_FAILURE;
    }

    const xiiUInt32 uiHeight = ref_fileInfoHeader.m_height;

    if (uiHeight > 65536)
    {
      xiiLog::Error("Image specifies height > 65536. Header corrupted?");
      return XII_FAILURE;
    }

    ref_uiDataSize = ref_fileInfoHeader.m_sizeImage;

    if (ref_uiDataSize > 1024 * 1024 * 1024)
    {
      xiiLog::Error("Image specifies data size > 1GiB. Header corrupted?");
      return XII_FAILURE;
    }

    const int uiRowPitchIn = (uiWidth * ref_uiBpp + 31) / 32 * 4;

    if (ref_uiDataSize == 0)
    {
      if (ref_fileInfoHeader.m_compression != RGB)
      {
        xiiLog::Error("The data size wasn't specified in the header.");
        return XII_FAILURE;
      }
      ref_uiDataSize = uiRowPitchIn * uiHeight;
    }

    // Set image data
    ref_header.SetImageFormat(format);
    ref_header.SetNumMipLevels(1);
    ref_header.SetNumArrayIndices(1);
    ref_header.SetNumFaces(1);

    ref_header.SetWidth(uiWidth);
    ref_header.SetHeight(uiHeight);
    ref_header.SetDepth(1);

    return XII_SUCCESS;
  }

} // namespace

xiiResult xiiBmpFileFormat::ReadImageHeader(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, xiiStringView sFileExtension) const
{
  XII_PROFILE_SCOPE("xiiBmpFileFormat::ReadImage");

  xiiBmpFileHeader     fileHeader;
  xiiBmpFileInfoHeader fileInfoHeader;
  bool                 bIndexed = false, bCompressed = false;
  xiiUInt32            uiBpp      = 0;
  xiiUInt32            uiDataSize = 0;

  return ReadImageInfo(ref_stream, ref_header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize);
}

xiiResult xiiBmpFileFormat::ReadImage(xiiStreamReader& ref_stream, xiiImage& ref_image, xiiStringView sFileExtension) const
{
  XII_PROFILE_SCOPE("xiiBmpFileFormat::ReadImage");

  xiiBmpFileHeader     fileHeader;
  xiiImageHeader       header;
  xiiBmpFileInfoHeader fileInfoHeader;
  bool                 bIndexed = false, bCompressed = false;
  xiiUInt32            uiBpp      = 0;
  xiiUInt32            uiDataSize = 0;

  XII_SUCCEED_OR_RETURN(ReadImageInfo(ref_stream, header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize));

  ref_image.ResetAndAlloc(header);

  xiiUInt64 uiRowPitch = ref_image.GetRowPitch(0);

  const int uiRowPitchIn = (header.GetWidth() * uiBpp + 31) / 32 * 4;

  if (bIndexed)
  {
    // If no palette size was specified, the full available palette size will be used
    xiiUInt32 paletteSize = fileInfoHeader.m_clrUsed;
    if (paletteSize == 0)
    {
      paletteSize = 1U << uiBpp;
    }
    else if (paletteSize > 65536)
    {
      xiiLog::Error("Palette size > 65536.");
      return XII_FAILURE;
    }

    xiiDynamicArray<xiiBmpBgrxQuad> palette;
    palette.SetCountUninitialized(paletteSize);
    if (ref_stream.ReadBytes(&palette[0], paletteSize * sizeof(xiiBmpBgrxQuad)) != paletteSize * sizeof(xiiBmpBgrxQuad))
    {
      xiiLog::Error("Failed to read palette data.");
      return XII_FAILURE;
    }

    if (bCompressed)
    {
      // Compressed data is always in pairs of bytes
      if (uiDataSize % 2 != 0)
      {
        xiiLog::Error("The data size is not a multiple of 2 bytes in an RLE-compressed file.");
        return XII_FAILURE;
      }

      xiiDynamicArray<xiiUInt8> compressedData;
      compressedData.SetCountUninitialized(uiDataSize);

      if (ref_stream.ReadBytes(&compressedData[0], uiDataSize) != uiDataSize)
      {
        xiiLog::Error("Failed to read data.");
        return XII_FAILURE;
      }

      const xiiUInt8* pIn    = &compressedData[0];
      const xiiUInt8* pInEnd = pIn + uiDataSize;

      // Current output position
      xiiUInt32 uiRow = fileInfoHeader.m_height - 1;
      xiiUInt32 uiCol = 0;

      xiiBmpBgrxQuad* pLine = ref_image.GetPixelPointer<xiiBmpBgrxQuad>(0, 0, 0, 0, uiRow, 0);

      // Decode RLE data directly to RGBX
      while (pIn < pInEnd)
      {
        xiiUInt32 uiByte1 = *pIn++;
        xiiUInt32 uiByte2 = *pIn++;

        // Relative mode - the first byte specified a number of indices to be repeated, the second one the indices
        if (uiByte1 > 0)
        {
          // Clamp number of repetitions to row width.
          // The spec isn't clear on this point, but some files pad the number of encoded indices for some reason.
          uiByte1 = xiiMath::Min(uiByte1, fileInfoHeader.m_width - uiCol);

          if (uiBpp == 4)
          {
            // Alternate between two indices.
            for (xiiUInt32 uiRep = 0; uiRep < uiByte1 / 2; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
              pLine[uiCol++] = palette[uiByte2 & 0x0F];
            }

            // Repeat the first index for odd numbers of repetitions.
            if (uiByte1 & 1)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
            }
          }
          else /* if (uiBpp == 8) */
          {
            // Repeat a single index.
            for (xiiUInt32 uiRep = 0; uiRep < uiByte1; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2];
            }
          }
        }
        else
        {
          // Absolute mode - the first byte specifies a number of indices encoded separately, or is a special marker
          switch (uiByte2)
          {
              // End of line marker
            case 0:
            {

              // Fill up with palette entry 0
              while (uiCol < fileInfoHeader.m_width)
              {
                pLine[uiCol++] = palette[0];
              }

              // Begin next line
              uiCol = 0;
              uiRow--;
              pLine -= fileInfoHeader.m_width;
            }

            break;

              // End of image marker
            case 1:
              // Check that we really reached the end of the image.
              if (uiRow != 0 && uiCol != fileInfoHeader.m_height - 1)
              {
                xiiLog::Error("Unexpected end of image marker found.");
                return XII_FAILURE;
              }
              break;

            case 2:
              xiiLog::Error("Found a RLE compression position delta - this is not supported.");
              return XII_FAILURE;

            default:
              // Read uiByte2 number of indices

              // More data than fits into the image or can be read?
              if (uiCol + uiByte2 > fileInfoHeader.m_width || pIn + (uiByte2 + 1) / 2 > pInEnd)
              {
                return XII_FAILURE;
              }

              if (uiBpp == 4)
              {
                for (xiiUInt32 uiRep = 0; uiRep < uiByte2 / 2; uiRep++)
                {
                  xiiUInt32 uiIndices = *pIn++;
                  pLine[uiCol++]      = palette[uiIndices >> 4];
                  pLine[uiCol++]      = palette[uiIndices & 0x0f];
                }

                if (uiByte2 & 1)
                {
                  pLine[uiCol++] = palette[*pIn++ >> 4];
                }

                // Pad to word boundary
                pIn += (uiByte2 / 2 + uiByte2) & 1;
              }
              else /* if (uiBpp == 8) */
              {
                for (xiiUInt32 uiRep = 0; uiRep < uiByte2; uiRep++)
                {
                  pLine[uiCol++] = palette[*pIn++];
                }

                // Pad to word boundary
                pIn += uiByte2 & 1;
              }
          }
        }
      }
    }
    else
    {
      xiiDynamicArray<xiiUInt8> indexedData;
      indexedData.SetCountUninitialized(uiDataSize);
      if (ref_stream.ReadBytes(&indexedData[0], uiDataSize) != uiDataSize)
      {
        xiiLog::Error("Failed to read data.");
        return XII_FAILURE;
      }

      // Convert to non-indexed
      for (xiiUInt32 uiRow = 0; uiRow < fileInfoHeader.m_height; uiRow++)
      {
        xiiUInt8* pIn = &indexedData[uiRowPitchIn * uiRow];

        // Convert flipped vertically
        xiiBmpBgrxQuad* pOut = ref_image.GetPixelPointer<xiiBmpBgrxQuad>(0, 0, 0, 0, fileInfoHeader.m_height - uiRow - 1, 0);
        for (xiiUInt32 uiCol = 0; uiCol < ref_image.GetWidth(0); uiCol++)
        {
          xiiUInt32 uiIndex = ExtractBits(pIn, uiCol * uiBpp, uiBpp);
          if (uiIndex >= palette.GetCount())
          {
            xiiLog::Error("Image contains invalid palette indices.");
            return XII_FAILURE;
          }
          pOut[uiCol] = palette[uiIndex];
        }
      }
    }
  }
  else
  {
    // Format must match the number of bits in the file
    if (xiiImageFormat::GetBitsPerPixel(header.GetImageFormat()) != uiBpp)
    {
      xiiLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
                    uiBpp, xiiImageFormat::GetBitsPerPixel(header.GetImageFormat()), xiiImageFormat::GetName(header.GetImageFormat()));
      return XII_FAILURE;
    }

    // Skip palette data. Having a palette here doesn't make sense, but is not explicitly disallowed by the standard.
    xiiUInt32 paletteSize = fileInfoHeader.m_clrUsed * sizeof(xiiBmpBgrxQuad);
    if (ref_stream.SkipBytes(paletteSize) != paletteSize)
    {
      xiiLog::Error("Failed to skip palette data.");
      return XII_FAILURE;
    }

    // Read rows in reverse order
    for (xiiInt32 iRow = fileInfoHeader.m_height - 1; iRow >= 0; iRow--)
    {
      if (ref_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != uiRowPitch)
      {
        xiiLog::Error("Failed to read row data.");
        return XII_FAILURE;
      }
      if (ref_stream.SkipBytes(uiRowPitchIn - uiRowPitch) != uiRowPitchIn - uiRowPitch)
      {
        xiiLog::Error("Failed to skip row data.");
        return XII_FAILURE;
      }
    }
  }

  return XII_SUCCESS;
}

bool xiiBmpFileFormat::CanReadFileType(xiiStringView sExtension) const
{
  return sExtension.IsEqual_NoCase("bmp") || sExtension.IsEqual_NoCase("dib") || sExtension.IsEqual_NoCase("rle");
}

bool xiiBmpFileFormat::CanWriteFileType(xiiStringView sExtension) const
{
  return CanReadFileType(sExtension);
}

XII_STATICLINK_FILE(Texture, Texture_Image_Formats_BmpFileFormat);
