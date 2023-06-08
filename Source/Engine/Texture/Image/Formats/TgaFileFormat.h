#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class XII_TEXTURE_DLL xiiTgaFileFormat : public xiiImageFileFormat
{
public:
  virtual xiiResult ReadImageHeader(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, const char* szFileExtension) const override;
  virtual xiiResult ReadImage(xiiStreamReader& ref_stream, xiiImage& ref_image, const char* szFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& ref_stream, const xiiImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
