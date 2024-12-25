#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

/// Png and jpeg file format support using stb_image.
///
/// stb_image also supports other formats, but we stick to our own loader code where we can.
/// Also, stb HDR image support is not handled here yet.
class XII_TEXTURE_DLL xiiStbImageFileFormats : public xiiImageFileFormat
{
public:
  virtual xiiResult ReadImageHeader(xiiStreamReader& inout_stream, xiiImageHeader& ref_header, xiiStringView sFileExtension) const override;
  virtual xiiResult ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const override;

  virtual bool CanReadFileType(xiiStringView sExtension) const override;
  virtual bool CanWriteFileType(xiiStringView sExtension) const override;
};
