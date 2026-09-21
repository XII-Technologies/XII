/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

/// File format implementation for loading TIFF files using WIC.
class XII_TEXTURE_DLL xiiWicFileFormat : public xiiImageFileFormat
{
public:
  xiiWicFileFormat();
  virtual ~xiiWicFileFormat();

  virtual xiiResult ReadImageDescription(xiiStreamReader& inout_stream, xiiGALTextureCreationDescription& ref_description, xiiStringView sFileExtension) const override;

  virtual xiiResult ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const override;

  virtual bool CanReadFileType(xiiStringView sExtension) const override;
  virtual bool CanWriteFileType(xiiStringView sExtension) const override;

private:
  mutable bool m_bTryCoInit          = true;  // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown = false; // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)

  xiiResult ReadFileData(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& storage) const;
};

#endif
