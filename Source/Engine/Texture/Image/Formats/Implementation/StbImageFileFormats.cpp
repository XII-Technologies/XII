/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

XII_STATICLINK_FORCE static xiiImageFileFormatRegistrator<xiiStbImageFileFormats> g_StbImageFormats;

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
  void write_func(void* pContext, void* pData, int iSize)
  {
    xiiStreamWriter* writer = static_cast<xiiStreamWriter*>(pContext);
    writer->WriteBytes(pData, iSize).IgnoreResult();
  }

  void* ReadImageData(xiiStreamReader& inout_stream, xiiDynamicArray<xiiUInt8>& ref_fileBuffer, xiiGALTextureCreationDescription& ref_imageHeader, bool& ref_bIsHDR)
  {
    xiiStreamUtils::ReadAllAndAppend(inout_stream, ref_fileBuffer);

    int width, height, numComp;

    ref_bIsHDR = !!stbi_is_hdr_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount());

    void* pSourceImageData = nullptr;
    if (ref_bIsHDR)
    {
      pSourceImageData = stbi_loadf_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    else
    {
      pSourceImageData = stbi_load_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    if (!pSourceImageData)
    {
      xiiLog::Error("stb_image failed to load: {0}", stbi_failure_reason());
      return nullptr;
    }
    ref_fileBuffer.Clear();

    xiiEnum<xiiGALResourceFormat> format = xiiGALResourceFormat::Unknown;
    switch (numComp)
    {
      case 1:
        format = (ref_bIsHDR) ? xiiGALResourceFormat::R32Float : xiiGALResourceFormat::R8UNormalized;
        break;
      case 2:
        format = (ref_bIsHDR) ? xiiGALResourceFormat::RG32Float : xiiGALResourceFormat::RG8UNormalized;
        break;
      case 3:
        format = (ref_bIsHDR) ? xiiGALResourceFormat::RGB32Float : xiiGALResourceFormat::RGBA8UNormalized;
        break;
      case 4:
        format = (ref_bIsHDR) ? xiiGALResourceFormat::RGBA32Float : xiiGALResourceFormat::RGBA8UNormalized;
        break;
    }

    // Set properties and allocate.
    ref_imageHeader.m_Format             = format;
    ref_imageHeader.m_Size.width         = width;
    ref_imageHeader.m_Size.height        = height;
    ref_imageHeader.m_uiMipLevels        = 1;
    ref_imageHeader.m_uiArraySizeOrDepth = 1;

    return pSourceImageData;
  }

} // namespace

xiiResult xiiStbImageFileFormats::ReadImageDescription(xiiStreamReader& inout_stream, xiiGALTextureCreationDescription& ref_description, xiiStringView sFileExtension) const
{
  XII_IGNORE_UNUSED(sFileExtension);

  XII_PROFILE_SCOPE("xiiStbImageFileFormats::ReadImageDescription");

  bool                      bIsHDR = false;
  xiiDynamicArray<xiiUInt8> fileBuffer;
  void*                     pSourceImageData = ReadImageData(inout_stream, fileBuffer, ref_description, bIsHDR);

  if (pSourceImageData == nullptr)
    return XII_FAILURE;

  stbi_image_free(pSourceImageData);
  return XII_SUCCESS;
}

xiiResult xiiStbImageFileFormats::ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const
{
  XII_IGNORE_UNUSED(sFileExtension);

  XII_PROFILE_SCOPE("xiiStbImageFileFormats::ReadImage");

  bool                             bIsHDR = false;
  xiiDynamicArray<xiiUInt8>        fileBuffer;
  xiiGALTextureCreationDescription imageHeader;
  void*                            pSourceImageData = ReadImageData(inout_stream, fileBuffer, imageHeader, bIsHDR);

  if (pSourceImageData == nullptr)
    return XII_FAILURE;

  ref_image.ResetAndAlloc(imageHeader);

  const xiiGALResourceFormatDescription& formatDescription = xiiGALTextureUtilities::GetResourceFormatProperties(imageHeader.m_Format);
  const size_t                           uiElementsToCopy  = static_cast<size_t>(imageHeader.m_Size.width) * static_cast<size_t>(imageHeader.m_Size.height) * formatDescription.m_uiComponentCount;

  // Set pixels. Different strategies depending on component count.
  if (bIsHDR)
  {
    float* targetImageData = ref_image.GetBlobPtr<float>().GetPtr();
    xiiMemoryUtils::Copy(targetImageData, (const float*)pSourceImageData, uiElementsToCopy);
  }
  else
  {
    xiiUInt8* targetImageData = ref_image.GetBlobPtr<xiiUInt8>().GetPtr();
    xiiMemoryUtils::Copy(targetImageData, (const xiiUInt8*)pSourceImageData, uiElementsToCopy);
  }

  stbi_image_free((void*)pSourceImageData);
  return XII_SUCCESS;
}

xiiResult xiiStbImageFileFormats::WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const
{
  xiiEnum<xiiGALResourceFormat> compatibleFormats[] = {xiiGALResourceFormat::R8UNormalized, xiiGALResourceFormat::RGBA8UNormalized, xiiGALResourceFormat::RGBA8UNormalized};

  // Find a compatible format closest to the one the image currently has
  xiiEnum<xiiGALResourceFormat> format = xiiImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == xiiGALResourceFormat::Unknown)
  {
    xiiLog::Error("No conversion from format '{0}' to a format suitable for PNG files known.", xiiArgEnum(image.GetImageFormat()));
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

  if (sFileExtension.IsEqual_NoCase("png"))
  {
    if (stbi_write_png_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), xiiGALTextureUtilities::GetComponentCount(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 0))
    {
      return XII_SUCCESS;
    }
  }

  if (sFileExtension.IsEqual_NoCase("jpg") || sFileExtension.IsEqual_NoCase("jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), xiiGALTextureUtilities::GetComponentCount(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 95))
    {
      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}

bool xiiStbImageFileFormats::CanReadFileType(xiiStringView sExtension) const
{
  if (sExtension.IsEqual_NoCase("hdr"))
    return true;

#if XII_DISABLED(XII_PLATFORM_WINDOWS)

  // on Windows Desktop, we prefer to use WIC (xiiWicFileFormat)
  if (sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg"))
  {
    return true;
  }
#endif

  return false;
}

bool xiiStbImageFileFormats::CanWriteFileType(xiiStringView sExtension) const
{
  // even when WIC is available, prefer to write these files through STB, to get consistent output
  if (sExtension.IsEqual_NoCase("png") || sExtension.IsEqual_NoCase("jpg") || sExtension.IsEqual_NoCase("jpeg"))
  {
    return true;
  }

  return false;
}

XII_STATICLINK_FILE(Texture, Texture_Image_Formats_StbImageFileFormats);
