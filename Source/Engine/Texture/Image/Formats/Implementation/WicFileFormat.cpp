/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Windows/HResultUtils.h>
#  include <Foundation/Containers/StaticArray.h>
#  include <Foundation/IO/Stream.h>
#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <Texture/Image/Formats/ImageFormatMappings.h>
#  include <Texture/Image/Formats/WicFileFormat.h>
#  include <Texture/Image/Image.h>
#  include <Texture/Image/ImageConversion.h>

#  include <DirectXTex/DirectXTex.h>

XII_DEFINE_AS_POD_TYPE(DirectX::Image); // Allow for storing this struct in XII containers.

XII_STATICLINK_FORCE static xiiImageFileFormatRegistrator<xiiWicFileFormat> g_WicFormat;

namespace
{
  /// Try to init COM, return true if we are the first(!) to successfully do so
  bool InitializeCOM()
  {
    HRESULT hResult = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (hResult == S_OK)
    {
      // We were the first one - deinit on shutdown
      return true;
    }
    else if (SUCCEEDED(hResult))
    {
      // We were not the first one, but we still succeeded, so we deinit COM right away.
      // Otherwise we might be the last one to call CoUninitialize(), but that is supposed to be the one who called CoInitialize[Ex]() first.
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

static void SetHeader(xiiGALTextureCreationDescription& ref_header, xiiEnum<xiiGALResourceFormat> imageFormat, const DirectX::TexMetadata& metadata)
{
  ref_header.m_Format      = imageFormat;
  ref_header.m_Size.width  = xiiUInt32(metadata.width);
  ref_header.m_Size.height = xiiUInt32(metadata.height);
  ref_header.m_uiMipLevels = 1;

  if (metadata.depth > 1)
  {
    ref_header.m_uiArraySizeOrDepth = xiiUInt32(metadata.depth);
  }
  else
  {
    ref_header.m_uiArraySizeOrDepth = xiiUInt32(metadata.arraySize);
  }
}

xiiResult xiiWicFileFormat::ReadImageDescription(xiiStreamReader& inout_stream, xiiGALTextureCreationDescription& ref_description, xiiStringView sFileExtension) const
{
  XII_PROFILE_SCOPE("xiiWicFileFormat::ReadImageDescription");

  xiiTemporaryArray<xiiUInt8> storage;
  XII_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  DirectX::TexMetadata  metadata;
  DirectX::ScratchImage scratchImage;
  DirectX::WIC_FLAGS    wicFlags = DirectX::WIC_FLAGS_ALL_FRAMES | DirectX::WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  HRESULT hLoadResult = DirectX::GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
  if (FAILED(hLoadResult))
  {
    xiiLog::Error("Failure to load image metadata. HRESULT:{}", xiiArgErrorCode(hLoadResult));

    return XII_FAILURE;
  }

  xiiEnum<xiiGALResourceFormat> imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Warning("Unable to use image format from '{}' file - trying conversion.", sFileExtension);

    wicFlags |= DirectX::WIC_FLAGS_FORCE_RGB;

    DirectX::GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);

    imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Error("Unable to use image format from '{}' file.", sFileExtension);

    return XII_FAILURE;
  }

  SetHeader(ref_description, imageFormat, metadata);

  return XII_SUCCESS;
}

xiiResult xiiWicFileFormat::ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const
{
  XII_PROFILE_SCOPE("xiiWicFileFormat::ReadImage");

  xiiTemporaryArray<xiiUInt8> storage;
  XII_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  DirectX::TexMetadata  metadata;
  DirectX::ScratchImage scratchImage;
  DirectX::WIC_FLAGS    wicFlags = DirectX::WIC_FLAGS_ALL_FRAMES | DirectX::WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  // Read WIC data from local storage
  HRESULT hLoadResult = DirectX::LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
  if (FAILED(hLoadResult))
  {
    xiiLog::Error("Failure to load image data. HRESULT:{}", xiiArgErrorCode(hLoadResult));
    return XII_FAILURE;
  }

  // Determine image format, re-reading image data if necessary
  metadata = scratchImage.GetMetadata();

  xiiEnum<xiiGALResourceFormat> imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Warning("Unable to use image format from '{}' file - trying conversion.", sFileExtension);

    wicFlags |= DirectX::WIC_FLAGS_FORCE_RGB;
    DirectX::LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
    imageFormat = xiiImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Error("Unable to use image format from '{}' file.", sFileExtension);
    return XII_FAILURE;
  }

  // Prepare destination image header and allocate storage
  xiiGALTextureCreationDescription imageHeader;
  SetHeader(imageHeader, imageFormat, metadata);

  ref_image.ResetAndAlloc(imageHeader);

  const xiiGALResourceFormatDescription& formatProperties = xiiGALTextureUtilities::GetResourceFormatProperties(imageHeader.m_Format);

  // Read image data into destination image
  xiiUInt64 uiDestinationRowPitch = formatProperties.GetRowPitch(imageHeader.m_Size.width);
  xiiUInt32 uiItemIndex           = 0;
  for (xiiUInt32 arrayIdx = 0; arrayIdx < imageHeader.m_uiArraySizeOrDepth; ++arrayIdx)
  {
    for (xiiUInt32 faceIdx = 0; faceIdx < 1; ++faceIdx, ++uiItemIndex) // \todo: Support cubemaps and other multi-face formats.
    {
      for (xiiUInt32 sliceIdx = 0; sliceIdx < 1; ++sliceIdx) // \todo Support 3D textures and other multi-slice formats.
      {
        const DirectX::Image* pSourceImage = scratchImage.GetImage(0, uiItemIndex, sliceIdx);
        xiiUInt8*             destPixels   = ref_image.GetPixelPointer<xiiUInt8>(0, faceIdx, arrayIdx, 0, 0, sliceIdx);

        if (pSourceImage && destPixels && pSourceImage->pixels)
        {
          if (uiDestinationRowPitch == pSourceImage->rowPitch)
          {
            // Fast path: Just copy the entire thing
            xiiMemoryUtils::Copy(destPixels, pSourceImage->pixels, static_cast<size_t>(imageHeader.m_Size.height * uiDestinationRowPitch));
          }
          else
          {
            // Row pitches don't match - copy row by row
            xiiUInt64      bytesPerRow  = xiiMath::Min(uiDestinationRowPitch, xiiUInt64(pSourceImage->rowPitch));
            const uint8_t* sourcePixels = pSourceImage->pixels;
            for (xiiUInt32 rowIdx = 0; rowIdx < imageHeader.m_Size.height; ++rowIdx)
            {
              xiiMemoryUtils::Copy(destPixels, sourcePixels, static_cast<size_t>(bytesPerRow));

              destPixels += uiDestinationRowPitch;
              sourcePixels += pSourceImage->rowPitch;
            }
          }
        }
      }
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiWicFileFormat::WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit          = false;
  }

  using namespace DirectX;

  // Convert into suitable output format
  xiiEnum<xiiGALResourceFormat> compatibleFormats[] = {
    xiiGALResourceFormat::RGBA8UNormalized,
    xiiGALResourceFormat::RGBA8UNormalizedSRGB,
    xiiGALResourceFormat::R8UNormalized,
    xiiGALResourceFormat::RGBA16UNormalized,
    xiiGALResourceFormat::R16UNormalized,
    xiiGALResourceFormat::RGBA32Float,
    xiiGALResourceFormat::RGB32Float,
  };

  // Find a compatible format closest to the one the image currently has
  xiiEnum<xiiGALResourceFormat> format = xiiImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Error("No conversion from format '{0}' to a format suitable for '{}' files known.", xiiArgEnum(image.GetImageFormat()), sFileExtension);
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

    return WriteImage(inout_stream, convertedImage, sFileExtension);
  }

  // Store xiiImage data in DirectXTex images
  xiiTemporaryArray<Image> outputImages;
  DXGI_FORMAT              imageFormat = DXGI_FORMAT(xiiImageFormatMappings::ToDxgiFormat(image.GetImageFormat()));
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
    if (inout_stream.WriteBytes(targetBlob.GetBufferPointer(), targetBlob.GetBufferSize()) != XII_SUCCESS)
    {
      xiiLog::Error("Failed to write image data!");
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

bool xiiWicFileFormat::CanReadFileType(xiiStringView sExtension) const
{
  // BMP Support.
  if (sExtension.IsEqual_NoCase("bmp") || sExtension.IsEqual_NoCase("dib") || sExtension.IsEqual_NoCase("rle"))
    return true;

  return sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg") || sExtension.IsEqual_NoCase("tif") || sExtension.IsEqual_NoCase("tiff");
}

bool xiiWicFileFormat::CanWriteFileType(xiiStringView sExtension) const
{
  // png, jpg and jpeg are handled by STB (xiiStbImageFileFormats)
  return sExtension.IsEqual_NoCase("tif") || sExtension.IsEqual_NoCase("tiff");
}

#endif

XII_STATICLINK_FILE(Texture, Texture_Image_Formats_WicFileFormat);
