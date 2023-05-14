#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/Stream.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Formats/WicFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <Texture/DirectXTex/DirectXTex.h>

using namespace DirectX;

XII_DEFINE_AS_POD_TYPE(DirectX::Image); // Allow for storing this struct in XII containers

xiiWicFileFormat g_wicFormat;

namespace
{
  /// \brief Try to init COM, return true if we are the first(!) to successfully do so
  bool InitializeCOM()
  {
    HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK)
    {
      // We were the first one - deinit on shutdown
      return true;
    }
    else if (SUCCEEDED(result))
    {
      // We were not the first one, but we still succeeded, so we deinit COM right away.
      // Otherwise we might be the last one to call CoUninitialize(), but that is supposed to be the one who called
      // CoInitialize[Ex]() first.
      CoUninitialize();
    }

    // We won't call CoUninitialize() on shutdown as either we were not the first one to init COM successfully (and uninitialized it right away),
    // or our call to CoInitializeEx() didn't succeed, because it was already called with another concurrency model specifier.
    return false;
  }
} // namespace


xiiWicFileFormat::xiiWicFileFormat() = default;

xiiWicFileFormat::~xiiWicFileFormat()
{
  if (m_bCoUninitOnShutdown)
  {
    // We were the first one to successfully initialize COM, so we are the one who needs to shut it down.
    CoUninitialize();
  }
}

xiiResult xiiWicFileFormat::ReadFileData(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& storage) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit          = false;
  }

  xiiStreamUtils::ReadAllAndAppend(stream, storage);

  if (storage.IsEmpty())
  {
    xiiLog::Error("Failure to retrieve image data.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

static void SetHeader(xiiImageHeader& header, xiiImageFormat::Enum imageFormat, const TexMetadata& metadata)
{
  header.SetImageFormat(imageFormat);

  header.SetWidth(xiiUInt32(metadata.width));
  header.SetHeight(xiiUInt32(metadata.height));
  header.SetDepth(xiiUInt32(metadata.depth));

  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(xiiUInt32(metadata.IsCubemap() ? (metadata.arraySize / 6) : metadata.arraySize));
  header.SetNumFaces(metadata.IsCubemap() ? 6 : 1);
}

xiiResult xiiWicFileFormat::ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiWicFileFormat::ReadImageHeader");

  xiiDynamicArray<xiiUInt8> storage;
  XII_SUCCEED_OR_RETURN(ReadFileData(stream, storage));

  TexMetadata  metadata;
  ScratchImage scratchImage;
  WIC_FLAGS    wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  HRESULT loadResult = GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
  if (FAILED(loadResult))
  {
    xiiLog::Error("Failure to load image metadata. HRESULT:{}", xiiArgErrorCode(loadResult));
    return XII_FAILURE;
  }

  xiiImageFormat::Enum imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Warning("Unable to use image format from '{}' file - trying conversion.", szFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
    imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Unable to use image format from '{}' file.", szFileExtension);
    return XII_FAILURE;
  }

  SetHeader(header, imageFormat, metadata);

  return XII_SUCCESS;
}

xiiResult xiiWicFileFormat::ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiWicFileFormat::ReadImage");

  xiiDynamicArray<xiiUInt8> storage;
  XII_SUCCEED_OR_RETURN(ReadFileData(stream, storage));

  TexMetadata  metadata;
  ScratchImage scratchImage;
  WIC_FLAGS    wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  // Read WIC data from local storage
  HRESULT loadResult = LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
  if (FAILED(loadResult))
  {
    xiiLog::Error("Failure to load image data. HRESULT:{}", xiiArgErrorCode(loadResult));
    return XII_FAILURE;
  }

  // Determine image format, re-reading image data if necessary
  metadata = scratchImage.GetMetadata();

  xiiImageFormat::Enum imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Warning("Unable to use image format from '{}' file - trying conversion.", szFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
    imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("Unable to use image format from '{}' file.", szFileExtension);
    return XII_FAILURE;
  }

  // Prepare destination image header and allocate storage
  xiiImageHeader imageHeader;
  SetHeader(imageHeader, imageFormat, metadata);

  image.ResetAndAlloc(imageHeader);

  // Read image data into destination image
  xiiUInt64 destRowPitch = imageHeader.GetRowPitch();
  xiiUInt32 itemIdx      = 0;
  for (xiiUInt32 arrayIdx = 0; arrayIdx < imageHeader.GetNumArrayIndices(); ++arrayIdx)
  {
    for (xiiUInt32 faceIdx = 0; faceIdx < imageHeader.GetNumFaces(); ++faceIdx, ++itemIdx)
    {
      for (xiiUInt32 sliceIdx = 0; sliceIdx < imageHeader.GetDepth(); ++sliceIdx)
      {
        const Image* sourceImage = scratchImage.GetImage(0, itemIdx, sliceIdx);
        xiiUInt8*    destPixels  = image.GetPixelPointer<xiiUInt8>(0, faceIdx, arrayIdx, 0, 0, sliceIdx);

        if (sourceImage && destPixels && sourceImage->pixels)
        {
          if (destRowPitch == sourceImage->rowPitch)
          {
            // Fast path: Just copy the entire thing
            xiiMemoryUtils::Copy(destPixels, sourceImage->pixels, static_cast<size_t>(imageHeader.GetHeight() * destRowPitch));
          }
          else
          {
            // Row pitches don't match - copy row by row
            xiiUInt64      bytesPerRow  = xiiMath::Min(destRowPitch, xiiUInt64(sourceImage->rowPitch));
            const uint8_t* sourcePixels = sourceImage->pixels;
            for (xiiUInt32 rowIdx = 0; rowIdx < imageHeader.GetHeight(); ++rowIdx)
            {
              xiiMemoryUtils::Copy(destPixels, sourcePixels, static_cast<size_t>(bytesPerRow));

              destPixels += destRowPitch;
              sourcePixels += sourceImage->rowPitch;
            }
          }
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiWicFileFormat::WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit          = false;
  }

  using namespace DirectX;

  // Convert into suitable output format
  xiiImageFormat::Enum compatibleFormats[] = {
    xiiImageFormat::R8G8B8A8_UNORM,
    xiiImageFormat::R8G8B8A8_UNORM_SRGB,
    xiiImageFormat::R8_UNORM,
    xiiImageFormat::R16G16B16A16_UNORM,
    xiiImageFormat::R16_UNORM,
    xiiImageFormat::R32G32B32A32_FLOAT,
    xiiImageFormat::R32G32B32_FLOAT,
  };

  // Find a compatible format closest to the one the image currently has
  xiiImageFormat::Enum format = xiiImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("No conversion from format '{0}' to a format suitable for '{}' files known.", xiiImageFormat::GetName(image.GetImageFormat()), szFileExtension);
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

  // Store xiiImage data in DirectXTex images
  xiiDynamicArray<Image> outputImages;
  DXGI_FORMAT            imageFormat = DXGI_FORMAT(xiiImageFormatMappings::ToDxgiFormat(image.GetImageFormat()));
  for (xiiUInt32 arrayIdx = 0; arrayIdx < image.GetNumArrayIndices(); ++arrayIdx)
  {
    for (xiiUInt32 faceIdx = 0; faceIdx < image.GetNumFaces(); ++faceIdx)
    {
      for (xiiUInt32 sliceIdx = 0; sliceIdx < image.GetDepth(); ++sliceIdx)
      {
        Image& currentImage     = outputImages.ExpandAndGetRef();
        currentImage.width      = image.GetWidth();
        currentImage.height     = image.GetHeight();
        currentImage.format     = imageFormat;
        currentImage.rowPitch   = static_cast<size_t>(image.GetRowPitch());
        currentImage.slicePitch = static_cast<size_t>(image.GetDepthPitch());
        currentImage.pixels     = const_cast<uint8_t*>(image.GetPixelPointer<uint8_t>(0, faceIdx, arrayIdx, 0, 0, sliceIdx));
      }
    }
  }

  if (!outputImages.IsEmpty())
  {
    // Store images in output blob
    Blob      targetBlob;
    WIC_FLAGS flags = WIC_FLAGS_NONE;
    HRESULT   res   = SaveToWICMemory(outputImages.GetData(), outputImages.GetCount(), flags, GetWICCodec(WIC_CODEC_TIFF), targetBlob);
    if (FAILED(res))
    {
      xiiLog::Error("Failed to save image data to local memory blob - result: {}!", xiiHRESULTtoString(res));
      return XII_FAILURE;
    }

    // Push blob into output stream
    if (stream.WriteBytes(targetBlob.GetBufferPointer(), targetBlob.GetBufferSize()) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write image data!");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

bool xiiWicFileFormat::CanReadFileType(const char* szExtension) const
{
  return xiiStringUtils::IsEqual_NoCase(szExtension, "png") || xiiStringUtils::IsEqual_NoCase(szExtension, "jpg") ||
    xiiStringUtils::IsEqual_NoCase(szExtension, "jpeg") ||
    // xiiStringUtils::IsEqual_NoCase(szExtension, "hdr") ||
    xiiStringUtils::IsEqual_NoCase(szExtension, "tif") || xiiStringUtils::IsEqual_NoCase(szExtension, "tiff");
}

bool xiiWicFileFormat::CanWriteFileType(const char* szExtension) const
{
  // png, jpg and jpeg are handled by STB (xiiStbImageFileFormats)
  return xiiStringUtils::IsEqual_NoCase(szExtension, "tif") || xiiStringUtils::IsEqual_NoCase(szExtension, "tiff");
}

#endif

XII_STATICLINK_FILE(Texture, Texture_Image_Formats_WicFileFormat);
