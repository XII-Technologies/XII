#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

static xiiInt32                                                                            s_iDataDirCounter = 0;
static xiiMap<xiiString, xiiInt32, xiiCompareHelper<xiiString>, xiiStaticAllocatorWrapper> s_KnownDataDirs;

static void FileSystemEventHandler(const xiiFileSystem::FileEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiFileSystem::FileEventType::AddDataDirectorySucceeded:
    {
      bool bExisted = false;
      auto it       = s_KnownDataDirs.FindOrAdd(e.m_szFileOrDirectory, &bExisted);

      if (!bExisted)
      {
        it.Value() = s_iDataDirCounter;
        ++s_iDataDirCounter;
      }

      xiiStringBuilder sName;
      sName.Format("IO/DataDirs/Dir{0}", xiiArgI(it.Value(), 2, true));

      xiiStats::SetStat(sName.GetData(), e.m_szFileOrDirectory);
    }
    break;

    case xiiFileSystem::FileEventType::RemoveDataDirectory:
    {
      auto it = s_KnownDataDirs.Find(e.m_szFileOrDirectory);

      if (!it.IsValid())
        break;

      xiiStringBuilder sName;
      sName.Format("IO/DataDirs/Dir{0}", xiiArgI(it.Value(), 2, true));

      xiiStats::RemoveStat(sName.GetData());
    }
    break;

    default:
      break;
  }
}

void AddFileSystemEventHandler()
{
  xiiFileSystem::RegisterEventHandler(FileSystemEventHandler);
}

void RemoveFileSystemEventHandler()
{
  xiiFileSystem::UnregisterEventHandler(FileSystemEventHandler);
}
