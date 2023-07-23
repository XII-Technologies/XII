#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

XII_DEFINE_AS_POD_TYPE(struct pollfd);

namespace
{
  xiiResult AddFdFlags(xiiInt32 fd, xiiInt32 addFlags)
  {
    xiiInt32 flags = fcntl(fd, F_GETFD);
    flags |= addFlags;
    if (fcntl(fd, F_SETFD, flags) != 0)
    {
      xiiLog::Error("Failed to set flags on {}: {}", fd, errno);
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }
} // namespace

struct xiiProcessImpl
{
  ~xiiProcessImpl()
  {
    StopStreamWatcher();
  }

  pid_t m_childPid          = -1;
  bool  m_exitCodeAvailable = false;
  bool  m_processSuspended  = false;

  struct StdStreamInfo
  {
    xiiInt32                         fd;
    xiiDelegate<void(xiiStringView)> callback;
  };
  xiiHybridArray<StdStreamInfo, 2>  m_streams;
  xiiDynamicArray<xiiStringBuilder> m_overflowBuffers;
  xiiUniquePtr<xiiOSThread>         m_streamWatcherThread;
  xiiInt32                          m_wakeupPipeReadEnd  = -1;
  xiiInt32                          m_wakeupPipeWriteEnd = -1;

  static void* StreamWatcherThread(void* context)
  {
    xiiProcessImpl* self = reinterpret_cast<xiiProcessImpl*>(context);
    char            szBuffer[4096];

    xiiHybridArray<struct pollfd, 3> pollfds;

    pollfds.PushBack({self->m_wakeupPipeReadEnd, POLLIN, 0});
    for (StdStreamInfo& stream : self->m_streams)
    {
      pollfds.PushBack({stream.fd, POLLIN, 0});
    }

    bool run = true;
    while (run)
    {
      xiiInt32 result = poll(pollfds.GetData(), pollfds.GetCount(), -1);
      if (result > 0)
      {
        // Result at index 0 is special and means there was a WakeUp
        if (pollfds[0].revents != 0)
        {
          run = false;
        }

        for (xiiUInt32 i = 1; i < pollfds.GetCount(); ++i)
        {
          if (pollfds[i].revents != 0)
          {
            xiiStringBuilder& overflowBuffer = self->m_overflowBuffers[i - 1];
            StdStreamInfo&    stream         = self->m_streams[i - 1];
            pollfds[i].revents               = 0;
            while (true)
            {
              ssize_t numBytes = read(stream.fd, szBuffer, XII_ARRAY_SIZE(szBuffer));
              if (numBytes < 0)
              {
                if (errno == EWOULDBLOCK)
                {
                  break;
                }
                xiiLog::Error("Process Posix read error on {}: {}", stream.fd, errno);
                return nullptr;
              }
              if (numBytes == 0)
              {
                break;
              }

              const char* szCurrentPos = szBuffer;
              const char* szEndPos     = szBuffer + numBytes;
              while (szCurrentPos < szEndPos)
              {
                const char* szFound = xiiStringUtils::FindSubString(szCurrentPos, "\n", szEndPos);
                if (szFound)
                {
                  if (overflowBuffer.IsEmpty())
                  {
                    // If there is nothing in the overflow buffer, this is a complete line and can be fired as is.
                    stream.callback(xiiStringView(szCurrentPos, szFound + 1));
                  }
                  else
                  {
                    // We have data in the overflow buffer so this is the final part of a partial line so we need to complete and fire the overflow buffer.
                    overflowBuffer.Append(xiiStringView(szCurrentPos, szFound + 1));
                    stream.callback(overflowBuffer);
                    overflowBuffer.Clear();
                  }
                  szCurrentPos = szFound + 1;
                }
                else
                {
                  // This is either the start or a middle segment of a line, append to overflow szBuffer.
                  overflowBuffer.Append(xiiStringView(szCurrentPos, szEndPos));
                  szCurrentPos = szEndPos;
                }
              }
            }
          }
        }
      }
      else if (result < 0)
      {
        xiiLog::Error("poll error {}", errno);
        break;
      }
    }

    for (xiiUInt32 i = 0; i < self->m_streams.GetCount(); ++i)
    {
      xiiStringBuilder& overflowBuffer = self->m_overflowBuffers[i];
      if (!overflowBuffer.IsEmpty())
      {
        self->m_streams[i].callback(overflowBuffer);
        overflowBuffer.Clear();
      }
    }

    return nullptr;
  }

  xiiResult StartStreamWatcher()
  {
    xiiInt32 wakeupPipe[2] = {-1, -1};
    if (pipe(wakeupPipe) < 0)
    {
      xiiLog::Error("Failed to setup wakeup pipe {}", errno);
      return XII_FAILURE;
    }
    else
    {
      m_wakeupPipeReadEnd  = wakeupPipe[0];
      m_wakeupPipeWriteEnd = wakeupPipe[1];
      if (AddFdFlags(m_wakeupPipeReadEnd, O_NONBLOCK | O_CLOEXEC).Failed() ||
          AddFdFlags(m_wakeupPipeWriteEnd, O_NONBLOCK | O_CLOEXEC).Failed())
      {
        close(m_wakeupPipeReadEnd);
        m_wakeupPipeReadEnd = -1;
        close(m_wakeupPipeWriteEnd);
        m_wakeupPipeWriteEnd = -1;
        return XII_FAILURE;
      }
    }

    m_streamWatcherThread = XII_DEFAULT_NEW(xiiOSThread, &StreamWatcherThread, this, "StdStrmWtch");
    m_streamWatcherThread->Start();

    return XII_SUCCESS;
  }

  void StopStreamWatcher()
  {
    if (m_streamWatcherThread)
    {
      char c = 0;
      XII_IGNORE_UNUSED(write(m_wakeupPipeWriteEnd, &c, 1));
      m_streamWatcherThread->Join();
      m_streamWatcherThread = nullptr;
    }
    close(m_wakeupPipeReadEnd);
    close(m_wakeupPipeWriteEnd);
    m_wakeupPipeReadEnd  = -1;
    m_wakeupPipeWriteEnd = -1;
  }

  void AddStream(xiiInt32 fd, const xiiDelegate<void(xiiStringView)>& callback)
  {
    m_streams.PushBack({fd, callback});
    m_overflowBuffers.SetCount(m_streams.GetCount());
  }

  static xiiResult StartChildProcess(const xiiProcessOptions& opt, pid_t& outPid, bool suspended, xiiInt32& outStdOutFd, xiiInt32& outStdErrFd)
  {
    xiiInt32 stdoutPipe[2] = {-1, -1};
    xiiInt32 stderrPipe[2] = {-1, -1};

    if (opt.m_onStdOut.IsValid())
    {
      if (pipe(stdoutPipe) < 0)
      {
        return XII_FAILURE;
      }
    }

    if (opt.m_onStdError.IsValid())
    {
      if (pipe(stderrPipe) < 0)
      {
        return XII_FAILURE;
      }
    }

    pid_t childPid = fork();
    if (childPid < 0)
    {
      return XII_FAILURE;
    }

    if (childPid == 0) // We are the child
    {
      if (suspended)
      {
        if (raise(SIGSTOP) < 0)
        {
          _exit(-1);
        }
      }

      if (opt.m_bHideConsoleWindow == true)
      {
        // Redirect STDIN to /dev/null
        xiiInt32 stdinReplace = open("/dev/null", O_RDONLY);
        dup2(stdinReplace, STDIN_FILENO);
        close(stdinReplace);

        if (!opt.m_onStdOut.IsValid())
        {
          xiiInt32 stdoutReplace = open("/dev/null", O_WRONLY);
          dup2(stdoutReplace, STDOUT_FILENO);
          close(stdoutReplace);
        }

        if (!opt.m_onStdError.IsValid())
        {
          xiiInt32 stderrReplace = open("/dev/null", O_WRONLY);
          dup2(stderrReplace, STDERR_FILENO);
          close(stderrReplace);
        }
      }
      else
      {
        // TODO: Launch a x-terminal-emulator with the command and somehow redirect STDOUT, etc?
        XII_ASSERT_NOT_IMPLEMENTED;
      }

      if (opt.m_onStdOut.IsValid())
      {
        close(stdoutPipe[0]);               // We don't need the read end of the pipe in the child process
        dup2(stdoutPipe[1], STDOUT_FILENO); // redirect the write end to STDOUT
        close(stdoutPipe[1]);
      }

      if (opt.m_onStdError.IsValid())
      {
        close(stderrPipe[0]);               // We don't need the read end of the pipe in the child process
        dup2(stderrPipe[1], STDERR_FILENO); // redirect the write end to STDERR
        close(stderrPipe[1]);
      }

      xiiHybridArray<char*, 9> args;

      for (const xiiString& arg : opt.m_Arguments)
      {
        args.PushBack(const_cast<char*>(arg.GetData()));
      }
      args.PushBack(nullptr);

      if (!opt.m_sWorkingDirectory.IsEmpty())
      {
        if (chdir(opt.m_sWorkingDirectory.GetData()) < 0)
        {
          _exit(-1); // Failed to change working directory
        }
      }

      if (execv(opt.m_sProcess.GetData(), args.GetData()) < 0)
      {
        _exit(-1);
      }
    }
    else
    {
      outPid = childPid;

      if (opt.m_onStdOut.IsValid())
      {
        close(stdoutPipe[1]); // Don't need the write end in the parent process
        outStdOutFd = stdoutPipe[0];
      }

      if (opt.m_onStdError.IsValid())
      {
        close(stderrPipe[1]); // Don't need the write end in the parent process
        outStdErrFd = stderrPipe[0];
      }
    }

    return XII_SUCCESS;
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

xiiResult xiiProcess::Execute(const xiiProcessOptions& opt, xiiInt32* out_iExitCode /*= nullptr*/)
{
  pid_t    childPid = 0;
  xiiInt32 stdoutFd = -1;
  xiiInt32 stderrFd = -1;
  if (xiiProcessImpl::StartChildProcess(opt, childPid, false, stdoutFd, stderrFd).Failed())
  {
    return XII_FAILURE;
  }

  xiiProcessImpl impl;
  if (stdoutFd >= 0)
  {
    impl.AddStream(stdoutFd, opt.m_onStdOut);
  }

  if (stderrFd >= 0)
  {
    impl.AddStream(stderrFd, opt.m_onStdError);
  }

  if (stdoutFd >= 0 || stderrFd >= 0)
  {
    if (impl.StartStreamWatcher().Failed())
    {
      return XII_FAILURE;
    }
  }

  xiiInt32 childStatus = -1;
  pid_t    waitedPid   = waitpid(childPid, &childStatus, 0);
  if (waitedPid < 0)
  {
    return XII_FAILURE;
  }
  if (out_iExitCode != nullptr)
  {
    if (WIFEXITED(childStatus))
    {
      *out_iExitCode = WEXITSTATUS(childStatus);
    }
    else
    {
      *out_iExitCode = -1;
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiProcess::Launch(const xiiProcessOptions& opt, xiiBitflags<xiiProcessLaunchFlags> launchFlags /*= xiiProcessLaunchFlags::None*/)
{
  XII_ASSERT_DEV(m_pImpl->m_childPid == -1, "Can not reuse an instance of xiiProcess");

  xiiInt32 stdoutFd = -1;
  xiiInt32 stderrFd = -1;

  if (xiiProcessImpl::StartChildProcess(opt, m_pImpl->m_childPid, launchFlags.IsSet(xiiProcessLaunchFlags::Suspended), stdoutFd, stderrFd).Failed())
  {
    return XII_FAILURE;
  }

  m_pImpl->m_exitCodeAvailable = false;
  m_pImpl->m_processSuspended  = launchFlags.IsSet(xiiProcessLaunchFlags::Suspended);

  if (stdoutFd >= 0)
  {
    m_pImpl->AddStream(stdoutFd, opt.m_onStdOut);
  }

  if (stderrFd >= 0)
  {
    m_pImpl->AddStream(stderrFd, opt.m_onStdError);
  }

  if (stdoutFd >= 0 || stderrFd >= 0)
  {
    if (m_pImpl->StartStreamWatcher().Failed())
    {
      return XII_FAILURE;
    }
  }

  if (launchFlags.IsSet(xiiProcessLaunchFlags::Detached))
  {
    Detach();
  }

  return XII_SUCCESS;
}

xiiResult xiiProcess::ResumeSuspended()
{
  if (m_pImpl->m_childPid < 0 || !m_pImpl->m_processSuspended)
  {
    return XII_FAILURE;
  }

  if (kill(m_pImpl->m_childPid, SIGCONT) < 0)
  {
    return XII_FAILURE;
  }
  m_pImpl->m_processSuspended = false;
  return XII_SUCCESS;
}

xiiResult xiiProcess::WaitToFinish(xiiTime timeout /*= xiiTime::Zero()*/)
{
  xiiInt32 childStatus = 0;
  XII_SCOPE_EXIT(m_pImpl->StopStreamWatcher());

  if (timeout.IsZero())
  {
    if (waitpid(m_pImpl->m_childPid, &childStatus, 0) < 0)
    {
      return XII_FAILURE;
    }
  }
  else
  {
    xiiInt32 waitResult = 0;
    xiiTime  startWait  = xiiTime::Now();
    while (true)
    {
      waitResult = waitpid(m_pImpl->m_childPid, &childStatus, WNOHANG);
      if (waitResult < 0)
      {
        return XII_FAILURE;
      }
      if (waitResult > 0)
      {
        break;
      }
      xiiTime timeSpent = xiiTime::Now() - startWait;
      if (timeSpent > timeout)
      {
        return XII_FAILURE;
      }
      xiiThreadUtils::Sleep(xiiMath::Min(xiiTime::Milliseconds(100.0), timeout - timeSpent));
    }
  }

  if (WIFEXITED(childStatus))
  {
    m_iExitCode = WEXITSTATUS(childStatus);
  }
  else
  {
    m_iExitCode = -1;
  }
  m_pImpl->m_exitCodeAvailable = true;

  return XII_SUCCESS;
}

xiiResult xiiProcess::Terminate()
{
  if (m_pImpl->m_childPid == -1)
  {
    return XII_FAILURE;
  }

  XII_SCOPE_EXIT(m_pImpl->StopStreamWatcher());

  if (kill(m_pImpl->m_childPid, SIGKILL) < 0)
  {
    if (errno != ESRCH) // ESRCH = Process does not exist
    {
      return XII_FAILURE;
    }
  }
  m_pImpl->m_exitCodeAvailable = true;
  m_iExitCode                  = -1;

  return XII_SUCCESS;
}

xiiProcessState xiiProcess::GetState() const
{
  if (m_pImpl->m_childPid == -1)
  {
    return xiiProcessState::NotStarted;
  }

  if (m_pImpl->m_exitCodeAvailable)
  {
    return xiiProcessState::Finished;
  }

  xiiInt32 childStatus = -1;
  xiiInt32 waitResult  = waitpid(m_pImpl->m_childPid, &childStatus, WNOHANG);
  if (waitResult > 0)
  {
    m_iExitCode                  = WEXITSTATUS(childStatus);
    m_pImpl->m_exitCodeAvailable = true;

    m_pImpl->StopStreamWatcher();

    return xiiProcessState::Finished;
  }

  return xiiProcessState::Running;
}

void xiiProcess::Detach()
{
  m_pImpl->m_childPid = -1;
}

xiiOsProcessHandle xiiProcess::GetProcessHandle() const
{
  XII_ASSERT_DEV(false, "There is no process handle on posix");
  return nullptr;
}

xiiOsProcessID xiiProcess::GetProcessID() const
{
  XII_ASSERT_DEV(m_pImpl->m_childPid != -1, "No ProcessID available");
  return m_pImpl->m_childPid;
}

xiiOsProcessID xiiProcess::GetCurrentProcessID()
{
  return getpid();
}
