/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>

/// Helper class to manage the top level exception handler.
///
/// A default exception handler is provided but not set by default.
/// The default implementation will write the exception and callstack to the output
/// and create a memory dump using WriteDump that create a dump file in the folder
/// specified via SetExceptionHandler.

/// This class allows to hook into the OS top-level exception handler to handle application crashes
///
/// Derive from this class to implement custom behavior. Call xiiCrashHandler::SetCrashHandler() to
/// register which instance to use.
///
/// For typical use-cases use xiiCrashHandler_WriteMiniDump::g_Instance.
class XII_FOUNDATION_DLL xiiCrashHandler
{
public:
  xiiCrashHandler();
  virtual ~xiiCrashHandler();

  static void             SetCrashHandler(xiiCrashHandler* pHandler);
  static xiiCrashHandler* GetCrashHandler();

  virtual void HandleCrash(void* pOsSpecificData) = 0;

private:
  static xiiCrashHandler* s_pActiveHandler;
};

/// A default implementation of xiiCrashHandler that tries to write a mini-dump and prints the callstack.
///
/// To use it, call xiiCrashHandler::SetCrashHandler(&xiiCrashHandler_WriteMiniDump::g_Instance);
/// Do not forget to also specify the dump-file path, otherwise writing dump-files is skipped.
class XII_FOUNDATION_DLL xiiCrashHandler_WriteMiniDump : public xiiCrashHandler
{
public:
  static xiiCrashHandler_WriteMiniDump g_Instance;

  struct PathFlags
  {
    using StorageType = xiiUInt8;

    enum Enum
    {
      AppendDate      = XII_BIT(0), ///< Whether to append the current date to the crash-dump file (YYYY-MM-DD_HH-MM-SS)
      AppendSubFolder = XII_BIT(1), ///< Whether to append "CrashDump" as a sub-folder
      AppendPID       = XII_BIT(2), ///< Whether to append the process ID to the crash-dump file

      Default = AppendDate | AppendSubFolder | AppendPID
    };

    struct Bits
    {
      StorageType AppendDate : 1;
      StorageType AppendSubFolder : 1;
      StorageType AppendPID : 1;
    };
  };

public:
  xiiCrashHandler_WriteMiniDump();

  /// Sets the raw path for the dump-file to write
  void SetFullDumpFilePath(xiiStringView sFullAbsDumpFilePath);

  /// Sets the dump-file path to "{szAbsDirectoryPath}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(xiiStringView sAbsDirectoryPath, xiiStringView sAppName, xiiBitflags<PathFlags> flags = PathFlags::Default);

  /// Sets the dump-file path to "{xiiOSFile::GetApplicationDirectory()}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(xiiStringView sAppName, xiiBitflags<PathFlags> flags = PathFlags::Default);

  virtual void HandleCrash(void* pOsSpecificData) override;

protected:
  virtual bool WriteOwnProcessMiniDump(void* pOsSpecificData);
  virtual void PrintStackTrace(void* pOsSpecificData);

  xiiString m_sDumpFilePath;
};

XII_DECLARE_FLAGS_OPERATORS(xiiCrashHandler_WriteMiniDump::PathFlags);
