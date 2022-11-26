#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiImageFileFormat);

xiiImageFileFormat* xiiImageFileFormat::GetReaderFormat(const char* extension)
{
  for (xiiImageFileFormat* pFormat = xiiImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(extension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

xiiImageFileFormat* xiiImageFileFormat::GetWriterFormat(const char* extension)
{
  for (xiiImageFileFormat* pFormat = xiiImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(extension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

xiiResult xiiImageFileFormat::ReadImageHeader(const char* szFileName, xiiImageHeader& header)
{
  XII_LOG_BLOCK("Read Image Header", szFileName);

  XII_PROFILE_SCOPE(xiiPathUtils::GetFileNameAndExtension(szFileName).GetStartPointer());

  xiiFileReader reader;
  if (reader.Open(szFileName) == XII_FAILURE)
  {
    xiiLog::Warning("Failed to open image file '{0}'", xiiArgSensitive(szFileName, "File"));
    return XII_FAILURE;
  }

  xiiStringView it = xiiPathUtils::GetFileExtension(szFileName);

  if (xiiImageFileFormat* pFormat = xiiImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImageHeader(reader, header, it.GetStartPointer()) != XII_SUCCESS)
    {
      xiiLog::Warning("Failed to read image file '{0}'", xiiArgSensitive(szFileName, "File"));
      return XII_FAILURE;
    }

    return XII_SUCCESS;
  }

  xiiLog::Warning("No known image file format for extension '{0}'", it);
  return XII_FAILURE;
}

XII_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFileFormat);
