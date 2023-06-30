#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER) && XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <Foundation/Threading/ThreadUtils.h>
#  include <ToolsFoundation/FileSystem/FileSystemModel.h>


XII_CREATE_SIMPLE_TEST_GROUP(FileSystem);

namespace
{
  xiiResult xiitCreateFile(xiiStringView sPath)
  {
    xiiFileWriter FileOut;
    XII_SUCCEED_OR_RETURN(FileOut.Open(sPath));
    XII_SUCCEED_OR_RETURN(FileOut.WriteString("Test"));
    FileOut.Close();
    return XII_SUCCESS;
  }
} // namespace

XII_CREATE_SIMPLE_TEST(FileSystem, FileSystemModel)
{
  constexpr xiiUInt32 WAIT_LOOPS = 1000;

  xiiStringBuilder sOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("Model");
  sOutputFolder.MakeCleanPath();

  xiiStringBuilder sOutputFolderResolved;
  xiiFileSystem::ResolveSpecialDirectory(sOutputFolder, sOutputFolderResolved).IgnoreResult();

  xiiApplicationFileSystemConfig                 fsConfig;
  xiiApplicationFileSystemConfig::DataDirConfig& dataDir = fsConfig.m_DataDirs.ExpandAndGetRef();
  dataDir.m_bWritable                                    = true;
  dataDir.m_sDataDirSpecialPath                          = sOutputFolder;
  dataDir.m_sRootName                                    = "output";

  // Files
  xiiHybridArray<xiiFileChangedEvent, 2> fileEvents;
  xiiHybridArray<xiiTime, 2>             fileEventTimestamps;
  xiiMutex                               fileEventLock;
  auto                                   fileEvent = [&](const xiiFileChangedEvent& e) {
    XII_LOCK(fileEventLock);
    fileEvents.PushBack(e);
    fileEventTimestamps.PushBack(xiiTime::Now());

    xiiFileStatus stat;
    switch (e.m_Type)
    {
      case xiiFileChangedEvent::Type::FileRemoved:
        XII_TEST_BOOL(xiiFileSystemModel::GetSingleton()->FindFile(e.m_sPath, stat).Failed());
        break;
      case xiiFileChangedEvent::Type::FileAdded:
      case xiiFileChangedEvent::Type::FileChanged:
      case xiiFileChangedEvent::Type::DocumentLinked:
        XII_TEST_BOOL(xiiFileSystemModel::GetSingleton()->FindFile(e.m_sPath, stat).Succeeded());
        break;

      case xiiFileChangedEvent::Type::ModelReset:
      default:
        break;
    }
  };
  xiiEventSubscriptionID fileId = xiiFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(fileEvent);

  // Folders
  xiiHybridArray<xiiFolderChangedEvent, 2> folderEvents;
  xiiHybridArray<xiiTime, 2>               folderEventTimestamps;
  xiiMutex                                 folderEventLock;
  auto                                     folderEvent = [&](const xiiFolderChangedEvent& e) {
    XII_LOCK(folderEventLock);
    folderEvents.PushBack(e);
    folderEventTimestamps.PushBack(xiiTime::Now());

    switch (e.m_Type)
    {
      case xiiFolderChangedEvent::Type::FolderAdded:
        XII_TEST_BOOL(xiiFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_sPath));
        break;
      case xiiFolderChangedEvent::Type::FolderRemoved:
        XII_TEST_BOOL(!xiiFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_sPath));
        break;
      case xiiFolderChangedEvent::Type::ModelReset:
      default:
        break;
    }
  };
  xiiEventSubscriptionID folderId = xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(folderEvent);

  // Helper functions
  auto CompareFiles = [&](xiiArrayPtr<xiiFileChangedEvent> expected) {
    XII_LOCK(fileEventLock);
    if (XII_TEST_INT(expected.GetCount(), fileEvents.GetCount()))
    {
      for (size_t i = 0; i < expected.GetCount(); i++)
      {
        XII_TEST_INT((int)expected[i].m_Type, (int)fileEvents[i].m_Type);
        XII_TEST_STRING(expected[i].m_sPath, fileEvents[i].m_sPath);
        // Ignore stats
      }
    }
  };

  auto ClearFiles = [&]() {
    XII_LOCK(fileEventLock);
    fileEvents.Clear();
    fileEventTimestamps.Clear();
  };

  auto CompareFolders = [&](xiiArrayPtr<xiiFolderChangedEvent> expected) {
    XII_LOCK(folderEventLock);
    if (XII_TEST_INT(expected.GetCount(), folderEvents.GetCount()))
    {
      for (size_t i = 0; i < expected.GetCount(); i++)
      {
        XII_TEST_INT((int)expected[i].m_Type, (int)folderEvents[i].m_Type);
        XII_TEST_STRING(expected[i].m_sPath, folderEvents[i].m_sPath);
        // Ignore stats
      }
    }
  };

  auto ClearFolders = [&]() {
    XII_LOCK(folderEventLock);
    folderEvents.Clear();
    folderEventTimestamps.Clear();
  };


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Startup")
  {
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);

    XII_TEST_RESULT(xiiOSFile::DeleteFolder(sOutputFolderResolved));
    XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sOutputFolderResolved));

    // for absolute paths
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory("", "", ":", xiiFileSystem::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", xiiFileSystem::AllowWrites) == XII_SUCCESS);

    xiiFileSystemModel::GetSingleton()->Initialize(fsConfig, {}, {});

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    auto it = xiiFileSystemModel::GetSingleton()->GetFolders()->GetIterator();
    XII_TEST_STRING(it.Key(), sOutputFolder);
    XII_TEST_BOOL(it.Value() == xiiFileStatus::Status::Valid);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Add file")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    XII_TEST_RESULT(xiitCreateFile(sFilePath));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileAdded)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "modify file")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    {
#  if XII_ENABLED(XII_PLATFORM_LINUX)
      // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
      xiiThreadUtils::Sleep(xiiTime::Seconds(1.0));
#  endif
      xiiFileWriter FileOut;
      XII_TEST_RESULT(FileOut.Open(sFilePath));
      XII_TEST_RESULT(FileOut.WriteString("Test2"));
      XII_TEST_RESULT(FileOut.Flush());
      FileOut.Close();
    }

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileChanged)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "rename file")
  {
    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile.txt");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(sFilePathNew, {}, xiiFileChangedEvent::Type::FileAdded),
      xiiFileChangedEvent(sFilePathOld, {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Add folder")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder1");

    XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sFolderPath));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(folderEventLock);
      if (folderEvents.GetCount() > 0)
        break;
    }

    xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(sFolderPath, xiiFolderChangedEvent::Type::FolderAdded)};
    CompareFolders(xiiMakeArrayPtr(expected));
    ClearFolders();
    CompareFiles({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "move file")
  {
    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile2.txt");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder1", "rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(sFilePathNew, {}, xiiFileChangedEvent::Type::FileAdded),
      xiiFileChangedEvent(sFilePathOld, {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "move folder")
  {
    xiiStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder1");

    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder1", "rootFile2.txt");

    xiiStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("Folder2");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder2", "rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(fileEventLock);
      XII_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(sFolderPathNew, xiiFolderChangedEvent::Type::FolderAdded),
        xiiFolderChangedEvent(sFolderPathOld, xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
    }

    {
      xiiFileChangedEvent expected[] = {
        xiiFileChangedEvent(sFilePathNew, {}, xiiFileChangedEvent::Type::FileAdded),
        xiiFileChangedEvent(sFilePathOld, {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
    }
    {
      XII_LOCK(fileEventLock);
      XII_LOCK(folderEventLock);
      // Check folder added before file
      XII_TEST_BOOL(fileEventTimestamps[0] > folderEventTimestamps[0]);
      // Check file removed before folder
      XII_TEST_BOOL(fileEventTimestamps[1] < folderEventTimestamps[1]);
    }

    ClearFolders();
    ClearFiles();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "HashFile")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder2", "rootFile2.txt");

    xiiFileStatus status;
    XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->HashFile(sFilePathNew, status));
    XII_TEST_INT((xiiInt64)status.m_uiHash, (xiiInt64)10983861097202158394u);
  }

  xiiMap<xiiString, xiiFileStatus>         referencedFiles;
  xiiMap<xiiString, xiiFileStatus::Status> referencedFolders;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shutdown")
  {
    xiiFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Startup Restore Model")
  {
    xiiFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFiles")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder2", "rootFile2.txt");

    xiiFileSystemModel::LockedFiles files = xiiFileSystemModel::GetSingleton()->GetFiles();
    XII_TEST_INT(files->GetCount(), 1);
    auto it = files->GetIterator();
    XII_TEST_STRING(it.Key(), sFilePathNew);
    XII_TEST_BOOL(it.Value().m_LastModified.IsValid());
    XII_TEST_INT((xiiInt64)it.Value().m_uiHash, (xiiInt64)10983861097202158394u);
    XII_TEST_BOOL(it.Value().m_Status == xiiFileStatus::Status::Valid);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFolders")
  {
    xiiStringBuilder sFolder(sOutputFolder);
    sFolder.AppendPath("Folder2");

    xiiFileSystemModel::LockedFolders folders = xiiFileSystemModel::GetSingleton()->GetFolders();
    XII_TEST_INT(folders->GetCount(), 2);
    auto it = folders->GetIterator();

    // xiiMap is sorted so the order is fixed.
    XII_TEST_STRING(it.Key(), sOutputFolder);
    XII_TEST_BOOL(it.Value() == xiiFileStatus::Status::Valid);

    it.Next();
    XII_TEST_STRING(it.Key(), sFolder);
    XII_TEST_BOOL(it.Value() == xiiFileStatus::Status::Valid);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CheckFileSystem")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder2");

    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder2", "rootFile2.txt");

    xiiFileSystemModel::GetSingleton()->CheckFileSystem();

    // #TODO_ASSET This FileChanged should be removed once the model is fixed to no longer require firing this after restoring the model from cache. See comment in xiiFileSystemModel::HandleSingleFile.
    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileChanged),
      xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NotifyOfChange - File")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");
    {
      XII_TEST_RESULT(xiitCreateFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NotifyOfChange - Folder")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("AnotherFolder");
    {
      XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sFolderPath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(sFolderPath, xiiFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFolder(sFolderPath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(sFolderPath, xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CheckFolder - File")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder2", "subFile.txt");
    {
      XII_TEST_RESULT(xiitCreateFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CheckFolder - Folder")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("YetAnotherFolder");
    xiiStringBuilder sFolderSubPath(sOutputFolder);
    sFolderSubPath.AppendPath("YetAnotherFolder", "SubFolder");
    {
      XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sFolderSubPath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(sFolderPath, xiiFolderChangedEvent::Type::FolderAdded),
        xiiFolderChangedEvent(sFolderSubPath, xiiFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 4);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFolder(sFolderPath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(sFolderSubPath, xiiFolderChangedEvent::Type::FolderRemoved),
        xiiFolderChangedEvent(sFolderPath, xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadDocument")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder2", "rootFile2.txt");

    auto callback = [](const xiiFileStatus& status, xiiStreamReader& ref_reader) -> xiiUuid {
      XII_TEST_INT((xiiInt64)status.m_uiHash, (xiiInt64)10983861097202158394u);
      xiiUuid guid;
      guid.CreateNewUuid();
      return guid;
    };

    XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->ReadDocument(sFilePathNew, callback));

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePathNew, {}, xiiFileChangedEvent::Type::DocumentLinked)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LinkDocument")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder2", "rootFile2.txt");

    {
      xiiUuid guid;
      guid.CreateNewUuid();
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePathNew, {}, xiiFileChangedEvent::Type::DocumentLinked)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }
    {
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(sFilePathNew, {}, xiiFileChangedEvent::Type::DocumentUnlinked)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "delete folder")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder2");

    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder2", "rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::DeleteFolder(sFolderPath));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

      XII_LOCK(fileEventLock);
      XII_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 1 && folderEvents.GetCount() == 1)
        break;
    }

    {
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(sFolderPath, xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
    }

    {
      xiiFileChangedEvent expected[] = {
        xiiFileChangedEvent(sFilePath, {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
    }

    {
      XII_LOCK(fileEventLock);
      XII_LOCK(folderEventLock);
      // Check file removed before folder.
      XII_TEST_BOOL(fileEventTimestamps[0] < folderEventTimestamps[0]);
    }

    ClearFolders();
    ClearFiles();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shutdown2")
  {
    xiiFileSystemModel::GetSingleton()->Deinitialize();
    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();

    xiiFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(fileId);
    xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(folderId);
  }
}

#endif