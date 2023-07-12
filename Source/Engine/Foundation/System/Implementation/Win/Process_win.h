#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <future>

struct xiiPipeWin
{
  HANDLE            m_pipeRead  = nullptr;
  HANDLE            m_pipeWrite = nullptr;
  std::thread       m_readThread;
  std::atomic<bool> m_running = false;

  bool IsRunning() const
  {
    return m_running;
  }

  void Create()
  {
    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr.nLength              = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle       = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe for the child process.
    if (!CreatePipe(&m_pipeRead, &m_pipeWrite, &saAttr, 0))
      xiiLog::Error("xiiPipeWin: CreatePipe failed");

    // Ensure the read handle to the pipe is not inherited.
    if (!SetHandleInformation(m_pipeRead, HANDLE_FLAG_INHERIT, 0))
      xiiLog::Error("Stdout SetHandleInformation");
  }

  void Close()
  {
    if (m_pipeWrite)
    {
      CloseHandle(m_pipeWrite);
      m_pipeWrite = nullptr;

      if (m_readThread.joinable())
      {
        m_readThread.join();
      }
      CloseHandle(m_pipeRead);
      m_pipeRead = nullptr;
    }
  }

  static void ReportString(xiiDelegate<void(xiiStringView)> func, xiiHybridArray<char, 256>& ref_temp)
  {
    xiiStringBuilder result;

    xiiUnicodeUtils::RepairNonUtf8Text(ref_temp.GetData(), ref_temp.GetData() + ref_temp.GetCount(), result);
    func(result);
  }

  static void ReportString(xiiDelegate<void(xiiStringView)> func, const char* szStart, const char* szEnd)
  {
    xiiHybridArray<char, 256> tmp;

    while (szStart < szEnd)
    {
      tmp.PushBack(*szStart);
      ++szStart;
    }

    ReportString(func, tmp);
  }

  void StartRead(xiiDelegate<void(xiiStringView)>& ref_onStdOut)
  {
    if (m_pipeWrite)
    {
      m_running    = true;
      m_readThread = std::thread([&]() {
        xiiHybridArray<char, 256> overflowBuffer;

        constexpr int BUFSIZE = 512;
        char          chBuf[BUFSIZE];
        while (true)
        {
          DWORD bytesRead = 0;
          bool  res       = ReadFile(m_pipeRead, chBuf, BUFSIZE, &bytesRead, nullptr);
          if (!res || bytesRead == 0)
          {
            if (!overflowBuffer.IsEmpty())
            {
              ReportString(ref_onStdOut, overflowBuffer);
            }
            break;
          }

          const char* szCurrentPos = chBuf;
          const char* szEndPos     = chBuf + bytesRead;

          while (szCurrentPos < szEndPos)
          {
            const char* szFound = xiiStringUtils::FindSubString(szCurrentPos, "\n", szEndPos);
            if (szFound)
            {
              if (overflowBuffer.IsEmpty())
              {
                // If there is nothing in the overflow buffer this is a complete line and can be fired as is.
                ReportString(ref_onStdOut, szCurrentPos, szFound + 1);
              }
              else
              {
                // We have data in the overflow buffer so this is the final part of a partial line so we need to complete and fire the overflow buffer.

                while (szCurrentPos < szFound + 1)
                {
                  overflowBuffer.PushBack(*szCurrentPos);
                  ++szCurrentPos;
                }

                ReportString(ref_onStdOut, overflowBuffer);

                overflowBuffer.Clear();
              }

              szCurrentPos = szFound + 1;
            }
            else
            {
              // This is either the start or a middle segment of a line, append to overflow buffer.

              while (szCurrentPos < szEndPos)
              {
                overflowBuffer.PushBack(*szCurrentPos);
                ++szCurrentPos;
              }
            }
          }
        }
        m_running = false;
        //
      });
    }
  }
};

struct xiiProcessImpl
{
  xiiOsProcessHandle m_ProcessHandle    = nullptr;
  xiiOsProcessHandle m_MainThreadHandle = nullptr;
  xiiOsProcessID     m_ProcessID        = 0;
  xiiPipeWin         m_pipeStdOut;
  xiiPipeWin         m_pipeStdErr;

  ~xiiProcessImpl() { Close(); }

  void Close()
  {
    if (m_MainThreadHandle != nullptr)
    {
      CloseHandle(m_MainThreadHandle);
      m_MainThreadHandle = nullptr;
    }

    if (m_ProcessHandle != nullptr)
    {
      CloseHandle(m_ProcessHandle);
      m_ProcessHandle = nullptr;
    }

    m_pipeStdOut.Close();
    m_pipeStdErr.Close();
  }
};

xiiProcess::xiiProcess()
{
  m_pImpl = XII_DEFAULT_NEW(xiiProcessImpl);
}

xiiProcess::~xiiProcess()
{
  if (GetState() == xiiProcessState::Running)
  {
    xiiLog::Dev("Process still running - terminating '{}'", m_sProcess);

    Terminate().IgnoreResult();
  }

  // Explicitly clear the implementation here so that member
  // state (e.g. delegates) used by the impl survives the implementation.
  m_pImpl.Clear();
}

xiiOsProcessHandle xiiProcess::GetProcessHandle() const
{
  return m_pImpl->m_ProcessHandle;
}

xiiOsProcessID xiiProcess::GetProcessID() const
{
  return m_pImpl->m_ProcessID;
}

xiiOsProcessID xiiProcess::GetCurrentProcessID()
{
  const xiiOsProcessID processID = GetCurrentProcessId();
  return processID;
}


// Taken from "Programmatically controlling which handles are inherited by new processes in Win32" by Raymond Chen
// https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
static BOOL CreateProcessWithExplicitHandles(LPCWSTR pLpApplicationName, LPWSTR pLpCommandLine, LPSECURITY_ATTRIBUTES pLpProcessAttributes, LPSECURITY_ATTRIBUTES pLpThreadAttributes, BOOL inheritHandles, DWORD uiDwCreationFlags, LPVOID pLpEnvironment, LPCWSTR pLpCurrentDirectory, LPSTARTUPINFOW pLpStartupInfo, LPPROCESS_INFORMATION pLpProcessInformation,
                                             // here is the new stuff
                                             DWORD   uiHandlesToInherit,
                                             HANDLE* pRgHandlesToInherit)
{
  BOOL                         fSuccess;
  BOOL                         fInitialized    = FALSE;
  SIZE_T                       size            = 0;
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList = nullptr;
  fSuccess                                     = uiHandlesToInherit < 0xFFFFFFFF / sizeof(HANDLE) && pLpStartupInfo->cb == sizeof(*pLpStartupInfo);
  if (!fSuccess)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
  }

  if (uiHandlesToInherit > 0)
  {
    if (fSuccess)
    {
      fSuccess = InitializeProcThreadAttributeList(nullptr, 1, 0, &size) || GetLastError() == ERROR_INSUFFICIENT_BUFFER;
    }
    if (fSuccess)
    {
      lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, size));
      fSuccess        = lpAttributeList != nullptr;
    }
    if (fSuccess)
    {
      fSuccess = InitializeProcThreadAttributeList(lpAttributeList, 1, 0, &size);
    }
    if (fSuccess)
    {
      fInitialized = TRUE;
      fSuccess     = UpdateProcThreadAttribute(lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, pRgHandlesToInherit, uiHandlesToInherit * sizeof(HANDLE), nullptr, nullptr);
    }
  }

  if (fSuccess)
  {
    STARTUPINFOEXW info;
    ZeroMemory(&info, sizeof(info));
    info.StartupInfo     = *pLpStartupInfo;
    info.StartupInfo.cb  = sizeof(info);
    info.lpAttributeList = lpAttributeList;

    // It is both possible to pass in (STARTUPINFOW*)&info OR info.StartupInfo ...
    fSuccess = CreateProcessW(pLpApplicationName, pLpCommandLine, pLpProcessAttributes, pLpThreadAttributes, inheritHandles,
                              uiDwCreationFlags | EXTENDED_STARTUPINFO_PRESENT, pLpEnvironment, pLpCurrentDirectory, &info.StartupInfo, pLpProcessInformation);
  }

  if (fInitialized)
    DeleteProcThreadAttributeList(lpAttributeList);

  if (lpAttributeList)
    HeapFree(GetProcessHeap(), 0, lpAttributeList);

  return fSuccess;
}

xiiResult xiiProcess::Launch(const xiiProcessOptions& opt, xiiBitflags<xiiProcessLaunchFlags> launchFlags /*= xiiAsyncProcessFlags::None*/)
{
  XII_ASSERT_DEV(m_pImpl->m_ProcessHandle == nullptr, "Cannot reuse an instance of xiiProcess");
  XII_ASSERT_DEV(m_pImpl->m_ProcessID == 0, "Cannot reuse an instance of xiiProcess");

  xiiStringBuilder sProcess = opt.m_sProcess;
  sProcess.MakeCleanPath();
  sProcess.ReplaceAll("/", "\\");

  m_sProcess   = sProcess;
  m_OnStdOut   = opt.m_onStdOut;
  m_OnStdError = opt.m_onStdError;

  STARTUPINFOW startupInformation;
  xiiMemoryUtils::ZeroFill(&startupInformation, 1);
  startupInformation.cb      = sizeof(startupInformation);
  startupInformation.dwFlags = STARTF_FORCEOFFFEEDBACK; // Do not show a wait cursor while launching the process

  // Attention: passing in even a single null handle will fail the handle inheritance entirely,
  // but CreateProcess will still return success.
  // Therefore we must ensure to only pass non-null handles to inherit
  HANDLE    HandlesToInherit[2];
  xiiUInt32 uiNumHandlesToInherit = 0;

  if (m_OnStdOut.IsValid())
  {
    m_pImpl->m_pipeStdOut.Create();
    startupInformation.hStdOutput = m_pImpl->m_pipeStdOut.m_pipeWrite;
    startupInformation.dwFlags |= STARTF_USESTDHANDLES;
    HandlesToInherit[uiNumHandlesToInherit++] = m_pImpl->m_pipeStdOut.m_pipeWrite;
  }
  if (m_OnStdError.IsValid())
  {
    m_pImpl->m_pipeStdErr.Create();
    startupInformation.hStdError = m_pImpl->m_pipeStdErr.m_pipeWrite;
    startupInformation.dwFlags |= STARTF_USESTDHANDLES;
    HandlesToInherit[uiNumHandlesToInherit++] = m_pImpl->m_pipeStdErr.m_pipeWrite;
  }

  PROCESS_INFORMATION processInformation;
  xiiMemoryUtils::ZeroFill(&processInformation, 1);

  xiiStringBuilder sCmdLine;
  BuildFullCommandLineString(opt, sProcess, sCmdLine);

  DWORD dwCreationFlags = NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT;

  if (opt.m_bHideConsoleWindow)
  {
    dwCreationFlags |= CREATE_NO_WINDOW;
  }

  if (launchFlags.IsSet(xiiProcessLaunchFlags::Suspended))
  {
    dwCreationFlags |= CREATE_SUSPENDED;
  }

  // We pass nullptr as lpApplicationName as setting it would prevent OpenProcess to run system apps or apps in PATH.
  // Instead, the module name is pre-pended to lpCommandLine in BuildFullCommandLineString.
  if (!CreateProcessWithExplicitHandles(nullptr, const_cast<wchar_t*>(xiiStringWChar(sCmdLine).GetData()),
                                        nullptr,                                  // lpProcessAttributes
                                        nullptr,                                  // lpThreadAttributes
                                        uiNumHandlesToInherit > 0 ? TRUE : FALSE, // bInheritHandles
                                        dwCreationFlags,
                                        nullptr, // lpEnvironment
                                        opt.m_sWorkingDirectory.IsEmpty() ? nullptr : xiiStringWChar(opt.m_sWorkingDirectory).GetData(),
                                        &startupInformation,   // lpStartupInfo
                                        &processInformation,   // lpProcessInformation
                                        uiNumHandlesToInherit, // cHandlesToInherit
                                        HandlesToInherit       // rgHandlesToInherit
                                        ))
  {
    m_pImpl->m_pipeStdOut.Close();
    m_pImpl->m_pipeStdErr.Close();
    xiiLog::Error("Failed to launch '{} {}' - {}", sProcess, xiiArgSensitive(sCmdLine, "CommandLine"), xiiArgErrorCode(GetLastError()));
    return XII_FAILURE;
  }
  m_pImpl->m_pipeStdOut.StartRead(m_OnStdOut);
  m_pImpl->m_pipeStdErr.StartRead(m_OnStdError);

  m_pImpl->m_ProcessHandle = processInformation.hProcess;
  m_pImpl->m_ProcessID     = processInformation.dwProcessId;

  if (launchFlags.IsSet(xiiProcessLaunchFlags::Suspended))
  {
    // Store the main thread handle for ResumeSuspended() later
    m_pImpl->m_MainThreadHandle = processInformation.hThread;
  }
  else
  {
    CloseHandle(processInformation.hThread);
  }

  if (launchFlags.IsSet(xiiProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return XII_SUCCESS;
}

xiiResult xiiProcess::ResumeSuspended()
{
  if (m_pImpl->m_ProcessHandle == nullptr || m_pImpl->m_MainThreadHandle == nullptr)
    return XII_FAILURE;

  ResumeThread(m_pImpl->m_MainThreadHandle);

  // Invalidate the thread handle, so that we cannot resume the process twice
  CloseHandle(m_pImpl->m_MainThreadHandle);
  m_pImpl->m_MainThreadHandle = nullptr;

  return XII_SUCCESS;
}

xiiResult xiiProcess::WaitToFinish(xiiTime timeout /*= xiiTime::Zero()*/)
{
  XII_ASSERT_DEV(m_pImpl->m_ProcessHandle != nullptr, "Launch a process before waiting on it");
  XII_ASSERT_DEV(m_pImpl->m_ProcessID != 0, "Launch a process before waiting on it");

  DWORD dwTimeout = INFINITE;

  if (timeout.IsPositive())
    dwTimeout = (DWORD)timeout.GetMilliseconds();
  else
    dwTimeout = INFINITE;

  const DWORD res = WaitForSingleObject(m_pImpl->m_ProcessHandle, dwTimeout);

  if (res == WAIT_TIMEOUT)
  {
    // The process is not yet finished, the timeout was reached
    return XII_FAILURE;
  }

  if (res == WAIT_FAILED)
  {
    xiiLog::Error("Failed to wait for '{}' - {}", m_sProcess, xiiArgErrorCode(GetLastError()));
    return XII_FAILURE;
  }

  // The process has finished

  m_pImpl->m_pipeStdOut.Close();
  m_pImpl->m_pipeStdErr.Close();

  GetExitCodeProcess(m_pImpl->m_ProcessHandle, reinterpret_cast<DWORD*>(&m_iExitCode));

  return XII_SUCCESS;
}

xiiResult xiiProcess::Execute(const xiiProcessOptions& opt, xiiInt32* out_pExitCode /*= nullptr*/)
{
  xiiProcess proc;

  XII_SUCCEED_OR_RETURN(proc.Launch(opt));
  XII_SUCCEED_OR_RETURN(proc.WaitToFinish());

  if (out_pExitCode != nullptr)
  {
    *out_pExitCode = proc.GetExitCode();
  }

  return XII_SUCCESS;
}

xiiResult xiiProcess::Terminate()
{
  XII_ASSERT_DEV(m_pImpl->m_ProcessHandle != nullptr, "Launch a process before terminating it");
  XII_ASSERT_DEV(m_pImpl->m_ProcessID != 0, "Launch a process before terminating it");

  if (TerminateProcess(m_pImpl->m_ProcessHandle, 0xFFFFFFFF) == FALSE)
  {
    xiiLog::Error("Failed to terminate process '{}' - {}", m_sProcess, xiiArgErrorCode(GetLastError()));
    return XII_FAILURE;
  }

  XII_SUCCEED_OR_RETURN(WaitToFinish());

  return XII_SUCCESS;
}

xiiProcessState xiiProcess::GetState() const
{
  if (m_pImpl->m_ProcessHandle == 0)
    return xiiProcessState::NotStarted;

  DWORD exitCode = 0;
  if (GetExitCodeProcess(m_pImpl->m_ProcessHandle, &exitCode) == FALSE)
  {
    xiiLog::Error("Failed to retrieve exit code for process '{}' - {}", m_sProcess, xiiArgErrorCode(GetLastError()));

    // Not sure what kind of errors can happen (probably access denied and such).
    // However, we have to return something, so lets claim the process is finished
    return xiiProcessState::Finished;
  }

  if (exitCode == STILL_ACTIVE)
    return xiiProcessState::Running;

  // Do not consider a process finished if the pipe threads have not exited yet.
  if (m_pImpl->m_pipeStdOut.IsRunning() || m_pImpl->m_pipeStdErr.IsRunning())
    return xiiProcessState::Running;

  m_iExitCode = (xiiInt32)exitCode;
  return xiiProcessState::Finished;
}

void xiiProcess::Detach()
{
  // Throw away the previous xiiProcessImpl and create a blank one
  m_pImpl = XII_DEFAULT_NEW(xiiProcessImpl);

  // Reset the exit code to the default
  m_iExitCode = -0xFFFF;
}
