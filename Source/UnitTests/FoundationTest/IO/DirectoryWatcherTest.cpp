/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Threading/ThreadUtils.h>

namespace DirectoryWatcherTestHelpers
{
  struct ExpectedEvent
  {
    ~ExpectedEvent() {}; // NOLINT: Ensure that the structure is non POD.

    const char*               path;
    xiiDirectoryWatcherAction action;
    xiiDirectoryWatcherType   type;

    bool operator==(const ExpectedEvent& other) const
    {
      return xiiStringView(path) == xiiStringView(other.path) && action == other.action && type == other.type;
    }
  };

  struct ExpectedEventStorage
  {
    xiiString                 path;
    xiiDirectoryWatcherAction action;
    xiiDirectoryWatcherType   type;
  };

  void TickWatcher(xiiDirectoryWatcher& ref_watcher)
  {
    ref_watcher.EnumerateChanges([&](xiiStringView sPath, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {}, xiiTime::MakeFromMilliseconds(100));
  }
} // namespace DirectoryWatcherTestHelpers


void DirectoryWatcherTest()
{
  using namespace DirectoryWatcherTestHelpers;

  xiiStringBuilder tmp, tmp2;
  xiiStringBuilder sTestRootPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();
  sTestRootPath.AppendPath("DirectoryWatcher/");

  auto CheckExpectedEvents = [&](xiiDirectoryWatcher& ref_watcher, xiiArrayPtr<ExpectedEvent> events) {
    xiiDynamicArray<ExpectedEventStorage> firedEvents;
    xiiUInt32                             i = 0;
    ref_watcher.EnumerateChanges([&](xiiStringView sPath, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
      tmp = sPath;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action, type});
      if (i < events.GetCount())
      {
        XII_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
        XII_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
        XII_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
      }
      i++;
    },
                                 xiiTime::MakeFromMilliseconds(100));
    XII_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsUnordered = [&](xiiDirectoryWatcher& ref_watcher, xiiArrayPtr<ExpectedEvent> events) {
    xiiDynamicArray<ExpectedEventStorage> firedEvents;
    xiiUInt32                             i = 0;
    xiiDynamicArray<bool>                 eventFired;
    eventFired.SetCount(events.GetCount());
    ref_watcher.EnumerateChanges([&](xiiStringView sPath, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
      tmp = sPath;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action, type});
      auto index = events.IndexOf({tmp, action, type});
      XII_TEST_BOOL_MSG(index != xiiInvalidIndex, "Event %d (%s, %d, %d) not found in expected events list", i, tmp.GetData(), (int)action, (int)type);
      if (index != xiiInvalidIndex)
      {
        eventFired[index] = true;
      }
      i++;
    },
                                 xiiTime::MakeFromMilliseconds(100));
    for (auto& fired : eventFired)
    {
      XII_TEST_BOOL(fired);
    }
    XII_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsMultiple = [&](xiiArrayPtr<xiiDirectoryWatcher*> watchers, xiiArrayPtr<ExpectedEvent> events) {
    xiiDynamicArray<ExpectedEventStorage> firedEvents;
    xiiUInt32                             i = 0;
    xiiDirectoryWatcher::EnumerateChanges(
      watchers, [&](xiiStringView sPath, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
        tmp = sPath;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        if (i < events.GetCount())
        {
          XII_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
          XII_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
          XII_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
        }
        i++;
      },
      xiiTime::MakeFromMilliseconds(100));
    XII_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CreateFile = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    xiiOSFile file;
    XII_TEST_BOOL(file.Open(tmp, xiiFileOpenMode::Write).Succeeded());
    XII_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto ModifyFile = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    xiiOSFile file;
    XII_TEST_BOOL(file.Open(tmp, xiiFileOpenMode::Append).Succeeded());
    XII_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto DeleteFile = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    XII_TEST_BOOL(xiiOSFile::DeleteFile(tmp).Succeeded());
  };

  auto CreateDirectory = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(tmp).Succeeded());
  };

  auto Rename = [&](const char* szFrom, const char* szTo) {
    tmp = sTestRootPath;
    tmp.AppendPath(szFrom);

    tmp2 = sTestRootPath;
    tmp2.AppendPath(szTo);

    XII_TEST_BOOL(xiiOSFile::MoveFileOrDirectory(tmp, tmp2).Succeeded());
  };

  auto DeleteDirectory = [&](const char* szRelPath, bool bTest = true) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    tmp.MakeCleanPath();

    if (bTest)
    {
      XII_TEST_BOOL(xiiOSFile::DeleteFolder(tmp).Succeeded());
    }
    else
    {
      xiiOSFile::DeleteFolder(tmp).IgnoreResult();
    }
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Git")
  {
    xiiOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    CreateFile("index");

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("index.lock");
    DeleteFile("index");
    Rename("index.lock", "index");

    ExpectedEvent expectedEvents[] = {
      {"index.lock", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"index", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"index.lock", xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::File},
      {"index", xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::File},

    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Create File")
  {
    xiiOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Delete File")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Modify File")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    TickWatcher(watcher);

    ModifyFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("test.file");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Rename File")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("test.file");
    Rename("test.file", "supertest.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"test.file", xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::File},
      {"supertest.file", xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("supertest.file");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Change File Casing")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    Rename("rename.file", "Rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"rename.file", xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::File},
      {"Rename.file", xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("Rename.file");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Windows Check For Correct Handling Of Pending File Remove Event #1")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    DeleteFile("rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"rename.file", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Windows Check For Correct Handling Of Pending File Remove Event #2")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("rename.file");
    DeleteFile("rename.file");
    CreateFile("Rename.file");
    DeleteFile("Rename.file");

    ExpectedEvent expectedEvents[] = {
      {"rename.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"rename.file", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"Rename.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"Rename.file", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Create Directory")
  {
    xiiOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates).Succeeded());

    CreateDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Delete Directory")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Rename Directory")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames).Succeeded());

    CreateDirectory("testDir");
    Rename("testDir", "supertestDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::Directory},
      {"supertestDir", xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("supertestDir");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Change Directory Casing")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    Rename("renameDir", "RenameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"renameDir", xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::Directory},
      {"RenameDir", xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("RenameDir");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Windows Check For Correct Handling Of Pending Directory Remove Event #1")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    DeleteDirectory("renameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"renameDir", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Windows Check For Correct Handling Of Pending Directory Remove Event #2")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("renameDir");
    DeleteDirectory("renameDir");
    CreateDirectory("RenameDir");
    DeleteDirectory("RenameDir");

    ExpectedEvent expectedEvents[] = {
      {"renameDir", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"renameDir", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
      {"RenameDir", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"RenameDir", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Subdirectory Create File")
  {
    tmp = sTestRootPath;
    tmp.AppendPath("subdir");
    XII_TEST_BOOL(xiiOSFile::CreateDirectoryStructure(tmp).Succeeded());

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Subdirectory Delete File")
  {

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    DeleteFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Subdirectory Modify File")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    TickWatcher(watcher);

    ModifyFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GUI Create Folder & File")
  {
    DeleteDirectory("sub", false);
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("New Folder");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("New Folder", "sub");

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
      {"sub/bla", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GUI Create Folder & file fast")
  {
    DeleteDirectory("sub", false);
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("New Folder");
    Rename("New Folder", "sub");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
      {"sub/bla", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GUI Create Folder & File Fast Subdirectory")
  {
    DeleteDirectory("sub", false);

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("New Folder/subsub");
    Rename("New Folder", "sub");

    TickWatcher(watcher);

    CreateFile("sub/subsub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/subsub/bla", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/subsub/bla");
    DeleteFile("sub/subsub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/subsub/bla", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
      {"sub/subsub/bla", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);

    DeleteDirectory("sub");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GUI Delete Folder")
  {
    DeleteDirectory("sub2", false);
    DeleteDirectory("../sub2", false);

    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/file1", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"sub2/subsub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    Rename("sub2", "../sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/subsub2/file2.txt", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"sub2/subsub2", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
      {"sub2/file1", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"sub2", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
    };
    // Issue here: After moving sub2 out of view, it remains in m_pathToWd
    CheckExpectedEvents(watcher, expectedEvents2);

    Rename("../sub2", "sub2");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/file1", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"sub2/subsub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Create, Delete, Create")
  {
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/file1", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"sub2/subsub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    DeleteDirectory("sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/file1", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"sub2/subsub2/file2.txt", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"sub2/subsub2", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
      {"sub2", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::Directory},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents2);

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/file1", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
      {"sub2/subsub2", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GUI Create file & delete")
  {
    DeleteDirectory("sub", false);
    xiiDirectoryWatcher watcher;
    XII_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Renames).Succeeded());

    CreateFile("file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("file2.txt", "datei2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"file2.txt", xiiDirectoryWatcherAction::RenamedOldName, xiiDirectoryWatcherType::File},
      {"datei2.txt", xiiDirectoryWatcherAction::RenamedNewName, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    DeleteFile("datei2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"datei2.txt", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enumerate multiple")
  {
    DeleteDirectory("watch1", false);
    DeleteDirectory("watch2", false);
    DeleteDirectory("watch3", false);
    xiiDirectoryWatcher watchers[3];

    xiiDirectoryWatcher* pWatchers[] = {watchers + 0, watchers + 1, watchers + 2};

    CreateDirectory("watch1");
    CreateDirectory("watch2");
    CreateDirectory("watch3");

    xiiStringBuilder watchPath;

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch1");
    XII_TEST_BOOL(watchers[0].OpenDirectory(watchPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Renames).Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch2");
    XII_TEST_BOOL(watchers[1].OpenDirectory(watchPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Renames).Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch3");
    XII_TEST_BOOL(watchers[2].OpenDirectory(watchPath, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Writes | xiiDirectoryWatcher::Watch::Renames).Succeeded());

    CreateFile("watch1/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"watch1/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents1);

    CreateFile("watch2/file2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"watch2/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents2);

    CreateFile("watch3/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"watch3/file2.txt", xiiDirectoryWatcherAction::Added, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents3);

    ModifyFile("watch1/file2.txt");
    ModifyFile("watch2/file2.txt");

    ExpectedEvent expectedEvents4[] = {
      {"watch1/file2.txt", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
      {"watch2/file2.txt", xiiDirectoryWatcherAction::Modified, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents4);

    DeleteFile("watch1/file2.txt");
    DeleteFile("watch2/file2.txt");
    DeleteFile("watch3/file2.txt");

    ExpectedEvent expectedEvents5[] = {
      {"watch1/file2.txt", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"watch2/file2.txt", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
      {"watch3/file2.txt", xiiDirectoryWatcherAction::Removed, xiiDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents5);
  }

  xiiOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
}

XII_CREATE_SIMPLE_TEST(IO, DirectoryWatcher)
{
  DirectoryWatcherTest();
}

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
XII_CREATE_SIMPLE_TEST(IO, DirectoryWatcherNonNTFS)
{
  auto* pForceNonNTFS = static_cast<xiiCVarBool*>(xiiCVar::FindCVarByName("DirectoryWatcher.ForceNonNTFS"));
  *pForceNonNTFS      = true;
  DirectoryWatcherTest();
  *pForceNonNTFS = false;
}
#  endif

#endif
