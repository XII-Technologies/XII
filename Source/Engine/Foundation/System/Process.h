/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>

using xiiOsProcessHandle = void*;
using xiiOsProcessID     = xiiUInt32;

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
enum class xiiProcessState
{
  NotStarted,
  Running,
  Finished
};

/// Options that describe how to run an external process
struct XII_FOUNDATION_DLL xiiProcessOptions
{
  /// Path to the binary to launch
  xiiString m_sProcess;

  /// Custom working directory for the launched process. If empty, inherits the CWD from the parent process.
  xiiString m_sWorkingDirectory;

  /// Arguments to pass to the process. Strings that contain spaces will be wrapped in quotation marks automatically
  xiiHybridArray<xiiString, 8> m_Arguments;

  /// If set to true, command line tools will not show their console window, but execute in the background
  bool m_bHideConsoleWindow = true;

  /// If set, stdout will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  xiiDelegate<void(xiiStringView)> m_onStdOut;

  /// If set, stderr will be captured and this function called on a separate thread. Requires bWaitForResult to be true.
  xiiDelegate<void(xiiStringView)> m_onStdError;

  /// Appends a formatted argument to m_Arguments
  ///
  /// This can be useful if a complex command needs to be added as a single argument.
  /// Ie. since arguments with spaces will be wrapped in quotes, it can make a difference
  /// whether a complex parameter is added as one or multiple arguments.
  void AddArgument(const xiiFormatString& arg);

  /// Overload of AddArgument(xiiFormatString) for convenience.
  template <typename... ARGS>
  void AddArgument(xiiStringView sFormat, ARGS&&... args)
  {
    AddArgument(xiiFormatStringImpl<ARGS...>(sFormat, std::forward<ARGS>(args)...));
  }

  /// Takes a full command line and appends it as individual arguments by splitting it along white-space and quotation marks.
  ///
  /// Brief, use this, if arguments are already pre-built as a full command line.
  void AddCommandLine(xiiStringView sCmdLine);

  /// Builds the command line from the process arguments and appends it to \a out_sCmdLine.
  void BuildCommandLineString(xiiStringBuilder& out_sCmdLine) const;
};

/// Flags for xiiProcess::Launch()
struct xiiProcessLaunchFlags
{
  using StorageType = xiiUInt32;

  enum Enum
  {
    None      = 0,
    Detached  = XII_BIT(0), ///< The process will be detached right after launch, as if xiiProcess::Detach() was called.
    Suspended = XII_BIT(1), ///< The process will be launched in a suspended state. Call xiiProcess::ResumeSuspended() to unpause it.
    Default   = None
  };

  struct Bits
  {
    StorageType Detached : 1;
    StorageType Suspended : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiProcessLaunchFlags);

/// Provides functionality to launch other processes
class XII_FOUNDATION_DLL xiiProcess
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiProcess);

public:
  xiiProcess();
  xiiProcess(xiiProcess&& rhs);

  /// Upon destruction the running process will be terminated.
  ///
  /// Use Detach() to prevent the termination of the launched process.
  ///
  /// \sa Terminate()
  /// \sa Detach()
  ~xiiProcess();

  /// Launches the specified process and waits for it to finish.
  static xiiResult Execute(const xiiProcessOptions& opt, xiiInt32* out_pExitCode = nullptr);

  /// Launches the specified process asynchronously.
  ///
  /// When the function returns, the process is typically starting or running.
  /// Call WaitToFinish() to wait for the process to shutdown or Terminate() to kill it.
  ///
  /// \sa xiiProcessLaunchFlags
  xiiResult Launch(const xiiProcessOptions& opt, xiiBitflags<xiiProcessLaunchFlags> launchFlags = xiiProcessLaunchFlags::None);

  /// Resumes a process that was launched in a suspended state. Returns XII_FAILURE if the process has not been launched or already
  /// resumed.
  xiiResult ResumeSuspended();

  /// Waits the given amount of time for the previously launched process to finish.
  ///
  /// Pass in xiiTime::MakeZero() to wait indefinitely.
  /// Returns XII_FAILURE, if the process did not finish within the given time.
  ///
  /// \note Asserts that the xiiProcess instance was used to successfully launch a process before.
  xiiResult WaitToFinish(xiiTime timeout = xiiTime::MakeZero());

  /// Kills the detached process, if possible.
  xiiResult Terminate();

  /// Returns the exit code of the process. The exit code will be -0xFFFF as long as the process has not finished.
  xiiInt32 GetExitCode() const;

  /// Returns the running state of the process
  ///
  /// If the state is 'finished' the exit code (as returned by GetExitCode() ) will be updated.
  xiiProcessState GetState() const;

  /// Detaches the running process from the xiiProcess instance.
  ///
  /// This means the xiiProcess instance loses control over terminating the process or communicating with it.
  /// It also means that the process will keep running and not get terminated when the xiiProcess instance is destroyed.
  void Detach();

  /// Returns the OS specific handle to the process
  xiiOsProcessHandle GetProcessHandle() const;

  /// Returns the OS-specific process ID (PID)
  xiiOsProcessID GetProcessID() const;

  /// Returns OS-specific process ID (PID) for the calling process
  static xiiOsProcessID GetCurrentProcessID();

private:
  void BuildFullCommandLineString(const xiiProcessOptions& opt, xiiStringView sProcess, xiiStringBuilder& cmd) const;

  xiiUniquePtr<struct xiiProcessImpl> m_pImpl;

  // the default value is used by GetExitCode() to determine whether it has to be reevaluated
  mutable xiiInt32 m_iExitCode = -0xFFFF;

  xiiString                        m_sProcess;
  xiiDelegate<void(xiiStringView)> m_OnStdOut;
  xiiDelegate<void(xiiStringView)> m_OnStdError;
  mutable xiiTime                  m_ProcessExited = xiiTime::MakeZero();
};
#endif
