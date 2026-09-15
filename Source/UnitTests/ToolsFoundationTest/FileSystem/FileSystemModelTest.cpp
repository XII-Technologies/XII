/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER) && XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/Configuration/CVar.h>
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

XII_CREATE_SIMPLE_TEST(FileSystem, DataDirPath)
{
  const xiiStringView sFilePathView = "C:/Source/XII/Data/Samples/Testing Chambers/Objects/Barrel.xiiPrefab"_xiisv;
  const xiiStringView sDataDirView  = "C:/Source/XII/Data/Samples/Testing Chambers"_xiisv;

  auto CheckIsValid = [&](const xiiDataDirPath& path) {
    XII_TEST_BOOL(path.IsValid());
    xiiStringView sAbs = path.GetAbsolutePath();
    XII_TEST_STRING(sAbs, sFilePathView);
    xiiStringView sDD = path.GetDataDir();
    XII_TEST_STRING(sDD, sDataDirView);
    xiiStringView sPR = path.GetDataDirParentRelativePath();
    XII_TEST_STRING(sPR, "Testing Chambers/Objects/Barrel.xiiPrefab");
    xiiStringView sR = path.GetDataDirRelativePath();
    XII_TEST_STRING(sR, "Objects/Barrel.xiiPrefab");
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Windows Path copy ctor")
  {
    xiiHybridArray<xiiString, 2> rootFolders;
    rootFolders.PushBack("C:/SomeOtherFolder/Folder");
    rootFolders.PushBack(sDataDirView);

    xiiDataDirPath path(sFilePathView, rootFolders);
    CheckIsValid(path);
    xiiUInt32 uiIndex = path.GetDataDirIndex();
    XII_TEST_INT(uiIndex, 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Linux Path move ctor")
  {
    xiiString                    sFilePathView = "/Source/XII/Data/Samples/Testing Chambers/Objects/Barrel.xiiPrefab"_xiisv;
    xiiString                    sFilePath     = sFilePathView;
    auto                         sDataDir      = "/Source/XII/Data/Samples/Testing Chambers"_xiisv;
    xiiHybridArray<xiiString, 2> rootFolders;
    rootFolders.PushBack(sDataDir);
    rootFolders.PushBack("/SomeOtherFolder/Folder");

    const char*    szRawStringPtr = sFilePath.GetData();
    xiiDataDirPath path(std::move(sFilePath), rootFolders);
    XII_TEST_BOOL(path.IsValid());
    xiiStringView sAbs = path.GetAbsolutePath();
    XII_TEST_STRING(sAbs, sFilePathView);
    XII_TEST_BOOL(szRawStringPtr == sAbs.GetStartPointer());
    xiiStringView sDD = path.GetDataDir();
    XII_TEST_STRING(sDD, sDataDir);
    xiiStringView sPR = path.GetDataDirParentRelativePath();
    XII_TEST_STRING(sPR, "Testing Chambers/Objects/Barrel.xiiPrefab");
    xiiStringView sR = path.GetDataDirRelativePath();
    XII_TEST_STRING(sR, "Objects/Barrel.xiiPrefab");
    xiiUInt32 uiIndex = path.GetDataDirIndex();
    XII_TEST_INT(uiIndex, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Path to DataDir Itself")
  {
    xiiString                    sDataDirView = (const char*)u8"/Source/XII/Data/Sämples/Testing Chämbers";
    xiiHybridArray<xiiString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    xiiDataDirPath path(sDataDirView.GetView(), rootFolders);
    XII_TEST_BOOL(path.IsValid());
    xiiStringView sAbs = path.GetAbsolutePath();
    XII_TEST_STRING(sAbs, sDataDirView);
    xiiStringView sDD = path.GetDataDir();
    XII_TEST_STRING(sDD, sDataDirView);
    xiiStringView sPR = path.GetDataDirParentRelativePath();
    XII_TEST_STRING(sPR, (const char*)u8"Testing Chämbers");
    xiiStringView sR = path.GetDataDirRelativePath();
    XII_TEST_STRING(sR, "");
    xiiUInt32 uiIndex = path.GetDataDirIndex();
    XII_TEST_INT(uiIndex, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move")
  {
    xiiHybridArray<xiiString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    xiiString      sFilePath      = sFilePathView;
    const char*    szRawStringPtr = sFilePath.GetData();
    xiiDataDirPath path(std::move(sFilePath), rootFolders);
    CheckIsValid(path);

    xiiStringView sAbs = path.GetAbsolutePath();
    XII_TEST_BOOL(szRawStringPtr == sAbs.GetStartPointer());

    xiiDataDirPath path2 = std::move(path);
    xiiStringView  sAbs2 = path2.GetAbsolutePath();
    XII_TEST_BOOL(szRawStringPtr == sAbs2.GetStartPointer());

    xiiDataDirPath path3(std::move(path2));
    xiiStringView  sAbs3 = path3.GetAbsolutePath();
    XII_TEST_BOOL(szRawStringPtr == sAbs3.GetStartPointer());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rebuild")
  {
    xiiHybridArray<xiiString, 2> rootFolders;
    rootFolders.PushBack(sDataDirView);

    xiiDataDirPath path(sFilePathView, rootFolders);
    CheckIsValid(path);
    XII_TEST_INT(path.GetDataDirIndex(), 0);

    xiiHybridArray<xiiString, 2> newRootFolders;
    newRootFolders.PushBack(sDataDirView);
    newRootFolders.PushBack("C:/Some/Other/DataDir");

    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    XII_TEST_INT(path.GetDataDirIndex(), 0);

    newRootFolders.InsertAt(0, "C:/Some/Other/DataDir2");
    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    XII_TEST_INT(path.GetDataDirIndex(), 1);

    newRootFolders.RemoveAtAndCopy(0);
    path.UpdateDataDirInfos(newRootFolders);
    CheckIsValid(path);
    XII_TEST_INT(path.GetDataDirIndex(), 0);

    newRootFolders.RemoveAtAndCopy(0);
    path.UpdateDataDirInfos(newRootFolders);
    XII_TEST_BOOL(!path.IsValid());
    xiiStringView sAbs = path.GetAbsolutePath();
    XII_TEST_STRING(sAbs, sFilePathView);
  }
}

void FileSystemModelTest()
{
  constexpr xiiUInt32 WAIT_LOOPS = 1000;

  xiiStringBuilder sOutputFolder = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("Model");
  sOutputFolder.MakeCleanPath();

  xiiStringBuilder sOutputFolderResolved;
  xiiFileSystem::ResolveSpecialDirectory(sOutputFolder, sOutputFolderResolved).IgnoreResult();

  xiiHybridArray<xiiString, 1> rootFolders;

  xiiApplicationFileSystemConfig                 fsConfig;
  xiiApplicationFileSystemConfig::DataDirConfig& dataDir = fsConfig.m_DataDirs.ExpandAndGetRef();
  dataDir.m_bWritable                                    = true;
  dataDir.m_sDataDirSpecialPath                          = sOutputFolder;
  dataDir.m_sRootName                                    = "output";
  rootFolders.PushBack(sOutputFolder);

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
        XII_TEST_BOOL(xiiFileSystemModel::GetSingleton()->FindFile(e.m_Path, stat).Failed());
        break;
      case xiiFileChangedEvent::Type::FileAdded:
      case xiiFileChangedEvent::Type::FileChanged:
      case xiiFileChangedEvent::Type::DocumentLinked:
        XII_TEST_BOOL(xiiFileSystemModel::GetSingleton()->FindFile(e.m_Path, stat).Succeeded());
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
        XII_TEST_BOOL(xiiFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_Path));
        break;
      case xiiFolderChangedEvent::Type::FolderRemoved:
        XII_TEST_BOOL(!xiiFileSystemModel::GetSingleton()->GetFolders()->Contains(e.m_Path));
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
      for (xiiUInt32 i = 0; i < expected.GetCount(); i++)
      {
        XII_TEST_INT((int)expected[i].m_Type, (int)fileEvents[i].m_Type);
        XII_TEST_STRING(expected[i].m_Path, fileEvents[i].m_Path);
        XII_TEST_BOOL(expected[i].m_Status.m_DocumentID == fileEvents[i].m_Status.m_DocumentID);
        // Ignore stats besudes GUID.
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
      for (xiiUInt32 i = 0; i < expected.GetCount(); i++)
      {
        XII_TEST_INT((int)expected[i].m_Type, (int)folderEvents[i].m_Type);
        XII_TEST_STRING(expected[i].m_Path, folderEvents[i].m_Path);
        // Ignore stats
      }
    }
  };

  auto ClearFolders = [&]() {
    XII_LOCK(folderEventLock);
    folderEvents.Clear();
    folderEventTimestamps.Clear();
  };

  auto MakePath = [&](xiiStringView sPath) {
    return xiiDataDirPath(sPath, rootFolders);
  };


  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Startup")
  {
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);

    XII_TEST_RESULT(xiiOSFile::DeleteFolder(sOutputFolderResolved));
    XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sOutputFolderResolved));

    // for absolute paths
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory("", "", ":", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);
    XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", xiiDataDirUsage::AllowWrites) == XII_SUCCESS);

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

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "git")
  {
    xiiStringBuilder sIndex(sOutputFolder);
    sIndex.AppendPath("index");
    xiiStringBuilder sLock(sOutputFolder);
    sLock.AppendPath("index.lock");

    XII_TEST_RESULT(xiitCreateFile(sIndex));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sIndex), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }

#  if XII_ENABLED(XII_PLATFORM_LINUX)
    // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
    // As we intend to swap the index and index.lock files later, we need to make sure the two files have sufficiently different modification dates so that the swap of the files is detected as a change to the original file.
    xiiThreadUtils::Sleep(xiiTime::MakeFromSeconds(1.0));
#  endif

    XII_TEST_RESULT(xiitCreateFile(sLock));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sLock), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }

    XII_TEST_RESULT(xiiOSFile::DeleteFile(sIndex));
    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sLock, sIndex));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() >= 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(MakePath(sIndex), {}, xiiFileChangedEvent::Type::FileChanged),
      xiiFileChangedEvent(MakePath(sLock), {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    // Cleanup test
    XII_TEST_RESULT(xiiOSFile::DeleteFile(sIndex));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    ClearFiles();
    ClearFolders();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Git")
  {
    xiiStringBuilder sIndex(sOutputFolder);
    sIndex.AppendPath("index");
    xiiStringBuilder sLock(sOutputFolder);
    sLock.AppendPath("index.lock");

    XII_TEST_RESULT(xiitCreateFile(sIndex));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sIndex), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }

#  if XII_ENABLED(XII_PLATFORM_LINUX)
    // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
    // As we intend to swap the index and index.lock files later, we need to make sure the two files have sufficiently different modification dates so that the swap of the files is detected as a change to the original file.
    xiiThreadUtils::Sleep(xiiTime::MakeFromSeconds(1.0));
#  endif

    XII_TEST_RESULT(xiitCreateFile(sLock));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    {
      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sLock), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }

    XII_TEST_RESULT(xiiOSFile::DeleteFile(sIndex));
    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sLock, sIndex));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() >= 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(MakePath(sIndex), {}, xiiFileChangedEvent::Type::FileChanged),
      xiiFileChangedEvent(MakePath(sLock), {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);

    // Cleanup test
    XII_TEST_RESULT(xiiOSFile::DeleteFile(sIndex));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }
    ClearFiles();
    ClearFolders();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Add File")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    XII_TEST_RESULT(xiitCreateFile(sFilePath));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileAdded)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Modify File")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");

    {
#  if XII_ENABLED(XII_PLATFORM_LINUX)
      // EXT3 filesystem only support second resolution so we won't detect the modification if it is done within the same second.
      xiiThreadUtils::Sleep(xiiTime::MakeFromSeconds(1.0));
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
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() > 0)
        break;
    }

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileChanged)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Rename File")
  {
    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile.txt");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(MakePath(sFilePathNew), {}, xiiFileChangedEvent::Type::FileAdded),
      xiiFileChangedEvent(MakePath(sFilePathOld), {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Add Folder")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder1");

    XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sFolderPath));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(folderEventLock);
      if (folderEvents.GetCount() > 0)
        break;
    }

    xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(MakePath(sFolderPath), xiiFolderChangedEvent::Type::FolderAdded)};
    CompareFolders(xiiMakeArrayPtr(expected));
    ClearFolders();
    CompareFiles({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move File")
  {
    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("rootFile2.txt");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder1", "rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(MakePath(sFilePathNew), {}, xiiFileChangedEvent::Type::FileAdded),
      xiiFileChangedEvent(MakePath(sFilePathOld), {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Move Folder")
  {
    xiiStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder1");

    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder1", "rootFile2.txt");

    xiiStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("Folder12");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      XII_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(MakePath(sFolderPathNew), xiiFolderChangedEvent::Type::FolderAdded),
        xiiFolderChangedEvent(MakePath(sFolderPathOld), xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
    }

    {
      xiiFileChangedEvent expected[] = {
        xiiFileChangedEvent(MakePath(sFilePathNew), {}, xiiFileChangedEvent::Type::FileAdded),
        xiiFileChangedEvent(MakePath(sFilePathOld), {}, xiiFileChangedEvent::Type::FileRemoved)};
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
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    xiiFileStatus status;
    XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->HashFile(sFilePathNew, status));
    XII_TEST_INT((xiiInt64)status.m_uiHash, (xiiInt64)10983861097202158394u);
  }

  xiiFileSystemModel::FilesMap   referencedFiles;
  xiiFileSystemModel::FoldersMap referencedFolders;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shutdown")
  {
    xiiFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    XII_TEST_INT(referencedFiles.GetCount(), 1);
    XII_TEST_INT(referencedFolders.GetCount(), 2);

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();
  }

  xiiStringBuilder sOutputFolder2 = sOutputFolderResolved;
  sOutputFolder2.ChangeFileNameAndExtension("Model2");

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Startup Restore Model")
  {
    {
      // Add another data directory. This is now at the index of the old one, requiring the indices to be updated inside xiiFileSystemModel::Initialize.
      XII_TEST_RESULT(xiiOSFile::DeleteFolder(sOutputFolder2));
      XII_TEST_RESULT(xiiFileSystem::CreateDirectoryStructure(sOutputFolder2));
      xiiApplicationFileSystemConfig::DataDirConfig dataDir;
      dataDir.m_bWritable           = true;
      dataDir.m_sDataDirSpecialPath = sOutputFolder2;
      dataDir.m_sRootName           = "output2";

      rootFolders.InsertAt(0, sOutputFolder);
      fsConfig.m_DataDirs.InsertAt(0, dataDir);
    }

    xiiFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);

    // Check that files have be remapped.
    for (auto it : *xiiFileSystemModel::GetSingleton()->GetFiles())
    {
      XII_TEST_INT(it.Key().GetDataDirIndex(), 1);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFiles")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

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
    sFolder.AppendPath("Folder12");

    xiiFileSystemModel::LockedFolders folders = xiiFileSystemModel::GetSingleton()->GetFolders();
    XII_TEST_INT(folders->GetCount(), 3);
    auto it = folders->GetIterator();

    // xiiMap is sorted so the order is fixed.
    XII_TEST_STRING(it.Key(), sOutputFolder);
    XII_TEST_BOOL(it.Value() == xiiFileStatus::Status::Valid);

    it.Next();
    XII_TEST_STRING(it.Key(), sFolder);
    XII_TEST_BOOL(it.Value() == xiiFileStatus::Status::Valid);

    it.Next();
    XII_TEST_STRING(it.Key(), sOutputFolder2);
    XII_TEST_BOOL(it.Value() == xiiFileStatus::Status::Valid);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CheckFileSystem")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("Folder12");

    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder12", "rootFile2.txt");

    xiiFileSystemModel::GetSingleton()->CheckFileSystem();

    // #TODO_ASSET This FileChanged should be removed once the model is fixed to no longer require firing this after restoring the model from cache. See comment in xiiFileSystemModel::HandleSingleFile.
    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileChanged),
      xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "NotifyOfChange - File")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("rootFile.txt");
    {
      XII_TEST_RESULT(xiitCreateFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFilePath);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    for (size_t i = 0; i < 15; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
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
      xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(MakePath(sFolderPath), xiiFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 4);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFolder(sFolderPath));
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFolderPath);

      CompareFiles({});
      ClearFiles();
      xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(MakePath(sFolderPath), xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "CheckFolder - File")
  {
    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("Folder12", "subFile.txt");
    {
      XII_TEST_RESULT(xiitCreateFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 2);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFile(sFilePath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
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
        xiiFolderChangedEvent(MakePath(sFolderPath), xiiFolderChangedEvent::Type::FolderAdded),
        xiiFolderChangedEvent(MakePath(sFolderSubPath), xiiFolderChangedEvent::Type::FolderAdded)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 5);
    }

    {
      XII_TEST_RESULT(xiiOSFile::DeleteFolder(sFolderPath));
      xiiFileSystemModel::GetSingleton()->CheckFolder(sOutputFolder);

      CompareFiles({});
      ClearFiles();
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(MakePath(sFolderSubPath), xiiFolderChangedEvent::Type::FolderRemoved),
        xiiFolderChangedEvent(MakePath(sFolderPath), xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
      XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
    }
    for (size_t i = 0; i < 15; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ReadDocument")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    xiiUuid docGuid  = xiiUuid::MakeUuid();
    auto    callback = [&](const xiiFileStatus& status, xiiStreamReader& ref_reader) {
      XII_TEST_INT((xiiInt64)status.m_uiHash, (xiiInt64)10983861097202158394u);
      xiiFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, docGuid).IgnoreResult();
    };

    XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->ReadDocument(sFilePathNew, callback));

    xiiFileStatus stat;
    stat.m_DocumentID              = docGuid;
    xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePathNew), stat, xiiFileChangedEvent::Type::DocumentLinked)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LinkDocument")
  {
    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "rootFile2.txt");

    xiiUuid guid  = xiiUuid::MakeUuid();
    xiiUuid guid2 = xiiUuid::MakeUuid();
    {
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid));
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->LinkDocument(sFilePathNew, guid2));

      xiiFileStatus stat;
      stat.m_DocumentID = guid;
      xiiFileStatus stat2;
      stat2.m_DocumentID = guid2;

      xiiFileChangedEvent expected[] = {
        xiiFileChangedEvent(MakePath(sFilePathNew), stat, xiiFileChangedEvent::Type::DocumentLinked),
        xiiFileChangedEvent(MakePath(sFilePathNew), stat, xiiFileChangedEvent::Type::DocumentUnlinked),
        xiiFileChangedEvent(MakePath(sFilePathNew), stat2, xiiFileChangedEvent::Type::DocumentLinked)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }
    {
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));
      XII_TEST_RESULT(xiiFileSystemModel::GetSingleton()->UnlinkDocument(sFilePathNew));

      xiiFileStatus stat2;
      stat2.m_DocumentID = guid2;

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePathNew), stat2, xiiFileChangedEvent::Type::DocumentUnlinked)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }
    for (size_t i = 0; i < 15; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    CompareFiles({});
    CompareFolders({});
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Change file casing")
  {
    xiiStringBuilder sFilePathOld(sOutputFolder);
    sFilePathOld.AppendPath("Folder12", "rootFile2.txt");

    xiiStringBuilder sFilePathNew(sOutputFolder);
    sFilePathNew.AppendPath("Folder12", "RootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFilePathOld, sFilePathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2)
        break;
    }

    xiiFileChangedEvent expected[] = {
      xiiFileChangedEvent(MakePath(sFilePathNew), {}, xiiFileChangedEvent::Type::FileAdded),
      xiiFileChangedEvent(MakePath(sFilePathOld), {}, xiiFileChangedEvent::Type::FileRemoved)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();
    CompareFolders({});

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Change folder casing")
  {
    xiiStringBuilder sFolderPathOld(sOutputFolder);
    sFolderPathOld.AppendPath("Folder12");

    xiiStringBuilder sFolderPathNew(sOutputFolder);
    sFolderPathNew.AppendPath("FOLDER12");

    XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(sFolderPathOld, sFolderPathNew));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      if (fileEvents.GetCount() == 2 && folderEvents.GetCount() == 2)
        break;
    }

    {
      xiiFolderChangedEvent expected[] = {
        xiiFolderChangedEvent(MakePath(sFolderPathNew), xiiFolderChangedEvent::Type::FolderAdded),
        xiiFolderChangedEvent(MakePath(sFolderPathOld), xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
      ClearFolders();
    }

    {
      xiiStringBuilder sFilePathOld(sOutputFolder);
      sFilePathOld.AppendPath("Folder12", "RootFile2.txt");
      xiiStringBuilder sFilePathNew(sOutputFolder);
      sFilePathNew.AppendPath("FOLDER12", "RootFile2.txt");

      xiiFileChangedEvent expected[] = {
        xiiFileChangedEvent(MakePath(sFilePathNew), {}, xiiFileChangedEvent::Type::FileAdded),
        xiiFileChangedEvent(MakePath(sFilePathOld), {}, xiiFileChangedEvent::Type::FileRemoved)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
    }

    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 1);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Delete folder")
  {
    xiiStringBuilder sFolderPath(sOutputFolder);
    sFolderPath.AppendPath("FOLDER12");

    xiiStringBuilder sFilePath(sOutputFolder);
    sFilePath.AppendPath("FOLDER12", "RootFile2.txt");

    XII_TEST_RESULT(xiiOSFile::DeleteFolder(sFolderPath));

    for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
    {
      xiiFileSystemModel::GetSingleton()->MainThreadTick();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

      XII_LOCK(fileEventLock);
      XII_LOCK(folderEventLock);
      if (fileEvents.GetCount() == 1 && folderEvents.GetCount() == 1)
        break;
    }

    {
      xiiFolderChangedEvent expected[] = {xiiFolderChangedEvent(MakePath(sFolderPath), xiiFolderChangedEvent::Type::FolderRemoved)};
      CompareFolders(xiiMakeArrayPtr(expected));
    }

    {
      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileRemoved)};
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
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 2);
  }

  referencedFiles   = {};
  referencedFolders = {};

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Shutdown with cached files and folders")
  {
    {
      // Add a file to test data directories being removed.
      xiiStringBuilder sFilePath(sOutputFolder);
      sFilePath.AppendPath("rootFile.txt");

      XII_TEST_RESULT(xiitCreateFile(sFilePath));

      for (xiiUInt32 i = 0; i < WAIT_LOOPS; i++)
      {
        xiiFileSystemModel::GetSingleton()->MainThreadTick();
        xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

        XII_LOCK(fileEventLock);
        if (fileEvents.GetCount() > 0)
          break;
      }

      xiiFileChangedEvent expected[] = {xiiFileChangedEvent(MakePath(sFilePath), {}, xiiFileChangedEvent::Type::FileAdded)};
      CompareFiles(xiiMakeArrayPtr(expected));
      ClearFiles();
      CompareFolders({});
    }

    xiiFileSystemModel::GetSingleton()->Deinitialize(&referencedFiles, &referencedFolders);
    XII_TEST_INT(referencedFiles.GetCount(), 1);
    XII_TEST_INT(referencedFolders.GetCount(), 2);

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Startup without data dirs")
  {
    fsConfig.m_DataDirs.Clear();

    xiiFileSystemModel::GetSingleton()->Initialize(fsConfig, std::move(referencedFiles), std::move(referencedFolders));
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFiles()->GetCount(), 0);
    XII_TEST_INT(xiiFileSystemModel::GetSingleton()->GetFolders()->GetCount(), 0);

    xiiFileChangedEvent expected[] = {xiiFileChangedEvent({}, {}, xiiFileChangedEvent::Type::ModelReset)};
    CompareFiles(xiiMakeArrayPtr(expected));
    ClearFiles();

    xiiFolderChangedEvent expected2[] = {xiiFolderChangedEvent({}, xiiFolderChangedEvent::Type::ModelReset)};
    CompareFolders(xiiMakeArrayPtr(expected2));
    ClearFolders();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Final shutdown")
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

XII_CREATE_SIMPLE_TEST(FileSystem, FileSystemModel)
{
  FileSystemModelTest();
}

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
XII_CREATE_SIMPLE_TEST(FileSystem, FileSystemModelNonNTFS)
{
  auto* pForceNonNTFS = static_cast<xiiCVarBool*>(xiiCVar::FindCVarByName("DirectoryWatcher.ForceNonNTFS"));
  *pForceNonNTFS      = true;

  FileSystemModelTest();

  *pForceNonNTFS = false;
}
#  endif

#endif
