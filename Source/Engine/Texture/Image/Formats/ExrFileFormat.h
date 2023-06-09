#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class XII_TEXTURE_DLL xiiExrFileFormat : public xiiImageFileFormat
{
public:
  xiiResult ReadImageHeader(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, const char* szFileExtension) const override;
  xiiResult ReadImage(xiiStreamReader& ref_stream, xiiImage& ref_image, const char* szFileExtension) const override;
  xiiResult WriteImage(xiiStreamWriter& ref_stream, const xiiImageView& image, const char* szFileExtension) const override;

  bool CanReadFileType(const char* szExtension) const override;
  bool CanWriteFileType(const char* szExtension) const override;
};

#endif
