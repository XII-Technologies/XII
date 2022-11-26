#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

/// \brief File format implementation for loading TIFF files using WIC
class XII_TEXTURE_DLL xiiWicFileFormat : public xiiImageFileFormat
{
public:
  xiiWicFileFormat();
  virtual ~xiiWicFileFormat();

  virtual xiiResult ReadImageHeader(xiiStreamReader& stream, xiiImageHeader& header, const char* szFileExtension) const override;
  virtual xiiResult ReadImage(xiiStreamReader& stream, xiiImage& image, const char* szFileExtension) const override;
  virtual xiiResult WriteImage(xiiStreamWriter& stream, const xiiImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;

private:
  mutable bool m_bTryCoInit = true; // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown =
    false; // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)

  xiiResult ReadFileData(xiiStreamReader& stream, xiiDynamicArray<xiiUInt8>& storage) const;
};

#endif
