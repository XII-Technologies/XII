#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/ProcessGroup.h>

struct xiiProcessGroupImpl
{
  HANDLE    m_hJobObject      = INVALID_HANDLE_VALUE;
  HANDLE    m_hCompletionPort = INVALID_HANDLE_VALUE;
  xiiString m_sName;

  ~xiiProcessGroupImpl();
  void Close();
  void Initialize();
};

xiiProcessGroupImpl::~xiiProcessGroupImpl()
{
  Close();
}

void xiiProcessGroupImpl::Close()
{
  if (m_hJobObject != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hJobObject);
    m_hJobObject = INVALID_HANDLE_VALUE;
  }
}

void xiiProcessGroupImpl::Initialize()
{
  if (m_hJobObject == INVALID_HANDLE_VALUE)
  {
    m_hJobObject = CreateJobObjectW(nullptr, nullptr);

    if (m_hJobObject == nullptr || m_hJobObject == INVALID_HANDLE_VALUE)
    {
      xiiLog::Error("Failed to create process group '{}' - {}", m_sName, xiiArgErrorCode(GetLastError()));
      return;
    }

    // Configure the job object such that it kill all processes once this job object is cleaned up
    // ie. Either when all job object handles are closed, or the application crashes

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION exinfo = {0};
    exinfo.BasicLimitInformation.LimitFlags     = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (SetInformationJobObject(m_hJobObject, JobObjectExtendedLimitInformation, &exinfo, sizeof(exinfo)) == FALSE)
    {
      xiiLog::Error("xiiProcessGroup: failed to configure 'kill jobs on close' - '{}'", xiiArgErrorCode(GetLastError()));
    }

    // the completion port is necessary to implement WaitToFinish()
    // see https://devblogs.microsoft.com/oldnewthing/20130405-00/?p=4743
    m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

    JOBOBJECT_ASSOCIATE_COMPLETION_PORT Port;
    Port.CompletionKey  = m_hJobObject;
    Port.CompletionPort = m_hCompletionPort;
    SetInformationJobObject(m_hJobObject, JobObjectAssociateCompletionPortInformation, &Port, sizeof(Port));
  }
}

xiiProcessGroup::xiiProcessGroup(xiiStringView sGroupName)
{
  m_pImpl          = XII_DEFAULT_NEW(xiiProcessGroupImpl);
  m_pImpl->m_sName = sGroupName;
}

xiiProcessGroup::~xiiProcessGroup()
{
  TerminateAll().IgnoreResult();
}

xiiResult xiiProcessGroup::Launch(const xiiProcessOptions& opt)
{
  m_pImpl->Initialize();

  xiiProcess& process = m_Processes.ExpandAndGetRef();
  XII_SUCCEED_OR_RETURN(process.Launch(opt, xiiProcessLaunchFlags::Suspended));

  if (AssignProcessToJobObject(m_pImpl->m_hJobObject, process.GetProcessHandle()) == FALSE)
  {
    xiiLog::Error("Failed to add process to process group '{}' - {}", m_pImpl->m_sName, xiiArgErrorCode(GetLastError()));
    m_Processes.PopBack();
    return XII_FAILURE;
  }

  if (process.ResumeSuspended().Failed())
  {
    xiiLog::Error("Failed to resume the given process. Processes must be launched in a suspended state before adding them to process groups.");
    m_Processes.PopBack();
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiProcessGroup::WaitToFinish(xiiTime timeout /*= xiiTime::Zero()*/)
{
  if (m_pImpl->m_hJobObject == INVALID_HANDLE_VALUE)
    return XII_SUCCESS;

  // Check if no new processes were launched, because waiting could end up in an infinite loop,
  // so don't even try in this case
  bool allProcessesGone = true;
  for (const xiiProcess& p : m_Processes)
  {
    DWORD exitCode = 0;
    GetExitCodeProcess(p.GetProcessHandle(), &exitCode);
    if (exitCode == STILL_ACTIVE)
    {
      allProcessesGone = false;
      break;
    }
  }

  if (allProcessesGone)
  {
    // We need to wait for processes even if the job is done as the threads for the pipes are potentially still alive and lead to incomplete stdout / stderr output even though the process has exited.
    for (xiiProcess& p : m_Processes)
    {
      p.WaitToFinish().IgnoreResult();
    }
    m_pImpl->Close();
    return XII_SUCCESS;
  }

  DWORD dwTimeout = INFINITE;

  if (timeout.IsPositive())
    dwTimeout = (DWORD)timeout.GetMilliseconds();
  else
    dwTimeout = INFINITE;

  DWORD        CompletionCode;
  ULONG_PTR    CompletionKey;
  LPOVERLAPPED Overlapped;

  xiiTime tStart = xiiTime::Now();

  while (true)
  {
    // ATTENTION !
    // If you are looking at a crash dump of XII this line will typically be at the top of the callstack.
    // That is because to write the crash dump an external process is called and this is where we are waiting for that process to finish.
    // To see the actual reason for the crash, locate the call to xiiCrashHandlerFunc further down in the callstack.
    // The crashing code is usually the one calling that function.

    if (GetQueuedCompletionStatus(m_pImpl->m_hCompletionPort, &CompletionCode, &CompletionKey, &Overlapped, dwTimeout) == FALSE)
    {
      DWORD res = GetLastError();

      if (res != WAIT_TIMEOUT)
      {
        xiiLog::Error("Failed to wait for process group '{}' - {}", m_pImpl->m_sName, xiiArgErrorCode(res));
      }

      return XII_FAILURE;
    }

    // We got the expected result, all processes have finished
    if (((HANDLE)CompletionKey == m_pImpl->m_hJobObject && CompletionCode == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO))
    {
      // We need to wait for processes even if the job is done as the threads for the pipes are potentially still alive and lead to incomplete stdout / stderr output even though the process has exited.
      for (xiiProcess& p : m_Processes)
      {
        p.WaitToFinish().IgnoreResult();
      }

      m_pImpl->Close();
      return XII_SUCCESS;
    }

    // We got some different message, ignore this.
    // However, we need to adjust our timeout

    if (timeout.IsPositive())
    {
      // Subtract the time that we spent
      const xiiTime now = xiiTime::Now();
      timeout -= now - tStart;
      tStart = now;

      // The timeout has been reached
      if (timeout.IsZeroOrNegative())
      {
        return XII_FAILURE;
      }

      // Otherwise, try again, but with a reduced timeout
      dwTimeout = (DWORD)timeout.GetMilliseconds();
    }
  }
}

xiiResult xiiProcessGroup::TerminateAll(xiiInt32 iForcedExitCode /*= -2*/)
{
  if (m_pImpl->m_hJobObject == INVALID_HANDLE_VALUE)
    return XII_SUCCESS;

  if (TerminateJobObject(m_pImpl->m_hJobObject, (UINT)iForcedExitCode) == FALSE)
  {
    xiiLog::Error("Failed to terminate process group '{}' - {}", m_pImpl->m_sName, xiiArgErrorCode(GetLastError()));
    return XII_FAILURE;
  }

  XII_SUCCEED_OR_RETURN(WaitToFinish());

  return XII_SUCCESS;
}
