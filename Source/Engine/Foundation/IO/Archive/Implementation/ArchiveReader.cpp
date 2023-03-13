#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>

#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Logging/Log.h>

xiiResult xiiArchiveReader::OpenArchive(xiiStringView sPath)
{
#if XII_ENABLED(XII_SUPPORTS_MEMORY_MAPPED_FILE)
  XII_LOG_BLOCK("OpenArchive", sPath);

  XII_SUCCEED_OR_RETURN(m_MemFile.Open(sPath, xiiMemoryMappedFile::Mode::ReadOnly));
  m_uiMemFileSize = m_MemFile.GetFileSize();

  // validate the archive
  {
    xiiRawMemoryStreamReader reader(m_MemFile.GetReadPointer(), m_MemFile.GetFileSize());

    xiiStringView extension = xiiPathUtils::GetFileExtension(sPath);

    if (xiiArchiveUtils::IsAcceptedArchiveFileExtensions(extension))
    {
      XII_SUCCEED_OR_RETURN(xiiArchiveUtils::ReadHeader(reader, m_uiArchiveVersion));

      m_pDataStart = m_MemFile.GetReadPointer(17, xiiMemoryMappedFile::OffsetBase::Start);

      XII_SUCCEED_OR_RETURN(xiiArchiveUtils::ExtractTOC(m_MemFile, m_ArchiveTOC, m_uiArchiveVersion));
    }

    else
    {
      xiiLog::Error("Unknown archive file extension '{}'", extension);
      return XII_FAILURE;
    }
  }

  // validate the entries
  {
    const xiiUInt32 uiMaxPathString = m_ArchiveTOC.m_AllPathStrings.GetCount();
    const xiiUInt64 uiValidSize     = m_uiMemFileSize - uiMaxPathString;

    for (const auto& e : m_ArchiveTOC.m_Entries)
    {
      if (e.m_uiDataStartOffset + e.m_uiStoredDataSize > uiValidSize)
      {
        xiiLog::Error("Archive is corrupt. Invalid entry data range.");
        return XII_FAILURE;
      }

      if (e.m_uiUncompressedDataSize < e.m_uiStoredDataSize)
      {
        xiiLog::Error("Archive is corrupt. Invalid compression info.");
        return XII_FAILURE;
      }

      if (e.m_uiPathStringOffset >= uiMaxPathString)
      {
        xiiLog::Error("Archive is corrupt. Invalid entry path-string offset.");
        return XII_FAILURE;
      }
    }
  }

  return XII_SUCCESS;
#else
  XII_REPORT_FAILURE("Memory mapped files are unsupported on this platform.");
  return XII_FAILURE;
#endif
}

const xiiArchiveTOC& xiiArchiveReader::GetArchiveTOC()
{
  return m_ArchiveTOC;
}

xiiResult xiiArchiveReader::ExtractAllFiles(xiiStringView sTargetFolder) const
{
  XII_LOG_BLOCK("ExtractAllFiles", sTargetFolder);

  const xiiUInt32 numEntries = m_ArchiveTOC.m_Entries.GetCount();

  for (xiiUInt32 e = 0; e < numEntries; ++e)
  {
    xiiStringView sPath = reinterpret_cast<const char*>(&m_ArchiveTOC.m_AllPathStrings[m_ArchiveTOC.m_Entries[e].m_uiPathStringOffset]);

    if (!ExtractNextFileCallback(e + 1, numEntries, sPath))
      return XII_FAILURE;

    XII_SUCCEED_OR_RETURN(ExtractFile(e, sTargetFolder));
  }

  return XII_SUCCESS;
}

void xiiArchiveReader::ConfigureRawMemoryStreamReader(xiiUInt32 uiEntryIdx, xiiRawMemoryStreamReader& memReader) const
{
  xiiArchiveUtils::ConfigureRawMemoryStreamReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart, memReader);
}

xiiUniquePtr<xiiStreamReader> xiiArchiveReader::CreateEntryReader(xiiUInt32 uiEntryIdx) const
{
  return xiiArchiveUtils::CreateEntryReader(m_ArchiveTOC.m_Entries[uiEntryIdx], m_pDataStart);
}

xiiResult xiiArchiveReader::ExtractFile(xiiUInt32 uiEntryIdx, xiiStringView sTargetFolder) const
{
  const char*     szFilePath = m_ArchiveTOC.GetEntryPathString(uiEntryIdx);
  const xiiUInt64 uiMaxSize  = m_ArchiveTOC.m_Entries[uiEntryIdx].m_uiUncompressedDataSize;

  xiiUniquePtr<xiiStreamReader> pReader = CreateEntryReader(uiEntryIdx);

  xiiStringBuilder sOutputFile = sTargetFolder;
  sOutputFile.AppendPath(szFilePath);

  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sOutputFile));

  xiiUInt8 uiTemp[1024 * 8];

  xiiUInt64 uiRead      = 0;
  xiiUInt64 uiReadTotal = 0;
  while (true)
  {
    uiRead = pReader->ReadBytes(uiTemp, XII_ARRAY_SIZE(uiTemp));

    if (uiRead == 0)
      break;

    XII_SUCCEED_OR_RETURN(file.WriteBytes(uiTemp, uiRead));

    uiReadTotal += uiRead;

    if (!ExtractFileProgressCallback(uiReadTotal, uiMaxSize))
      return XII_FAILURE;
  }

  XII_ASSERT_DEV(uiReadTotal == uiMaxSize, "Failed to read entire file");

  return XII_SUCCESS;
}

bool xiiArchiveReader::ExtractNextFileCallback(xiiUInt32 uiCurEntry, xiiUInt32 uiMaxEntries, xiiStringView sSourceFile) const
{
  return true;
}

bool xiiArchiveReader::ExtractFileProgressCallback(xiiUInt64 bytesWritten, xiiUInt64 bytesTotal) const
{
  return true;
}


XII_STATICLINK_FILE(Foundation, Foundation_IO_Archive_Implementation_ArchiveReader);
