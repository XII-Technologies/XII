/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Basics/Platform/Windows/MinWindows.h>
#include <Foundation/Types/Status.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
extern "C"
{
  struct _EXCEPTION_POINTERS;
}
#endif

/// Functionality for writing process mini-dumps (callstacks, memory snapshots, etc)
struct XII_FOUNDATION_DLL xiiMiniDumpUtils
{
  /// Tries to write a mini-dump for the external process with the given process ID.
  ///
  /// \sa WriteProcessMiniDump()
  static xiiStatus WriteExternalProcessMiniDump(xiiStringView sDumpFile, xiiUInt32 uiProcessID);

  /// Tries to launch xii's 'MiniDumpTool' to write a mini-dump for THIS process (the recommended way when an application is crashing).
  ///
  /// \note On Windows: If the command line option '-fullcrashdumps' is specified, it is forwarded to the MiniDumpTool.
  static xiiStatus LaunchMiniDumpTool(xiiStringView sDumpFile);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  /// Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \sa WriteProcessMiniDump()
  static xiiStatus WriteOwnProcessMiniDump(xiiStringView sDumpFile, struct _EXCEPTION_POINTERS* pExceptionInfo);

  /// Given a process ID this function tries to get a HANDLE to the process with the necessary access rights to write a mini-dump.
  static xiiMinWindows::HANDLE GetProcessHandleWithNecessaryRights(xiiUInt32 uiProcessID);

  /// Windows-specific implementation for writing a mini-dump of another process.
  ///
  /// \sa WriteProcessMiniDump()
  static xiiStatus WriteExternalProcessMiniDump(xiiStringView sDumpFile, xiiUInt32 uiProcessID, xiiMinWindows::HANDLE hProcess);

  /// Windows-specific implementation for writing a mini-dump of the running process.
  ///
  /// \note On Windows: If the command line option '-fullcrashdumps' is specified, a crash-dump with a full memory capture is made.
  static xiiStatus WriteProcessMiniDump(xiiStringView sDumpFile, xiiUInt32 uiProcessID, xiiMinWindows::HANDLE hProcess, struct _EXCEPTION_POINTERS* pExceptionInfo);
#endif
};
