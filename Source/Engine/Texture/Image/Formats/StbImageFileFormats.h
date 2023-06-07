#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

/// Png and jpeg file format support using stb_image.
///
/// stb_image also supports other formats, but we stick to our own loader code where we can.
/// Also, stb HDR image support is not handled here yet.
class XII_TEXTURE_DLL xiiStbImageFileFormats : public xiiImageFileFormat
{
public:
  virtual xiiResult ReadImageHeader(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, const char* szFileExtension) const override;
  virtual xiiResult ReadImage(xiiStreamReader& ref_stream, xiiImage& ref_image, const char* szFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& ref_stream, const xiiImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
