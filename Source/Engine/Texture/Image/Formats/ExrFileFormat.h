#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class XII_TEXTURE_DLL xiiExrFileFormat : public xiiImageFileFormat
{
public:
  xiiResult ReadImageHeader(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, xiiStringView sFileExtension) const override;
  xiiResult ReadImage(xiiStreamReader& ref_stream, xiiImage& ref_image, xiiStringView sFileExtension) const override;
  xiiResult WriteImage(xiiStreamWriter& ref_stream, const xiiImageView& image, xiiStringView sFileExtension) const override;

  bool CanReadFileType(xiiStringView sExtension) const override;
  bool CanWriteFileType(xiiStringView sExtension) const override;
};

#endif
