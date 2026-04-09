#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Timestamp.h>

static void PrintHelper(const char* szString)
{
  xiiLog::Printf("%s", szString);
}

//////////////////////////////////////////////////////////////////////////

xiiCrashHandler* xiiCrashHandler::s_pActiveHandler = nullptr;

xiiCrashHandler::xiiCrashHandler() = default;

xiiCrashHandler::~xiiCrashHandler()
{
  if (s_pActiveHandler == this)
  {
    SetCrashHandler(nullptr);
  }
}

xiiCrashHandler* xiiCrashHandler::GetCrashHandler()
{
  return s_pActiveHandler;
}

//////////////////////////////////////////////////////////////////////////

xiiCrashHandler_WriteMiniDump xiiCrashHandler_WriteMiniDump::g_Instance;

xiiCrashHandler_WriteMiniDump::xiiCrashHandler_WriteMiniDump() = default;

void xiiCrashHandler_WriteMiniDump::SetFullDumpFilePath(xiiStringView sFullAbsDumpFilePath)
{
  m_sDumpFilePath = sFullAbsDumpFilePath;
}

void xiiCrashHandler_WriteMiniDump::SetDumpFilePath(xiiStringView sAbsDirectoryPath, xiiStringView sAppName, xiiBitflags<PathFlags> flags)
{
  xiiStringBuilder sOutputPath = sAbsDirectoryPath;

  if (flags.IsSet(PathFlags::AppendSubFolder))
  {
    sOutputPath.AppendPath("CrashDumps");
  }

  sOutputPath.AppendPath(sAppName);

  if (flags.IsSet(PathFlags::AppendDate))
  {
    const xiiDateTime date = xiiDateTime::MakeFromTimestamp(xiiTimestamp::CurrentTimestamp());
    sOutputPath.AppendFormat("_{}", date);
  }

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
  if (flags.IsSet(PathFlags::AppendPID))
  {
    const xiiUInt32 pid = xiiProcess::GetCurrentProcessID();
    sOutputPath.AppendFormat("_{}", pid);
  }
#endif

  sOutputPath.Append(".dmp");

  SetFullDumpFilePath(sOutputPath);
}

void xiiCrashHandler_WriteMiniDump::SetDumpFilePath(xiiStringView sAppName, xiiBitflags<PathFlags> flags)
{
  SetDumpFilePath(xiiOSFile::GetApplicationDirectory(), sAppName, flags);
}

void xiiCrashHandler_WriteMiniDump::HandleCrash(void* pOsSpecificData)
{
  bool crashDumpWritten = false;
  if (!m_sDumpFilePath.IsEmpty())
  {
#if XII_ENABLED(XII_SUPPORTS_CRASH_DUMPS)
    if (xiiMiniDumpUtils::LaunchMiniDumpTool(m_sDumpFilePath).Failed())
    {
      xiiLog::Print("Could not launch MiniDumpTool, trying to write crash-dump from crashed process directly.\n");

      crashDumpWritten = WriteOwnProcessMiniDump(pOsSpecificData);
    }
    else
    {
      crashDumpWritten = true;
    }
#else
    crashDumpWritten = WriteOwnProcessMiniDump(pOsSpecificData);
#endif
  }
  else
  {
    xiiLog::Print("xiiCrashHandler_WriteMiniDump: No dump-file location specified.\n");
  }

  PrintStackTrace(pOsSpecificData);

  if (crashDumpWritten)
  {
    xiiLog::Printf("Application crashed. Crash-dump written to '%s'\n.", m_sDumpFilePath.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/CrashHandler_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/CrashHandler_posix.h>
#else
#  error "xiiCrashHandler is not implemented on current platform"
#endif


XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_CrashHandler);
