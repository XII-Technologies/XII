#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/MinWindows.h>
#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)

#  include <Dbghelp.h>
#  include <Shlwapi.h>
#  include <tchar.h>
#  include <werapi.h>

xiiCommandLineOptionBool opt_FullCrashDumps("app", "-fullcrashdumps", "If enabled, crash dumps will contain the full memory image.", false);

using MINIDUMPWRITEDUMP = BOOL(WINAPI*)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

xiiMinWindows::HANDLE xiiMiniDumpUtils::GetProcessHandleWithNecessaryRights(xiiUInt32 uiProcessID)
{
  // Try to get more than we need
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, uiProcessID);

  if (hProcess == NULL)
  {
    // Try to get all that we need for a nice dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, uiProcessID);
  }

  if (hProcess == NULL)
  {
    // Try to get rights for a limited dump
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, uiProcessID);
  }

  return hProcess;
}

xiiStatus xiiMiniDumpUtils::WriteProcessMiniDump(
  const char*                 szDumpFile,
  xiiUInt32                   uiProcessID,
  xiiMinWindows::HANDLE       pProcess,
  struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  HMODULE hDLL = ::LoadLibraryA("dbghelp.dll");

  if (hDLL == nullptr)
  {
    return xiiStatus("dbghelp.dll could not be loaded.");
  }

  MINIDUMPWRITEDUMP MiniDumpWriteDumpFunc = (MINIDUMPWRITEDUMP)::GetProcAddress(hDLL, "MiniDumpWriteDump");

  if (MiniDumpWriteDumpFunc == nullptr)
  {
    return xiiStatus("'MiniDumpWriteDump' function address could not be resolved.");
  }

  xiiUInt32 dumpType = MiniDumpWithHandleData | MiniDumpWithModuleHeaders | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
    MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo;

  if (opt_FullCrashDumps.GetOptionValue(xiiCommandLineOption::LogMode::Always))
  {
    dumpType |= MiniDumpWithFullMemory;
  }

  // Make sure the target folder exists
  {
    xiiStringBuilder folder = szDumpFile;
    folder.PathParentDirectory();
    if (xiiOSFile::CreateDirectoryStructure(folder).Failed())
      return xiiStatus("Failed to create output directory structure.");
  }

  HANDLE hFile = CreateFileW(xiiDosDevicePath(szDumpFile), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile == INVALID_HANDLE_VALUE)
  {
    return xiiStatus(xiiFmt("Creating dump file '{}' failed (Error: '{}').", szDumpFile, xiiArgErrorCode(GetLastError())));
  }

  XII_SCOPE_EXIT(CloseHandle(hFile););

  MINIDUMP_EXCEPTION_INFORMATION exceptionParam;
  exceptionParam.ThreadId          = GetCurrentThreadId(); // This is only valid for WriteOwnProcessMiniDump()
  exceptionParam.ExceptionPointers = pExceptionInfo;
  exceptionParam.ClientPointers    = TRUE;

  if (MiniDumpWriteDumpFunc(
        pProcess, uiProcessID, hFile, (MINIDUMP_TYPE)dumpType, pExceptionInfo != nullptr ? &exceptionParam : nullptr, nullptr, nullptr) == FALSE)
  {
    return xiiStatus(xiiFmt("Writing dump file failed: '{}'.", xiiArgErrorCode(GetLastError())));
  }

  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiMiniDumpUtils::WriteOwnProcessMiniDump(const char* szDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo)
{
  return WriteProcessMiniDump(szDumpFile, GetCurrentProcessId(), GetCurrentProcess(), pExceptionInfo);
}

xiiStatus xiiMiniDumpUtils::WriteExternalProcessMiniDump(const char* szDumpFile, xiiUInt32 uiProcessID, xiiMinWindows::HANDLE pProcess)
{
  return WriteProcessMiniDump(szDumpFile, uiProcessID, pProcess, nullptr);
}

#endif

xiiStatus xiiMiniDumpUtils::WriteExternalProcessMiniDump(const char* szDumpFile, xiiUInt32 uiProcessID)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  HANDLE hProcess = xiiMiniDumpUtils::GetProcessHandleWithNecessaryRights(uiProcessID);

  if (hProcess == nullptr)
  {
    return xiiStatus("Cannot access process for mini-dump writing (PID invalid or not enough rights).");
  }

  return WriteProcessMiniDump(szDumpFile, uiProcessID, hProcess, nullptr);

#else
  return xiiStatus("Not implemented on UWP");
#endif
}

xiiStatus xiiMiniDumpUtils::LaunchMiniDumpTool(const char* szDumpFile)
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  xiiStringBuilder sDumpToolPath = xiiOSFile::GetApplicationDirectory();
  sDumpToolPath.AppendPath("MiniDumpTool.exe");
  sDumpToolPath.MakeCleanPath();

  if (!xiiOSFile::ExistsFile(sDumpToolPath))
    return xiiStatus(xiiFmt("MiniDumpTool.exe not found in '{}'", sDumpToolPath));

  xiiProcessOptions procOpt;
  procOpt.m_sProcess = sDumpToolPath;
  procOpt.m_Arguments.PushBack("-PID");
  procOpt.AddArgument("{}", xiiProcess::GetCurrentProcessID());
  procOpt.m_Arguments.PushBack("-f");
  procOpt.m_Arguments.PushBack(szDumpFile);

  if (opt_FullCrashDumps.GetOptionValue(xiiCommandLineOption::LogMode::Always))
  {
    // Forward the '-fullcrashdumps' command line argument
    procOpt.AddArgument("-fullcrashdumps");
  }

  xiiProcessGroup proc;
  if (proc.Launch(procOpt).Failed())
    return xiiStatus(xiiFmt("Failed to launch '{}'", sDumpToolPath));

  if (proc.WaitToFinish().Failed())
    return xiiStatus("Waiting for MiniDumpTool to finish failed.");

  return xiiStatus(XII_SUCCESS);

#else
  return xiiStatus("Not implemented on UWP");
#endif
}
