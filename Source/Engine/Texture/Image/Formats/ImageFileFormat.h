/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Utilities/EnumerableClass.h>

class xiiStreamReader;
class xiiStreamWriter;
class xiiImage;
class xiiImageView;

struct xiiGALTextureCreationDescription;

class XII_TEXTURE_DLL xiiImageFileFormat
{
public:
  /// Reads only the header information for an image and ignores the data. Much faster than reading the entire image, if the pixel data is not needed.
  virtual xiiResult ReadImageDescription(xiiStreamReader& inout_stream, xiiGALTextureCreationDescription& ref_description, xiiStringView sFileExtension) const = 0;

  /// Reads the data from the given stream and creates the image from it. Errors are written to the given xiiLogInterface.
  virtual xiiResult ReadImage(xiiStreamReader& inout_stream, xiiImage& ref_image, xiiStringView sFileExtension) const = 0;

  /// Writes the data to the given stream in this format. Errors are written to the given xiiLogInterface.
  virtual xiiResult WriteImage(xiiStreamWriter& inout_stream, const xiiImageView& image, xiiStringView sFileExtension) const = 0;

  /// Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(xiiStringView sExtension) const = 0;

  /// Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(xiiStringView sExtension) const = 0;

  /// Returns a xiiImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate xiiImageFileFormat.
  static const xiiImageFileFormat* GetReaderFormat(xiiStringView sExtension);

  /// Returns a xiiImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate xiiImageFileFormat.
  static const xiiImageFileFormat* GetWriterFormat(xiiStringView sExtension);

  static xiiResult ReadImageDescription(xiiStringView sFileName, xiiGALTextureCreationDescription& ref_description);
};

/// Base class for a registered (globally known) xiiImageFileFormat.
///
/// This is an enumerable class, so all known formats can be retrieved through the xiiEnumerable interface.
/// For example:
///
///   for (auto format = xiiRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
///   {
///     auto& type = format->GetFormatType();
///   }
class XII_TEXTURE_DLL xiiRegisteredImageFileFormat : public xiiEnumerable<xiiRegisteredImageFileFormat>
{
  XII_DECLARE_ENUMERABLE_CLASS(xiiRegisteredImageFileFormat);

public:
  xiiRegisteredImageFileFormat();
  ~xiiRegisteredImageFileFormat();

  virtual const xiiImageFileFormat& GetFormatType() const = 0;
};

/// Template used to automatically register a xiiImageFileFormat globally.
///
/// Place a global variable of the desired type in some CPP file to register the type:
///
///   xiiImageFileFormatRegistrator<xiiDdsFileFormat> g_ddsFormat;
///
/// For the format to be available on platforms that use static linking, you may also need to add
///   XII_STATICLINK_FORCE
template <class TYPE>
class xiiImageFileFormatRegistrator : public xiiRegisteredImageFileFormat
{
public:
  virtual const xiiImageFileFormat& GetFormatType() const override
  {
    return m_Format;
  }

private:
  TYPE m_Format;
};
