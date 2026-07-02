/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

const xiiImageFileFormat* xiiImageFileFormat::GetReaderFormat(xiiStringView sExtension)
{
  for (auto format = xiiRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanReadFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

const xiiImageFileFormat* xiiImageFileFormat::GetWriterFormat(xiiStringView sExtension)
{
  for (auto format = xiiRegisteredImageFileFormat::GetFirstInstance(); format != nullptr; format = format->GetNextInstance())
  {
    if (format->GetFormatType().CanWriteFileType(sExtension))
    {
      return &format->GetFormatType();
    }
  }

  return nullptr;
}

xiiResult xiiImageFileFormat::ReadImageDescription(xiiStringView sFileName, xiiGALTextureCreationDescription& ref_header)
{
  XII_LOG_BLOCK("Read Image Description", sFileName);

  XII_PROFILE_SCOPE(xiiPathUtils::GetFileNameAndExtension(sFileName));

  xiiFileReader reader;
  if (reader.Open(sFileName) == XII_FAILURE)
  {
    xiiLog::Warning("Failed to open image file '{0}'", xiiArgSensitive(sFileName, "File"));
    return XII_FAILURE;
  }

  xiiStringView it = xiiPathUtils::GetFileExtension(sFileName);

  if (const xiiImageFileFormat* pFormat = xiiImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImageDescription(reader, ref_header, it) != XII_SUCCESS)
    {
      xiiLog::Warning("Failed to read image file '{0}'", xiiArgSensitive(sFileName, "File"));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiLog::Warning("No known image file format for extension '{0}'", it);
  return XII_FAILURE;
}

//////////////////////////////////////////////////////////////////////////

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiRegisteredImageFileFormat);

xiiRegisteredImageFileFormat::xiiRegisteredImageFileFormat()  = default;
xiiRegisteredImageFileFormat::~xiiRegisteredImageFileFormat() = default;
