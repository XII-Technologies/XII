#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/TextureDLL.h>

class xiiStreamReader;
class xiiStreamWriter;
class xiiImage;
class xiiImageView;
class xiiStringBuilder;
class xiiImageHeader;

class XII_TEXTURE_DLL xiiImageFileFormat : public xiiEnumerable<xiiImageFileFormat>
{
public:
  /// \brief Reads only the header information for an image and ignores the data. Much faster than reading the entire image, if the pixel data is not needed.
  virtual xiiResult ReadImageHeader(xiiStreamReader& ref_stream, xiiImageHeader& ref_header, xiiStringView sFileExtension) const = 0;

  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given xiiLogInterface.
  virtual xiiResult ReadImage(xiiStreamReader& ref_stream, xiiImage& ref_image, xiiStringView sFileExtension) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given xiiLogInterface.
  virtual xiiResult WriteImage(xiiStreamWriter& ref_stream, const xiiImageView& image, xiiStringView sFileExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(xiiStringView sExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(xiiStringView sExtension) const = 0;

  /// \brief Returns a xiiImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate xiiImageFileFormat.
  static xiiImageFileFormat* GetReaderFormat(xiiStringView sExtension);

  /// \brief Returns a xiiImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate xiiImageFileFormat.
  static xiiImageFileFormat* GetWriterFormat(xiiStringView sExtension);

  static xiiResult ReadImageHeader(xiiStringView sFileName, xiiImageHeader& ref_header);

  XII_DECLARE_ENUMERABLE_CLASS(xiiImageFileFormat);
};
