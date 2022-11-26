#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/TgaFileFormat.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>


xiiTgaFileFormat g_TgaFormat;

struct TgaImageDescriptor
{
  xiiUInt8 m_iAlphaBits : 4;
  xiiUInt8 m_bFlipH : 1;
  xiiUInt8 m_bFlipV : 1;
  xiiUInt8 m_Ignored : 2;
};

// see Wikipedia for details:
// http://de.wikipedia.org/wiki/Targa_Image_File
struct TgaHeader
{
  xiiInt8            m_iImageIDLength;
  xiiInt8            m_Ignored1;
  xiiInt8            m_ImageType;
  xiiInt8            m_Ignored2[9];
  xiiInt16           m_iImageWidth;
  xiiInt16           m_iImageHeight;
  xiiInt8            m_iBitsPerPixel;
  TgaImageDescriptor m_ImageDescriptor;
};

XII_CHECK_AT_COMPILETIME(sizeof(TgaHeader) == 18);


static inline xiiColorLinearUB GetPixelColor(const xiiImageView& image, xiiUInt32 x, xiiUInt32 y, const xiiUInt32 uiHeight)
{
  xiiColorLinearUB c(255, 255, 255, 255);

  const xiiUInt8* pPixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, x, uiHeight - y - 1, 0);

  switch (image.GetImageFormat())
  {
    case xiiImageFormat::R8G8B8A8_UNORM:
      c.r = pPixel[0];
      c.g = pPixel[1];
      c.b = pPixel[2];
      c.a = pPixel[3];
      break;
    case xiiImageFormat::B8G8R8A8_UNORM:
      c.a = pPixel[3];
      // fall through
    case xiiImageFormat::B8G8R8_UNORM:
    case xiiImageFormat::B8G8R8X8_UNORM:
      c.r = pPixel[2];
      c.g = pPixel[1];
      c.b = pPixel[0];
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return c;
}


xiiResult xiiTgaFileFormat::WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  xiiImageFormat::Enum compatibleFormats[] = {
    xiiImageFormat::R8G8B8A8_UNORM,
    xiiImageFormat::B8G8R8A8_UNORM,
    xiiImageFormat::B8G8R8X8_UNORM,
    xiiImageFormat::B8G8R8_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  xiiImageFormat::Enum format = xiiImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("No conversion from format '{0}' to a format suitable for TGA files known.", xiiImageFormat::GetName(image.GetImageFormat()));
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

    return WriteImage(stream, convertedImage, szFileExtension);
  }

  const bool bCompress = true;

  // Write the header
  {
    xiiUInt8 uiHeader[18];
    xiiMemoryUtils::ZeroFill(uiHeader, 18);

    if (!bCompress)
    {
      // uncompressed TGA
      uiHeader[2] = 2;
    }
    else
    {
      // compressed TGA
      uiHeader[2] = 10;
    }

    uiHeader[13] = static_cast<xiiUInt8>(image.GetWidth(0) / 256);
    uiHeader[15] = static_cast<xiiUInt8>(image.GetHeight(0) / 256);
    uiHeader[12] = static_cast<xiiUInt8>(image.GetWidth(0) % 256);
    uiHeader[14] = static_cast<xiiUInt8>(image.GetHeight(0) % 256);
    uiHeader[16] = static_cast<xiiUInt8>(xiiImageFormat::GetBitsPerPixel(image.GetImageFormat()));

    stream.WriteBytes(uiHeader, 18).IgnoreResult();
  }

  const bool bAlpha = image.GetImageFormat() != xiiImageFormat::B8G8R8_UNORM;

  const xiiUInt32 uiWidth  = image.GetWidth(0);
  const xiiUInt32 uiHeight = image.GetHeight(0);

  if (!bCompress)
  {
    // Write image uncompressed

    for (xiiUInt32 y = 0; y < uiWidth; ++y)
    {
      for (xiiUInt32 x = 0; x < uiHeight; ++x)
      {
        const xiiColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        stream << c.b;
        stream << c.g;
        stream << c.r;

        if (bAlpha)
          stream << c.a;
      }
    }
  }
  else
  {
    // write image RLE compressed

    xiiInt32 iRLE = 0;

    xiiColorLinearUB                      pc = {};
    xiiStaticArray<xiiColorLinearUB, 129> unequal;
    xiiInt32                              iEqual = 0;

    for (xiiUInt32 y = 0; y < uiHeight; ++y)
    {
      for (xiiUInt32 x = 0; x < uiWidth; ++x)
      {
        const xiiColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        if (iRLE == 0) // no comparison possible yet
        {
          pc   = c;
          iRLE = 1;
          unequal.PushBack(c);
        }
        else if (iRLE == 1) // has one value gathered for comparison
        {
          if (c == pc)
          {
            iRLE   = 2; // two values were equal
            iEqual = 2; // go into equal-mode
          }
          else
          {
            iRLE = 3; // two values were unequal
            pc   = c; // go into unequal-mode
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 2) // equal values
        {
          if ((c == pc) && (iEqual < 128))
            ++iEqual;
          else
          {
            xiiUInt8 uiRepeat = static_cast<xiiUInt8>(iEqual + 127);

            stream << uiRepeat;
            stream << pc.b;
            stream << pc.g;
            stream << pc.r;

            if (bAlpha)
              stream << pc.a;

            pc   = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 3)
        {
          if ((c != pc) && (unequal.GetCount() < 128))
          {
            unequal.PushBack(c);
            pc = c;
          }
          else
          {
            xiiUInt8 uiRepeat = (unsigned char)(unequal.GetCount()) - 1;
            stream << uiRepeat;

            for (xiiUInt32 i = 0; i < unequal.GetCount(); ++i)
            {
              stream << unequal[i].b;
              stream << unequal[i].g;
              stream << unequal[i].r;

              if (bAlpha)
                stream << unequal[i].a;
            }

            pc   = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
      }
    }


    if (iRLE == 1) // has one value gathered for comparison
    {
      xiiUInt8 uiRepeat = 0;

      stream << uiRepeat;
      stream << pc.b;
      stream << pc.g;
      stream << pc.r;

      if (bAlpha)
        stream << pc.a;
    }
    else if (iRLE == 2) // equal values
    {
      xiiUInt8 uiRepeat = static_cast<xiiUInt8>(iEqual + 127);

      stream << uiRepeat;
      stream << pc.b;
      stream << pc.g;
      stream << pc.r;

      if (bAlpha)
        stream << pc.a;
    }
    else if (iRLE == 3)
    {
      xiiUInt8 uiRepeat = (xiiUInt8)(unequal.GetCount()) - 1;
      stream << uiRepeat;

      for (xiiUInt32 i = 0; i < unequal.GetCount(); ++i)
      {
        stream << unequal[i].b;
        stream << unequal[i].g;
        stream << unequal[i].r;

        if (bAlpha)
          stream << unequal[i].a;
      }
    }
  }

  return XII_SUCCESS;
}

static xiiResult ReadImageHeaderImpl(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension, TgaHeader& tgaHeader)
{
  stream >> tgaHeader.m_iImageIDLength;
  stream >> tgaHeader.m_Ignored1;
  stream >> tgaHeader.m_ImageType;
  stream.ReadBytes(&tgaHeader.m_Ignored2, 9);
  stream >> tgaHeader.m_iImageWidth;
  stream >> tgaHeader.m_iImageHeight;
  stream >> tgaHeader.m_iBitsPerPixel;
  stream >> reinterpret_cast<xiiUInt8&>(tgaHeader.m_ImageDescriptor);

  // ignore optional data
  stream.SkipBytes(tgaHeader.m_iImageIDLength);

  const xiiUInt32 uiBytesPerPixel = tgaHeader.m_iBitsPerPixel / 8;

  // check whether width, height an BitsPerPixel are valid
  if ((tgaHeader.m_iImageWidth <= 0) || (tgaHeader.m_iImageHeight <= 0) || ((uiBytesPerPixel != 1) && (uiBytesPerPixel != 3) && (uiBytesPerPixel != 4)) || (tgaHeader.m_ImageType != 2 && tgaHeader.m_ImageType != 3 && tgaHeader.m_ImageType != 10 && tgaHeader.m_ImageType != 11))
  {
    xiiLog::Error("TGA has an invalid header: Width = {0}, Height = {1}, BPP = {2}, ImageType = {3}", tgaHeader.m_iImageWidth, tgaHeader.m_iImageHeight, tgaHeader.m_iBitsPerPixel, tgaHeader.m_ImageType);
    return XII_FAILURE;
  }

  // Set image data

  if (uiBytesPerPixel == 1)
    header.SetImageFormat(xiiImageFormat::R8_UNORM);
  else if (uiBytesPerPixel == 3)
    header.SetImageFormat(xiiImageFormat::B8G8R8_UNORM);
  else
    header.SetImageFormat(xiiImageFormat::B8G8R8A8_UNORM);

  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(1);
  header.SetNumFaces(1);

  header.SetWidth(tgaHeader.m_iImageWidth);
  header.SetHeight(tgaHeader.m_iImageHeight);
  header.SetDepth(1);

  return XII_SUCCESS;
}

xiiResult xiiTgaFileFormat::ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiTgaFileFormat::ReadImageHeader");

  TgaHeader tgaHeader;
  return ReadImageHeaderImpl(stream, header, szFileExtension, tgaHeader);
}

xiiResult xiiTgaFileFormat::ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiTgaFileFormat::ReadImage");

  xiiImageHeader imageHeader;
  TgaHeader      tgaHeader;
  XII_SUCCEED_OR_RETURN(ReadImageHeaderImpl(stream, imageHeader, szFileExtension, tgaHeader));

  const xiiUInt32 uiBytesPerPixel = tgaHeader.m_iBitsPerPixel / 8;

  image.ResetAndAlloc(imageHeader);

  if (tgaHeader.m_ImageType == 3)
  {
    // uncompressed greyscale

    const xiiUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (xiiInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (xiiInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (xiiInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else if (tgaHeader.m_ImageType == 2)
  {
    // uncompressed

    const xiiUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (xiiInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (xiiInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (xiiInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else
  {
    // compressed

    xiiInt32  iCurrentPixel = 0;
    const int iPixelCount   = tgaHeader.m_iImageWidth * tgaHeader.m_iImageHeight;

    do
    {
      xiiUInt8 uiChunkHeader = 0;

      stream >> uiChunkHeader;

      const xiiInt32 numToRead = (uiChunkHeader & 127) + 1;

      if (iCurrentPixel + numToRead > iPixelCount)
      {
        xiiLog::Error("TGA contents are invalid");
        return XII_FAILURE;
      }

      if (uiChunkHeader < 128)
      {
        // If the header is < 128, it means it is the number of RAW color packets minus 1
        // that follow the header
        // add 1 to get number of following color values

        // Read RAW color values
        for (xiiInt32 i = 0; i < numToRead; ++i)
        {
          const xiiInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const xiiInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          stream.ReadBytes(image.GetPixelPointer<void>(0, 0, 0, col, row, 0), uiBytesPerPixel);

          ++iCurrentPixel;
        }
      }
      else // chunk header > 128 RLE data, next color repeated (chunk header - 127) times
      {
        xiiUInt8 uiBuffer[4] = {255, 255, 255, 255};

        // read the current color
        stream.ReadBytes(uiBuffer, uiBytesPerPixel);

        // if it is a 24-Bit TGA (3 channels), the fourth channel stays at 255 all the time, since the 4th value in ucBuffer is never overwritten

        // copy the color into the image data as many times as dictated
        for (xiiInt32 i = 0; i < numToRead; ++i)
        {
          const xiiInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const xiiInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row    = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col    = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          xiiUInt8*  pPixel = image.GetPixelPointer<xiiUInt8>(0, 0, 0, col, row, 0);

          // BGR
          pPixel[0] = uiBuffer[0];

          if (uiBytesPerPixel > 1)
          {
            pPixel[1] = uiBuffer[1];
            pPixel[2] = uiBuffer[2];

            // Alpha
            if (uiBytesPerPixel == 4)
              pPixel[3] = uiBuffer[3];
          }

          ++iCurrentPixel;
        }
      }
    } while (iCurrentPixel < iPixelCount);
  }

  return XII_SUCCESS;
}

bool xiiTgaFileFormat::CanReadFileType(const char* szExtension) const
{
  return xiiStringUtils::IsEqual_NoCase(szExtension, "tga");
}

bool xiiTgaFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}



XII_STATICLINK_FILE(Texture, Texture_Image_Formats_TgaFileFormat);
