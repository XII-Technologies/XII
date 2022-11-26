#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>


xiiStbImageFileFormats g_StbImageFormats;

// stb_image callbacks would be better than loading the entire file into memory.
// However, it turned out that it does not map well to xiiStreamReader

// namespace
//{
//  // fill 'data' with 'size' bytes.  return number of bytes actually read
//  int read(void *user, char *data, int size)
//  {
//    xiiStreamReader* pStream = static_cast<xiiStreamReader*>(user);
//    return static_cast<int>(pStream->ReadBytes(data, size));
//  }
//  // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
//  void skip(void *user, int n)
//  {
//    xiiStreamReader* pStream = static_cast<xiiStreamReader*>(user);
//    if(n > 0)
//      pStream->SkipBytes(n);
//    else
//      // ?? We cannot reverse skip.
//
//  }
//  // returns nonzero if we are at end of file/data
//  int eof(void *user)
//  {
//    xiiStreamReader* pStream = static_cast<xiiStreamReader*>(user);
//    // ?
//  }
//}

namespace
{

  void write_func(void* context, void* data, int size)
  {
    xiiStreamWriter* writer = static_cast<xiiStreamWriter*>(context);
    writer->WriteBytes(data, size).IgnoreResult();
  }

  void* ReadImageData(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& fileBuffer, xiiImageHeader& imageHeader, bool& isHDR)
  {
    xiiStreamUtils::ReadAllAndAppend(stream, fileBuffer);

    int width, height, numComp;

    isHDR = !!stbi_is_hdr_from_memory(fileBuffer.GetData(), fileBuffer.GetCount());

    void* sourceImageData = nullptr;
    if (isHDR)
    {
      sourceImageData = stbi_loadf_from_memory(fileBuffer.GetData(), fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    else
    {
      sourceImageData = stbi_load_from_memory(fileBuffer.GetData(), fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    if (!sourceImageData)
    {
      xiiLog::Error("stb_image failed to load: {0}", stbi_failure_reason());
      return nullptr;
    }
    fileBuffer.Clear();

    xiiImageFormat::Enum format = xiiImageFormat::UNKNOWN;
    switch (numComp)
    {
      case 1:
        format = (isHDR) ? xiiImageFormat::R32_FLOAT : xiiImageFormat::R8_UNORM;
        break;
      case 2:
        format = (isHDR) ? xiiImageFormat::R32G32_FLOAT : xiiImageFormat::R8G8_UNORM;
        break;
      case 3:
        format = (isHDR) ? xiiImageFormat::R32G32B32_FLOAT : xiiImageFormat::R8G8B8_UNORM;
        break;
      case 4:
        format = (isHDR) ? xiiImageFormat::R32G32B32A32_FLOAT : xiiImageFormat::R8G8B8A8_UNORM;
        break;
    }

    // Set properties and allocate.
    imageHeader.SetImageFormat(format);
    imageHeader.SetNumMipLevels(1);
    imageHeader.SetNumArrayIndices(1);
    imageHeader.SetNumFaces(1);

    imageHeader.SetWidth(width);
    imageHeader.SetHeight(height);
    imageHeader.SetDepth(1);

    return sourceImageData;
  }

} // namespace

xiiResult xiiStbImageFileFormats::ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiStbImageFileFormats::ReadImageHeader");

  bool                      isHDR = false;
  xiiDynamicArray<xiiUInt8> fileBuffer;
  void*                     sourceImageData = ReadImageData(stream, fileBuffer, header, isHDR);

  if (sourceImageData == nullptr)
    return XII_FAILURE;

  stbi_image_free(sourceImageData);
  return XII_SUCCESS;
}

xiiResult xiiStbImageFileFormats::ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const
{
  XII_PROFILE_SCOPE("xiiStbImageFileFormats::ReadImage");

  bool                      isHDR = false;
  xiiDynamicArray<xiiUInt8> fileBuffer;
  xiiImageHeader            imageHeader;
  void*                     sourceImageData = ReadImageData(stream, fileBuffer, imageHeader, isHDR);

  if (sourceImageData == nullptr)
    return XII_FAILURE;

  image.ResetAndAlloc(imageHeader);

  const size_t numComp = xiiImageFormat::GetNumChannels(imageHeader.GetImageFormat());

  const size_t elementsToCopy = static_cast<size_t>(imageHeader.GetWidth()) * static_cast<size_t>(imageHeader.GetHeight()) * numComp;

  // Set pixels. Different strategies depending on component count.
  if (isHDR)
  {
    float* targetImageData = image.GetBlobPtr<float>().GetPtr();
    xiiMemoryUtils::Copy(targetImageData, (const float*)sourceImageData, elementsToCopy);
  }
  else
  {
    xiiUInt8* targetImageData = image.GetBlobPtr<xiiUInt8>().GetPtr();
    xiiMemoryUtils::Copy(targetImageData, (const xiiUInt8*)sourceImageData, elementsToCopy);
  }

  stbi_image_free((void*)sourceImageData);
  return XII_SUCCESS;
}

xiiResult xiiStbImageFileFormats::WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const
{
  xiiImageFormat::Enum compatibleFormats[] = {xiiImageFormat::R8_UNORM, xiiImageFormat::R8G8B8_UNORM, xiiImageFormat::R8G8B8A8_UNORM};

  // Find a compatible format closest to the one the image currently has
  xiiImageFormat::Enum format = xiiImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == xiiImageFormat::UNKNOWN)
  {
    xiiLog::Error("No conversion from format '{0}' to a format suitable for PNG files known.", xiiImageFormat::GetName(image.GetImageFormat()));
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

  if (xiiStringUtils::IsEqual_NoCase(szFileExtension, "png"))
  {
    if (stbi_write_png_to_func(write_func, &stream, image.GetWidth(), image.GetHeight(), xiiImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 0))
    {
      return XII_SUCCESS;
    }
  }

  if (xiiStringUtils::IsEqual_NoCase(szFileExtension, "jpg") || xiiStringUtils::IsEqual_NoCase(szFileExtension, "jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &stream, image.GetWidth(), image.GetHeight(), xiiImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 95))
    {
      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

bool xiiStbImageFileFormats::CanReadFileType(const char* szExtension) const
{
  if (xiiStringUtils::IsEqual_NoCase(szExtension, "hdr"))
    return true;

#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP)

  // on Windows Desktop, we prefer to use WIC (xiiWicFileFormat)
  if (xiiStringUtils::IsEqual_NoCase(szExtension, "png") || xiiStringUtils::IsEqual_NoCase(szExtension, "jpg") || xiiStringUtils::IsEqual_NoCase(szExtension, "jpeg"))
  {
    return true;
  }
#endif

  return false;
}

bool xiiStbImageFileFormats::CanWriteFileType(const char* szExtension) const
{
  // even when WIC is available, prefer to write these files through STB, to get consistent output
  if (xiiStringUtils::IsEqual_NoCase(szExtension, "png") || xiiStringUtils::IsEqual_NoCase(szExtension, "jpg") || xiiStringUtils::IsEqual_NoCase(szExtension, "jpeg"))
  {
    return true;
  }

  return false;
}



XII_STATICLINK_FILE(Texture, Texture_Image_Formats_StbImageFileFormats);
