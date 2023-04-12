#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class XII_TEXTURE_DLL xiiExrFileFormat : public xiiImageFileFormat
{
public:
  xiiResult ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const override;
  xiiResult ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const override;
  xiiResult WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const override;

  bool CanReadFileType(const char* szExtension) const override;
  bool CanWriteFileType(const char* szExtension) const override;
};

#endif
