/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

class XII_TEXTURE_DLL xiiBmpFileFormat : public xiiImageFileFormat
{
public:
  virtual xiiResult ReadImageHeader(xiiStreamReader& inout_stream, xiiImageHeader& ref_header, xiiStringView sFileExtension) const override;

  virtual xiiResult ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const override;

  virtual bool CanReadFileType(xiiStringView sExtension) const override;
  virtual bool CanWriteFileType(xiiStringView sExtension) const override;
};
