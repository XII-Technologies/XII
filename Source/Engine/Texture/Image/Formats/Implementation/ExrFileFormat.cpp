#include <Texture/TexturePCH.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)

#  include <Texture/Image/Formats/ExrFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <tinyexr/tinyexr.h>

xiiExrFileFormat g_ExrFileFormat;

xiiResult ReadImageData(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& fileBuffer, xiiImageHeader& header, EXRHeader& exrHeader, EXRImage& exrImage)
{
  // read the entire file to memory
  xiiStreamUtils::ReadAllAndAppend(stream, fileBuffer);

  // read the EXR version
  EXRVersion exrVersion;

  if (ParseEXRVersionFromMemory(&exrVersion, fileBuffer.GetData(), fileBuffer.GetCount()) != 0)
  {
    xiiLog::Error("Invalid EXR file: Cannot read version.");
    return XII_FAILURE;
  }

  if (exrVersion.multipart)
  {
    xiiLog::Error("Invalid EXR file: Multi-part formats are not supported.");
    return XII_FAILURE;
  }

  // read the EXR header
  const char* err = nullptr;
  if (ParseEXRHeaderFromMemory(&exrHeader, &exrVersion, fileBuffer.GetData(), fileBuffer.GetCount(), &err) != 0)
  {
    xiiLog::Error("Invalid EXR file: '{0}'", err);
    FreeEXRErrorMessage(err);
    return XII_FAILURE;
  }

  for (int c = 1; c < exrHeader.num_channels; ++c)
  {
    if (exrHeader.pixel_types[c - 1] != exrHeader.pixel_types[c])
    {
      xiiLog::Error("Unsupported EXR file: all channels should have the same size.");
      break;
    }
  }

  if (LoadEXRImageFromMemory(&exrImage, &exrHeader, fileBuffer.GetData(), fileBuffer.GetCount(), &err) != 0)
  {
    xiiLog::Error("Invalid EXR file: '{0}'", err);

    FreeEXRHeader(&exrHeader);
    FreeEXRErrorMessage(err);
    return XII_FAILURE;
  }

  xiiImageFormat::Enum imageFormat = xiiImageFormat::UNKNOWN;

  switch (exrHeader.num_channels)
  {
    case 1:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = xiiImageFormat::R32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = xiiImageFormat::R16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = xiiImageFormat::R32_UINT;
          break;
      }

      break;
    }

    case 2:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = xiiImageFormat::R32G32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = xiiImageFormat::R16G16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = xiiImageFormat::R32G32_UINT;
          break;
      }

      break;
    }

    case 3:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = xiiImageFormat::R32G32B32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = xiiImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = xiiImageFormat::R32G32B32_UINT;
          break;
      }

      break;
    }

    case 4:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = xiiImageFormat::R32G32B32A32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = xiiImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          xiiImageFormat::R32G32B32A32_UINT;
          break;
      }

      break;
    }
  }

  if (imageFormat == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Unsupported EXR file: {}-channel files with format '{}' are unsupported.", exrHeader.num_channels, exrHeader.pixel_types[0]);
    return XII_FAILURE;
  }

  header.SetWidth(exrImage.width);
  header.SetHeight(exrImage.height);
  header.SetImageFormat(imageFormat);

  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(1);
  header.SetNumFaces(1);
  header.SetDepth(1);

  return XII_SUCCESS;
}

xiiResult xiiExrFileFormat::ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiExrFileFormat::ReadImageHeader");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  XII_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  XII_SCOPE_EXIT(FreeEXRImage(&exrImage));

  xiiDynamicArray<xiiUInt8> fileBuffer;
  return ReadImageData(stream, fileBuffer, header, exrHeader, exrImage);
}

static void CopyChannel(xiiUInt8* pDst, const xiiUInt8* pSrc, xiiUInt32 uiNumElements, xiiUInt32 uiElementSize, xiiUInt32 uiDstStride)
{
  if (uiDstStride == uiElementSize)
  {
    // fast path to copy everything in one operation
    // this only happens for single-channel formats
    xiiMemoryUtils::RawByteCopy(pDst, pSrc, uiNumElements * uiElementSize);
  }
  else
  {
    for (xiiUInt32 i = 0; i < uiNumElements; ++i)
    {
      xiiMemoryUtils::RawByteCopy(pDst, pSrc, uiElementSize);

      pSrc = xiiMemoryUtils::AddByteOffset(pSrc, uiElementSize);
      pDst = xiiMemoryUtils::AddByteOffset(pDst, uiDstStride);
    }
  }
}

xiiResult xiiExrFileFormat::ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiExrFileFormat::ReadImage");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  XII_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  XII_SCOPE_EXIT(FreeEXRImage(&exrImage));

  xiiImageHeader            header;
  xiiDynamicArray<xiiUInt8> fileBuffer;

  XII_SUCCEED_OR_RETURN(ReadImageData(stream, fileBuffer, header, exrHeader, exrImage));

  image.ResetAndAlloc(header);

  const xiiUInt32 uiPixelCount     = header.GetWidth() * header.GetHeight();
  const xiiUInt32 uiNumDstChannels = xiiImageFormat::GetNumChannels(header.GetImageFormat());
  const xiiUInt32 uiNumSrcChannels = exrHeader.num_channels;

  xiiUInt32 uiSrcStride = 0;
  switch (exrHeader.pixel_types[0])
  {
    case TINYEXR_PIXELTYPE_FLOAT:
      uiSrcStride = sizeof(float);
      break;

    case TINYEXR_PIXELTYPE_HALF:
      uiSrcStride = sizeof(float) / 2;
      break;

    case TINYEXR_PIXELTYPE_UINT:
      uiSrcStride = sizeof(xiiUInt32);
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }


  // src and dst element size is always identical, we only copy from float->float, half->half or uint->uint
  // however data is interleaved in dst, but not interleaved in src

  const xiiUInt32 uiDstStride = uiSrcStride * uiNumDstChannels;
  xiiUInt8*       pDstBytes   = image.GetBlobPtr<xiiUInt8>().GetPtr();

  if (uiNumDstChannels > uiNumSrcChannels)
  {
    // if we have more dst channels, than in the input data, fill everything with white
    xiiMemoryUtils::PatternFill(pDstBytes, 0xFF, uiDstStride * uiPixelCount);
  }

  xiiUInt32 c = 0;

  if (uiNumSrcChannels >= 4)
  {
    const xiiUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 4)
    {
      // copy to alpha
      CopyChannel(pDstBytes + 3 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 3)
  {
    const xiiUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 3)
    {
      // copy to blue
      CopyChannel(pDstBytes + 2 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 2)
  {
    const xiiUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 2)
    {
      // copy to green
      CopyChannel(pDstBytes + uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 1)
  {
    const xiiUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 1)
    {
      // copy to red
      CopyChannel(pDstBytes, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiExrFileFormat::WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}

bool xiiExrFileFormat::CanReadFileType(const char* szExtension) const
{
  return xiiStringUtils::IsEqual_NoCase(szExtension, "exr");
}

bool xiiExrFileFormat::CanWriteFileType(const char* szExtension) const
{
  return false;
}

#endif

XII_STATICLINK_FILE(Texture, Texture_Image_Formats_ExrFileFormat);
