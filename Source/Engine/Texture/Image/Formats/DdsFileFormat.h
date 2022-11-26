#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class XII_TEXTURE_DLL xiiDdsFileFormat : public xiiImageFileFormat
{
public:
  virtual xiiResult ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const override;
  virtual xiiResult ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
