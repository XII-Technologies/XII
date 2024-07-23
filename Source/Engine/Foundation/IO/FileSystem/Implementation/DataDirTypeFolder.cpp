#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/Log.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FolderDataDirectory)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

namespace xiiDataDirectory
{
  xiiString FolderType::s_sRedirectionFile;
  xiiString FolderType::s_sRedirectionPrefix;

  xiiResult FolderReader::InternalOpen(xiiFileShareMode::Enum FileShareMode)
  {
    xiiStringBuilder sPath = ((xiiDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath());

    return m_File.Open(sPath.GetData(), xiiFileOpenMode::Read, FileShareMode);
  }

  void FolderReader::InternalClose()
  {
    m_File.Close();
  }

  xiiUInt64 FolderReader::Skip(xiiUInt64 uiBytes)
  {
    if (uiBytes == 0)
    {
      return 0;
    }

    const xiiUInt64 fileSize         = m_File.GetFileSize();
    const xiiUInt64 origFilePosition = m_File.GetFilePosition();
    XII_ASSERT_DEBUG(origFilePosition <= fileSize, "");

    const xiiUInt64 newFilePosition = xiiMath::Min(fileSize, origFilePosition + uiBytes);
    m_File.SetFilePosition(newFilePosition, xiiFileSeekMode::FromStart);
    XII_ASSERT_DEBUG(newFilePosition == m_File.GetFilePosition(), "");

    XII_ASSERT_DEBUG(newFilePosition >= origFilePosition, "");
    return newFilePosition - origFilePosition;
  }

  xiiUInt64 FolderReader::Read(void* pBuffer, xiiUInt64 uiBytes)
  {
    return m_File.Read(pBuffer, uiBytes);
  }

  xiiUInt64 FolderReader::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  xiiResult FolderWriter::InternalOpen(xiiFileShareMode::Enum FileShareMode)
  {
    xiiStringBuilder sPath = ((xiiDataDirectory::FolderType*)GetDataDirectory())->GetRedirectedDataDirectoryPath();
    sPath.AppendPath(GetFilePath());

    return m_File.Open(sPath.GetData(), xiiFileOpenMode::Write, FileShareMode);
  }

  void FolderWriter::InternalClose()
  {
    m_File.Close();
  }

  xiiResult FolderWriter::Write(const void* pBuffer, xiiUInt64 uiBytes)
  {
    return m_File.Write(pBuffer, uiBytes);
  }

  xiiUInt64 FolderWriter::GetFileSize() const
  {
    return m_File.GetFileSize();
  }

  xiiDataDirectoryType* FolderType::Factory(xiiStringView sDataDirectory, xiiStringView sGroup, xiiStringView sRootName, xiiDataDirUsage usage)
  {
    FolderType* pDataDir = XII_DEFAULT_NEW(FolderType);

    if (pDataDir->InitializeDataDirectory(sDataDirectory) == XII_SUCCESS)
      return pDataDir;

    XII_DEFAULT_DELETE(pDataDir);
    return nullptr;
  }

  void FolderType::RemoveDataDirectory()
  {
    {
      XII_LOCK(m_ReaderWriterMutex);
      for (xiiUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        XII_ASSERT_DEV(!m_Readers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }

      for (xiiUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        XII_ASSERT_DEV(!m_Writers[i]->m_bIsInUse, "Cannot remove a data directory while there are still files open in it.");
      }
    }
    FolderType* pThis = this;
    XII_DEFAULT_DELETE(pThis);
  }

  void FolderType::DeleteFile(xiiStringView sFile)
  {
    xiiStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sFile);

    xiiOSFile::DeleteFile(sPath.GetData()).IgnoreResult();
  }

  FolderType::~FolderType()
  {
    XII_LOCK(m_ReaderWriterMutex);
    for (xiiUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      XII_DEFAULT_DELETE(m_Readers[i]);

    for (xiiUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      XII_DEFAULT_DELETE(m_Writers[i]);
  }

  void FolderType::ReloadExternalConfigs()
  {
    LoadRedirectionFile();
  }

  void FolderType::LoadRedirectionFile()
  {
    XII_LOCK(m_RedirectionMutex);
    m_FileRedirection.Clear();

    if (!s_sRedirectionFile.IsEmpty())
    {
      xiiStringBuilder sRedirectionFile(GetRedirectedDataDirectoryPath(), "/", s_sRedirectionFile);
      sRedirectionFile.MakeCleanPath();

      XII_LOG_BLOCK("LoadRedirectionFile", sRedirectionFile.GetData());

      xiiOSFile file;
      if (file.Open(sRedirectionFile, xiiFileOpenMode::Read).Succeeded())
      {
        xiiHybridArray<char, 1024 * 10> content;
        char                            uiTemp[4096];

        xiiUInt64 uiRead = 0;

        do
        {
          uiRead = file.Read(uiTemp, XII_ARRAY_SIZE(uiTemp));
          content.PushBackRange(xiiArrayPtr<char>(uiTemp, (xiiUInt32)uiRead));
        } while (uiRead == XII_ARRAY_SIZE(uiTemp));

        content.PushBack(0); // Make sure the string is terminated

        const char* szLineStart = content.GetData();
        const char* szSeparator = nullptr;
        const char* szLineEnd   = nullptr;

        xiiStringBuilder sFileToRedirect, sRedirection;

        while (true)
        {
          szSeparator = xiiStringUtils::FindSubString(szLineStart, ";");
          szLineEnd   = xiiStringUtils::FindSubString(szSeparator, "\n");

          if (szLineStart == nullptr || szSeparator == nullptr || szLineEnd == nullptr)
            break;

          sFileToRedirect.SetSubString_FromTo(szLineStart, szSeparator);
          sRedirection.SetSubString_FromTo(szSeparator + 1, szLineEnd);

          m_FileRedirection[sFileToRedirect] = sRedirection;

          szLineStart = szLineEnd + 1;
        }

        // xiiLog::Debug("Redirection file contains {0} entries", m_FileRedirection.GetCount());
      }
      // else
      // xiiLog::Debug("No Redirection file found in: '{0}'", sRedirectionFile);
    }
  }


  bool FolderType::ExistsFile(xiiStringView sFile, bool bOneSpecificDataDir)
  {
    xiiStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(sFile, sRedirectedAsset);

    xiiStringBuilder sPath = GetRedirectedDataDirectoryPath();
    sPath.AppendPath(sRedirectedAsset);
    return xiiOSFile::ExistsFile(sPath);
  }

  xiiResult FolderType::GetFileStats(xiiStringView sFileOrFolder, bool bOneSpecificDataDir, xiiFileStats& out_Stats)
  {
    xiiStringBuilder sRedirectedAsset;
    ResolveAssetRedirection(sFileOrFolder, sRedirectedAsset);

    xiiStringBuilder sPath = GetRedirectedDataDirectoryPath();

    if (xiiPathUtils::IsAbsolutePath(sRedirectedAsset))
    {
      if (!sRedirectedAsset.StartsWith_NoCase(sPath))
        return XII_FAILURE;

      sPath.Clear();
    }

    sPath.AppendPath(sRedirectedAsset);

    if (!xiiPathUtils::IsAbsolutePath(sPath))
      return XII_FAILURE;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
    return xiiOSFile::GetFileStats(sPath, out_Stats);
#else
    return XII_FAILURE;
#endif
  }

  xiiResult FolderType::InternalInitializeDataDirectory(xiiStringView sDirectory)
  {
    // Allow to set the 'empty' directory to handle all absolute paths
    if (sDirectory.IsEmpty())
      return XII_SUCCESS;

    xiiStringBuilder sRedirected;
    if (xiiFileSystem::ResolveSpecialDirectory(sDirectory, sRedirected).Succeeded())
    {
      m_sRedirectedDataDirPath = sRedirected;
    }
    else
    {
      m_sRedirectedDataDirPath = sDirectory;
    }

    if (!xiiOSFile::ExistsDirectory(m_sRedirectedDataDirPath))
      return XII_FAILURE;

    ReloadExternalConfigs();

    return XII_SUCCESS;
  }

  void FolderType::OnReaderWriterClose(xiiDataDirectoryReaderWriterBase* pClosed)
  {
    XII_LOCK(m_ReaderWriterMutex);
    if (pClosed->IsReader())
    {
      FolderReader* pReader = (FolderReader*)pClosed;
      pReader->m_bIsInUse   = false;
    }
    else
    {
      FolderWriter* pWriter = (FolderWriter*)pClosed;
      pWriter->m_bIsInUse   = false;
    }
  }

  xiiDataDirectory::FolderReader* FolderType::CreateFolderReader() const
  {
    return XII_DEFAULT_NEW(FolderReader, 0);
  }

  xiiDataDirectory::FolderWriter* FolderType::CreateFolderWriter() const
  {
    return XII_DEFAULT_NEW(FolderWriter, 0);
  }

  xiiDataDirectoryReader* FolderType::OpenFileToRead(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir)
  {
    xiiStringBuilder sFileToOpen;
    ResolveAssetRedirection(sFile, sFileToOpen);

    // We know that these files cannot be opened, so don't even try
    if (xiiConversionUtils::IsStringUuid(sFileToOpen))
      return nullptr;

    FolderReader* pReader = nullptr;
    {
      XII_LOCK(m_ReaderWriterMutex);
      for (xiiUInt32 i = 0; i < m_Readers.GetCount(); ++i)
      {
        if (!m_Readers[i]->m_bIsInUse)
          pReader = m_Readers[i];
      }

      if (pReader == nullptr)
      {
        m_Readers.PushBack(CreateFolderReader());
        pReader = m_Readers.PeekBack();
      }
      pReader->m_bIsInUse = true;
    }

    // If opening the file fails, the reader's m_bIsInUse needs to be reset.
    if (pReader->Open(sFileToOpen, this, FileShareMode) == XII_FAILURE)
    {
      XII_LOCK(m_ReaderWriterMutex);
      pReader->m_bIsInUse = false;
      return nullptr;
    }

    // If it succeeds, we return the reader
    return pReader;
  }


  bool FolderType::ResolveAssetRedirection(xiiStringView sFile, xiiStringBuilder& out_sRedirection)
  {
    XII_LOCK(m_RedirectionMutex);
    // Check if we know about a file redirection for this
    auto it = m_FileRedirection.Find(sFile);

    // If available, open the file that is mentioned in the redirection file instead
    if (it.IsValid())
    {

      if (it.Value().StartsWith("?"))
      {
        // ? is an option to tell the system to skip the redirection prefix and use the path as is
        out_sRedirection = &it.Value().GetData()[1];
      }
      else
      {
        out_sRedirection.Set(s_sRedirectionPrefix, it.Value());
      }
      return true;
    }
    else
    {
      out_sRedirection = sFile;
      return false;
    }
  }

  xiiDataDirectoryWriter* FolderType::OpenFileToWrite(xiiStringView sFile, xiiFileShareMode::Enum FileShareMode)
  {
    FolderWriter* pWriter = nullptr;

    {
      XII_LOCK(m_ReaderWriterMutex);
      for (xiiUInt32 i = 0; i < m_Writers.GetCount(); ++i)
      {
        if (!m_Writers[i]->m_bIsInUse)
          pWriter = m_Writers[i];
      }

      if (pWriter == nullptr)
      {
        m_Writers.PushBack(CreateFolderWriter());
        pWriter = m_Writers.PeekBack();
      }
      pWriter->m_bIsInUse = true;
    }

    // If opening the file fails, the writer's m_bIsInUse needs to be reset.
    if (pWriter->Open(sFile, this, FileShareMode) == XII_FAILURE)
    {
      XII_LOCK(m_ReaderWriterMutex);
      pWriter->m_bIsInUse = false;
      return nullptr;
    }

    // If it succeeds, we return the reader
    return pWriter;
  }
} // namespace xiiDataDirectory


XII_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_DataDirTypeFolder);
