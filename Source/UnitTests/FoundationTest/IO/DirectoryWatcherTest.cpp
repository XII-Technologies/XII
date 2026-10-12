/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Configuration/CVar.h>
#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Threading/Thread.h>
#  include <Foundation/Threading/ThreadUtils.h>
#  include <Foundation/Types/ScopeExit.h>

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
    const xiiTime                         deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
    do
    {
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
    } while (firedEvents.GetCount() < events.GetCount() && xiiTime::Now() < deadline);
    if (firedEvents.GetCount() != events.GetCount())
    {
      xiiStringBuilder observed;
      for (const auto& event : firedEvents)
        observed.AppendFormat(" [{0}:{1}]", event.path, static_cast<xiiUInt32>(event.action));
      XII_TEST_BOOL_MSG(false, "Expected %u events, observed %u:%s", events.GetCount(), firedEvents.GetCount(), observed.GetData());
    }
  };

  auto CheckExpectedEventsUnordered = [&](xiiDirectoryWatcher& ref_watcher, xiiArrayPtr<ExpectedEvent> events) {
    xiiDynamicArray<ExpectedEventStorage> firedEvents;
    xiiUInt32                             i = 0;
    xiiDynamicArray<bool>                 eventFired;
    eventFired.SetCount(events.GetCount());
    const xiiTime deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
    do
    {
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
    } while (firedEvents.GetCount() < events.GetCount() && xiiTime::Now() < deadline);
    for (auto& fired : eventFired)
    {
      XII_TEST_BOOL(fired);
    }
    XII_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsMultiple = [&](xiiArrayPtr<xiiDirectoryWatcher*> watchers, xiiArrayPtr<ExpectedEvent> events) {
    xiiDynamicArray<ExpectedEventStorage> firedEvents;
    xiiDynamicArray<bool>                 eventFired;
    eventFired.SetCount(events.GetCount());
    const xiiTime deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
    do
    {
      xiiDirectoryWatcher::EnumerateChanges(watchers, [&](xiiStringView path, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
        tmp = path;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        const auto index = events.IndexOf({tmp, action, type});
        XII_TEST_BOOL(index != xiiInvalidIndex);
        if (index != xiiInvalidIndex)
        {
          XII_TEST_BOOL(!eventFired[index]);
          eventFired[index] = true;
        } }, xiiTime::MakeFromMilliseconds(100));
    } while (firedEvents.GetCount() < events.GetCount() && xiiTime::Now() < deadline);
    for (bool fired : eventFired)
      XII_TEST_BOOL(fired);
    XII_TEST_INT(firedEvents.GetCount(), events.GetCount());
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

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
XII_CREATE_SIMPLE_TEST_GROUP(DirectoryRecovery);

XII_CREATE_SIMPLE_TEST(DirectoryRecovery, WorkerReadAndReopen)
{
  class PollThread : public xiiThread
  {
  public:
    PollThread(xiiDirectoryWatcher& watcher) : xiiThread("Directory watcher poll"), m_Watcher(watcher) {}
    xiiDirectoryWatcher& m_Watcher;
    xiiUInt32            Run() override
    {
      m_Watcher.EnumerateChanges([](xiiStringView, xiiDirectoryWatcherAction, xiiDirectoryWatcherType) {}, xiiTime::MakeFromMilliseconds(100));
      return 0;
    }
  };
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/DirectoryWorkerRead");
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(root));
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(root));
  xiiDirectoryWatcher watcher;
  XII_SCOPE_EXIT(watcher.CloseDirectory(); xiiOSFile::DeleteFolder(root).IgnoreResult());
  for (xiiUInt32 i = 0; i < 16; ++i)
  {
    if (!XII_TEST_RESULT(watcher.OpenDirectory(root, xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Deletes)))
      return;
    xiiStringBuilder path;
    path.SetFormat("{0}/File{1}", root, i);
    {
      xiiOSFile file;
      XII_TEST_RESULT(file.Open(path, xiiFileOpenMode::Write));
    }
    PollThread thread(watcher);
    thread.Start();
    thread.Join();
    // The pending read was reissued by the worker. Closing and reopening on the
    // main thread must finish that read before reusing its OVERLAPPED and buffer.
    watcher.CloseDirectory();
    XII_TEST_BOOL(watcher.GetDirectory().IsEmpty());
    XII_TEST_RESULT(xiiOSFile::DeleteFile(path));
  }
}

XII_CREATE_SIMPLE_TEST(DirectoryRecovery, NestedCaseChanges)
{
  using namespace DirectoryWatcherTestHelpers;
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/DirectoryCasingRecovery");
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(root));
  xiiStringBuilder parent(root, "/Mixed"), child(parent, "/Nested"), quietRoot(root, "/Quiet");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(child));
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(quietRoot));
  {
    xiiOSFile file;
    XII_TEST_RESULT(file.Open(xiiStringBuilder(child, "/child.txt"), xiiFileOpenMode::Write));
    XII_TEST_RESULT(file.Write("data", 4));
  }
  xiiDirectoryWatcher watcher, quiet;
  XII_SCOPE_EXIT(watcher.CloseDirectory(); quiet.CloseDirectory(); xiiOSFile::DeleteFolder(root).IgnoreResult());
  const auto flags = xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Creates | xiiDirectoryWatcher::Watch::Subdirectories;
  if (!XII_TEST_RESULT(watcher.OpenDirectory(root, flags)) || !XII_TEST_RESULT(quiet.OpenDirectory(quietRoot, flags)))
    return;
  xiiDirectoryWatcher*                  watchers[] = {&watcher, &quiet};
  xiiDynamicArray<ExpectedEventStorage> events;
  auto                                  count = [&](xiiStringView path, xiiDirectoryWatcherAction action) {
    xiiUInt32 result = 0;
    for (const auto& event : events)
      result += event.path == path && event.action == action;
    return result;
  };
  auto pump = [&] {
    xiiDirectoryWatcher::EnumerateChanges(xiiMakeArrayPtr(watchers), [&](xiiStringView path, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) { events.PushBack({path, action, type}); }, xiiTime::MakeFromMilliseconds(20));
  };
  auto wait = [&](auto predicate) {
    const xiiTime deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
    while (!predicate() && xiiTime::Now() < deadline)
      pump();
    XII_TEST_BOOL(predicate());
  };

  xiiStringBuilder renamedChild(parent, "/NESTED");
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(child, renamedChild));
  // Do no other filesystem writes until this silent rename has been observed.
  wait([&] { return count(renamedChild, xiiDirectoryWatcherAction::RenamedNewName) == 1; });
  XII_TEST_INT(count(child, xiiDirectoryWatcherAction::RenamedOldName), 1);
  xiiStringBuilder renamedParent(root, "/MIXED");
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(parent, renamedParent));
  wait([&] { return count(renamedParent, xiiDirectoryWatcherAction::RenamedNewName) == 1; });
  XII_TEST_INT(count(parent, xiiDirectoryWatcherAction::RenamedOldName), 1);

  // A subsequent operation releases ReFS's delayed OLD_NAME records. They must
  // neither duplicate the recovered renames nor corrupt the mirror before deletion.
  xiiStringBuilder poke(root, "/poke.txt");
  {
    xiiOSFile file;
    XII_TEST_RESULT(file.Open(poke, xiiFileOpenMode::Write));
  }
  wait([&] { return count(poke, xiiDirectoryWatcherAction::Added) == 1; });
  xiiUInt32 renameEvents = 0;
  for (const auto& event : events)
    renameEvents += event.action == xiiDirectoryWatcherAction::RenamedOldName || event.action == xiiDirectoryWatcherAction::RenamedNewName;
  XII_TEST_INT(renameEvents, 4);
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(renamedParent));
  xiiStringBuilder removedChild(renamedParent, "/NESTED/child.txt");
  wait([&] { return count(renamedParent, xiiDirectoryWatcherAction::Removed) == 1; });
  XII_TEST_INT(count(removedChild, xiiDirectoryWatcherAction::Removed), 1);
}

XII_CREATE_SIMPLE_TEST(DirectoryRecovery, BatchingAndNonRecursiveScope)
{
  using namespace DirectoryWatcherTestHelpers;
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/DirectoryCasingBatch");
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(root));
  for (xiiUInt32 i = 0; i < 260; ++i)
  {
    xiiStringBuilder path;
    path.SetFormat("{0}/Dir{1}", root, xiiArgU(i, 3, true));
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(path));
  }
  xiiStringBuilder nested(root, "/Dir000/Nested"), renamedNested(root, "/Dir000/NESTED");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(nested));
  xiiDirectoryWatcher watcher;
  XII_SCOPE_EXIT(watcher.CloseDirectory(); xiiOSFile::DeleteFolder(root).IgnoreResult());
  if (!XII_TEST_RESULT(watcher.OpenDirectory(root, xiiDirectoryWatcher::Watch::Renames)))
    return;
  xiiDynamicArray<ExpectedEventStorage> events;
  auto                                  pump = [&] {
    watcher.EnumerateChanges([&](xiiStringView path, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
      events.PushBack({path, action, type});
    },
                             xiiTime::MakeFromMilliseconds(20));
  };
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(nested, renamedNested));
  auto deadline = xiiTime::Now() + xiiTime::MakeFromMilliseconds(250);
  while (xiiTime::Now() < deadline)
    pump();
  XII_TEST_INT(events.GetCount(), 0);
  xiiStringBuilder original(root, "/Dir259"), renamed(root, "/DIR259");
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(original, renamed));
  deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (events.GetCount() < 2 && xiiTime::Now() < deadline)
    pump();
  XII_TEST_INT(events.GetCount(), 2);
  if (events.GetCount() == 2)
  {
    XII_TEST_STRING(events[0].path, original);
    XII_TEST_BOOL(events[0].action == xiiDirectoryWatcherAction::RenamedOldName);
    XII_TEST_STRING(events[1].path, renamed);
    XII_TEST_BOOL(events[1].action == xiiDirectoryWatcherAction::RenamedNewName);
  }
  // A normal rename releases the delayed case record and must still pair correctly.
  xiiStringBuilder moved(root, "/Moved");
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(renamed, moved));
  deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (events.GetCount() < 4 && xiiTime::Now() < deadline)
    pump();
  XII_TEST_INT(events.GetCount(), 4);
  if (events.GetCount() == 4)
  {
    XII_TEST_STRING(events[2].path, renamed);
    XII_TEST_STRING(events[3].path, moved);
  }
}

XII_CREATE_SIMPLE_TEST(DirectoryRecovery, CoalescedCaseChanges)
{
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/DirectoryCasingCoalesced");
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(root));
  xiiStringBuilder parent(root, "/Mixed"), nested(parent, "/Nested"), file(nested, "/child.txt");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(nested));
  {
    xiiOSFile stream;
    XII_TEST_RESULT(stream.Open(file, xiiFileOpenMode::Write));
  }
  xiiDirectoryWatcher watcher;
  XII_SCOPE_EXIT(watcher.CloseDirectory(); xiiOSFile::DeleteFolder(root).IgnoreResult());
  if (!XII_TEST_RESULT(watcher.OpenDirectory(root, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Subdirectories)))
    return;
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(file, xiiStringBuilder(nested, "/CHILD.txt")));
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(nested, xiiStringBuilder(parent, "/NESTED")));
  xiiStringBuilder renamed(root, "/MIXED");
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(parent, renamed));
  xiiUInt32 oldNames = 0, newNames = 0, removedFiles = 0, removedFolders = 0;
  auto      pump = [&] {
    watcher.EnumerateChanges([&](xiiStringView, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
      oldNames += action == xiiDirectoryWatcherAction::RenamedOldName;
      newNames += action == xiiDirectoryWatcherAction::RenamedNewName;
      if (action == xiiDirectoryWatcherAction::Removed)
      {
        if (type == xiiDirectoryWatcherType::File)
          ++removedFiles;
        else
          ++removedFolders;
      }
    },
                             xiiTime::MakeFromMilliseconds(20));
  };
  auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (newNames < 3 && xiiTime::Now() < deadline)
    pump();
  XII_TEST_INT(oldNames, 3);
  XII_TEST_INT(newNames, 3);
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(renamed));
  deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (removedFolders < 2 && xiiTime::Now() < deadline)
    pump();
  XII_TEST_INT(removedFiles, 1);
  XII_TEST_INT(removedFolders, 2);
  XII_TEST_INT(oldNames, 3);
  XII_TEST_INT(newNames, 3);
}

XII_CREATE_SIMPLE_TEST(DirectoryRecovery, DeleteBeforePolling)
{
  using namespace DirectoryWatcherTestHelpers;
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/DirectoryCasingDeleted");
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(root));
  xiiStringBuilder original(root, "/Mixed/Nested"), renamed(root, "/MIXED");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(original));
  {
    xiiOSFile file;
    XII_TEST_RESULT(file.Open(xiiStringBuilder(original, "/child.txt"), xiiFileOpenMode::Write));
  }
  xiiDirectoryWatcher watcher;
  XII_SCOPE_EXIT(watcher.CloseDirectory(); xiiOSFile::DeleteFolder(root).IgnoreResult());
  if (!XII_TEST_RESULT(watcher.OpenDirectory(root, xiiDirectoryWatcher::Watch::Renames | xiiDirectoryWatcher::Watch::Deletes | xiiDirectoryWatcher::Watch::Subdirectories)))
    return;
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(xiiStringBuilder(original, "/child.txt"), xiiStringBuilder(original, "/CHILD.txt")));
  XII_TEST_RESULT(xiiOSFile::MoveFileOrDirectory(xiiStringBuilder(root, "/Mixed"), renamed));
  XII_TEST_RESULT(xiiOSFile::DeleteFolder(renamed));
  xiiUInt32  removedFiles = 0, removedFolders = 0;
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (removedFolders < 2 && xiiTime::Now() < deadline)
    watcher.EnumerateChanges([&](xiiStringView, xiiDirectoryWatcherAction action, xiiDirectoryWatcherType type) {
      if (action == xiiDirectoryWatcherAction::Removed)
      {
        if (type == xiiDirectoryWatcherType::File)
          ++removedFiles;
        else
          ++removedFolders;
      }
    },
                             xiiTime::MakeFromMilliseconds(20));
  XII_TEST_INT(removedFiles, 1);
  XII_TEST_INT(removedFolders, 2);
}
#  endif

#endif
