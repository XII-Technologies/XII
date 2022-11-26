#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

/// Png and jpeg file format support using stb_image.
///
/// stb_image also supports other formats, but we stick to our own loader code where we can.
/// Also, stb HDR image support is not handled here yet.
class XII_TEXTURE_DLL xiiStbImageFileFormats : public xiiImageFileFormat
{
public:
  virtual xiiResult ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const override;
  virtual xiiResult ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
