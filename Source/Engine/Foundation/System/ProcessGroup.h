/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)
#  include <Foundation/System/Process.h>

/// Process groups are used to tie multiple processes together and ensure they get terminated either on demand or when the
/// application crashes
///
/// On Windows when a xiiProcessGroup instance is destroyed (either normally or due to a crash), all processes that have
/// been added to the group will be terminated by the OS. Other operating systems do not provide the terminate on crash guarantee.
///
/// Only processes that were launched asynchronously and in a suspended state can be added to process groups.
/// They will be resumed by the group.
class XII_FOUNDATION_DLL xiiProcessGroup
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiProcessGroup);

public:
  /// Creates a process group. The name is only used for debugging purposes.
  xiiProcessGroup(xiiStringView sGroupName = {});
  ~xiiProcessGroup();

  /// Launches a new process in the group.
  xiiResult Launch(const xiiProcessOptions& opt);

  /// Waits for all the processes in the group to terminate.
  ///
  /// Returns XII_SUCCESS only if all processes have shut down.
  /// In all other cases, e.g. if the optional timeout is reached,
  /// XII_FAILURE is returned.
  xiiResult WaitToFinish(xiiTime timeout = xiiTime::MakeZero());

  /// Tries to kill all processes associated with this group.
  ///
  /// Sends a kill command to all processes and then waits indefinitely for them to terminate.
  /// Note: iForcedExitCode is only supported on Windows.
  xiiResult TerminateAll(xiiInt32 iForcedExitCode = -2);

  /// Returns the container holding all processes of this group.
  ///
  /// This can be used to query per-process information such as exit codes.
  const xiiHybridArray<xiiProcess, 8>& GetProcesses() const;

private:
  xiiUniquePtr<struct xiiProcessGroupImpl> m_pImpl;

  xiiHybridArray<xiiProcess, 8> m_Processes;
};
#endif
