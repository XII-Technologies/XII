#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiImageFileFormat);

xiiImageFileFormat* xiiImageFileFormat::GetReaderFormat(xiiStringView sExtension)
{
  for (xiiImageFileFormat* pFormat = xiiImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(sExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

xiiImageFileFormat* xiiImageFileFormat::GetWriterFormat(xiiStringView sExtension)
{
  for (xiiImageFileFormat* pFormat = xiiImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(sExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

xiiResult xiiImageFileFormat::ReadImageHeader(xiiStringView sFileName, xiiImageHeader& ref_header)
{
  XII_LOG_BLOCK("Read Image Header", sFileName);

  XII_PROFILE_SCOPE(xiiPathUtils::GetFileNameAndExtension(sFileName));

  xiiFileReader reader;
  if (reader.Open(sFileName) == XII_FAILURE)
  {
    xiiLog::Warning("Failed to open image file '{0}'", xiiArgSensitive(sFileName, "File"));
    return XII_FAILURE;
  }

  xiiStringView it = xiiPathUtils::GetFileExtension(sFileName);

  if (xiiImageFileFormat* pFormat = xiiImageFileFormat::GetReaderFormat(it))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it) != XII_SUCCESS)
    {
      xiiLog::Warning("Failed to read image file '{0}'", xiiArgSensitive(sFileName, "File"));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiLog::Warning("No known image file format for extension '{0}'", it);
  return XII_FAILURE;
}

XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFileFormat);
