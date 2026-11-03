/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/System/SystemInformation.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

XII_DEFINE_AS_POD_TYPE(struct pollfd);

class xiiFd
{
public:
  xiiFd()             = default;
  xiiFd(const xiiFd&) = delete;
  xiiFd(xiiFd&& other)
  {
    m_fd       = other.m_fd;
    other.m_fd = -1;
  }

  ~xiiFd()
  {
    Close();
  }

  void Close()
  {
    if (m_fd != -1)
    {
      close(m_fd);
      m_fd = -1;
    }
  }

  bool IsValid() const
  {
    return m_fd >= 0;
  }

  void operator=(const xiiFd&) = delete;
  void operator=(xiiFd&& other)
  {
    Close();
    m_fd       = other.m_fd;
    other.m_fd = -1;
  }

  void TakeOwnership(xiiInt32 fd)
  {
    Close();
    m_fd = fd;
  }

  xiiInt32 Borrow() const { return m_fd; }

  xiiInt32 Detach()
  {
    auto result = m_fd;
    m_fd        = -1;
    return result;
  }

  xiiResult AddFlags(xiiInt32 addFlags)
  {
    if (m_fd < 0)
      return XII_FAILURE;

    if (addFlags & O_CLOEXEC)
    {
      xiiInt32 flags = fcntl(m_fd, F_GETFD);
      flags |= FD_CLOEXEC;
      if (fcntl(m_fd, F_SETFD, flags) != 0)
      {
        xiiLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return XII_FAILURE;
      }
      addFlags &= ~O_CLOEXEC;
    }

    if (addFlags)
    {
      xiiInt32 flags = fcntl(m_fd, F_GETFL);
      flags |= addFlags;
      if (fcntl(m_fd, F_SETFD, flags) != 0)
      {
        xiiLog::Error("Failed to set flags on {}: {}", m_fd, errno);
        return XII_FAILURE;
      }
    }

    return XII_SUCCESS;
  }

  static xiiResult MakePipe(xiiFd (&fds)[2], xiiInt32 flags = 0)
  {
    fds[0].Close();
    fds[1].Close();
#if XII_ENABLED(XII_USE_LINUX_POSIX_EXTENSIONS)
    if (pipe2((xiiInt32*)fds, flags) != 0)
    {
      return XII_FAILURE;
    }
#else
    if (pipe((xiiInt32*)fds) != 0)
    {
      return XII_FAILURE;
    }
    if (flags != 0 && (fds[0].AddFlags(flags).Failed() || fds[1].AddFlags(flags).Failed()))
    {
      fds[0].Close();
      fds[1].Close();
      return XII_FAILURE;
    }
#endif
    return XII_SUCCESS;
  }

private:
  xiiInt32 m_fd = -1;
};

namespace
{
  struct ProcessStartupError
  {
    enum class Type : xiiUInt32
    {
      FailedToChangeWorkingDirectory = 0,
      FailedToExecv                  = 1
    };

    Type     type;
    xiiInt32 errorCode;
  };
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
    xiiFd                            fd;
    xiiDelegate<void(xiiStringView)> callback;
  };
  xiiHybridArray<StdStreamInfo, 2>  m_streams;
  xiiDynamicArray<xiiStringBuilder> m_overflowBuffers;
  xiiUniquePtr<xiiOSThread>         m_streamWatcherThread;
  xiiFd                             m_wakeupPipeReadEnd;
  xiiFd                             m_wakeupPipeWriteEnd;

  static void* StreamWatcherThread(void* context)
  {
    xiiProcessImpl* self = reinterpret_cast<xiiProcessImpl*>(context);
    char            buffer[4096];

    xiiHybridArray<struct pollfd, 3> pollfds;

    pollfds.PushBack({self->m_wakeupPipeReadEnd.Borrow(), POLLIN, 0});
    for (StdStreamInfo& stream : self->m_streams)
    {
      pollfds.PushBack({stream.fd.Borrow(), POLLIN, 0});
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
          if (pollfds[i].revents & POLLIN)
          {
            xiiStringBuilder& overflowBuffer = self->m_overflowBuffers[i - 1];
            StdStreamInfo&    stream         = self->m_streams[i - 1];
            while (true)
            {
              ssize_t numBytes = read(stream.fd.Borrow(), buffer, XII_ARRAY_SIZE(buffer));
              if (numBytes < 0)
              {
                if (errno == EWOULDBLOCK)
                {
                  break;
                }
                xiiLog::Error("Process Posix read error on {}: {}", stream.fd.Borrow(), errno);
                return nullptr;
              }

              const char* szCurrentPos = buffer;
              const char* szEndPos     = buffer + numBytes;
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
                  // This is either the start or a middle segment of a line, append to overflow buffer.
                  overflowBuffer.Append(xiiStringView(szCurrentPos, szEndPos));
                  szCurrentPos = szEndPos;
                }
              }

              if (numBytes < XII_ARRAY_SIZE(buffer))
              {
                break;
              }
            }
          }
          pollfds[i].revents = 0;
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

      self->m_streams[i].fd.Close();
    }

    return nullptr;
  }

  xiiResult StartStreamWatcher()
  {
    xiiFd wakeupPipe[2];
    if (xiiFd::MakePipe(wakeupPipe, O_NONBLOCK | O_CLOEXEC).Failed())
    {
      xiiLog::Error("Failed to setup wakeup pipe {}", errno);
      return XII_FAILURE;
    }
    else
    {
      m_wakeupPipeReadEnd  = std::move(wakeupPipe[0]);
      m_wakeupPipeWriteEnd = std::move(wakeupPipe[1]);
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
      XII_IGNORE_UNUSED(write(m_wakeupPipeWriteEnd.Borrow(), &c, 1));
      m_streamWatcherThread->Join();
      m_streamWatcherThread = nullptr;
    }
    m_wakeupPipeReadEnd.Close();
    m_wakeupPipeWriteEnd.Close();
  }

  void AddStream(xiiFd fd, const xiiDelegate<void(xiiStringView)>& callback)
  {
    m_streams.PushBack({std::move(fd), callback});
    m_overflowBuffers.SetCount(m_streams.GetCount());
  }

  xiiUInt32 GetNumStreams() const { return m_streams.GetCount(); }

  static xiiResult StartChildProcess(const xiiProcessOptions& opt, pid_t& outPid, bool suspended, xiiFd& outStdOutFd, xiiFd& outStdErrFd)
  {
    xiiFd stdoutPipe[2];
    xiiFd stderrPipe[2];
    xiiFd startupErrorPipe[2];

    xiiStringBuilder executablePath = opt.m_sProcess;
    xiiFileStats     stats;
    if (!opt.m_sProcess.IsAbsolutePath())
    {
      executablePath = xiiOSFile::GetCurrentWorkingDirectory();
      executablePath.AppendPath(opt.m_sProcess);
    }

    if (xiiOSFile::GetFileStats(executablePath, stats).Failed() || stats.m_bIsDirectory)
    {
      xiiHybridArray<char, 512> confPath;
      auto                      envPATH = getenv("PATH");
      if (envPATH == nullptr) // if no PATH environment variable is available, we need to fetch the system default;
      {
#if _POSIX_C_SOURCE >= 2 || _XOPEN_SOURCE
        size_t confPathSize = confstr(_CS_PATH, nullptr, 0);
        if (confPathSize > 0)
        {
          confPath.SetCountUninitialized(confPathSize);
          if (confstr(_CS_PATH, confPath.GetData(), confPath.GetCount()) == 0)
          {
            confPath.SetCountUninitialized(0);
          }
        }
#endif
        if (confPath.GetCount() == 0)
        {
          confPath.PushBack('\0');
        }
        envPATH = confPath.GetData();
      }

      xiiStringView                     path = envPATH;
      xiiHybridArray<xiiStringView, 16> pathParts;
      path.Split(false, pathParts, ":");

      for (auto& pathPart : pathParts)
      {
        executablePath = pathPart;
        executablePath.AppendPath(opt.m_sProcess);
        if (xiiOSFile::GetFileStats(executablePath, stats).Succeeded() && !stats.m_bIsDirectory)
        {
          break;
        }
        executablePath.Clear();
      }
    }

    if (executablePath.IsEmpty())
    {
      return XII_FAILURE;
    }

    if (opt.m_onStdOut.IsValid())
    {
      if (xiiFd::MakePipe(stdoutPipe).Failed())
      {
        return XII_FAILURE;
      }
      if (stdoutPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return XII_FAILURE;
      }
    }

    if (opt.m_onStdError.IsValid())
    {
      if (xiiFd::MakePipe(stderrPipe).Failed())
      {
        return XII_FAILURE;
      }
      if (stderrPipe[0].AddFlags(O_NONBLOCK).Failed())
      {
        return XII_FAILURE;
      }
    }

    if (xiiFd::MakePipe(startupErrorPipe, O_CLOEXEC).Failed())
    {
      return XII_FAILURE;
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
        stdoutPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stdoutPipe[1].Borrow(), STDOUT_FILENO); // redirect the write end to STDOUT
        stdoutPipe[1].Close();
      }

      if (opt.m_onStdError.IsValid())
      {
        stderrPipe[0].Close();                       // We don't need the read end of the pipe in the child process
        dup2(stderrPipe[1].Borrow(), STDERR_FILENO); // redirect the write end to STDERR
        stderrPipe[1].Close();
      }

      startupErrorPipe[0].Close(); // we don't need the read end of the startup error pipe in the child process

      xiiHybridArray<char*, 9> args;

      args.PushBack(const_cast<char*>(executablePath.GetData()));
      for (const xiiString& arg : opt.m_Arguments)
      {
        args.PushBack(const_cast<char*>(arg.GetData()));
      }
      args.PushBack(nullptr);

      if (!opt.m_sWorkingDirectory.IsEmpty())
      {
        if (chdir(opt.m_sWorkingDirectory.GetData()) < 0)
        {
          auto err = ProcessStartupError{ProcessStartupError::Type::FailedToChangeWorkingDirectory, 0};
          XII_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
          startupErrorPipe[1].Close();
          _exit(-1);
        }
      }

      if (execv(executablePath, args.GetData()) < 0)
      {
        auto err = ProcessStartupError{ProcessStartupError::Type::FailedToExecv, errno};
        XII_IGNORE_UNUSED(write(startupErrorPipe[1].Borrow(), &err, sizeof(err)));
        startupErrorPipe[1].Close();
        _exit(-1);
      }
    }
    else
    {
      startupErrorPipe[1].Close(); // We don't need the write end of the startup error pipe in the parent process
      stdoutPipe[1].Close();       // Don't need the write end in the parent process
      stderrPipe[1].Close();       // Don't need the write end in the parent process

      ProcessStartupError err     = {};
      auto                errSize = read(startupErrorPipe[0].Borrow(), &err, sizeof(err));
      startupErrorPipe[0].Close(); // we no longer need the read end of the startup error pipe

      // There are two possible cases here
      // Case 1: errSize is equal to 0, which means no error happened on the startupErrorPipe was closed during the execv call
      // Case 2: errSize > 0 in which case there was an error before the pipe was closed normally.
      if (errSize > 0)
      {
        XII_ASSERT_DEV(errSize == sizeof(err), "Child process should have written a full ProcessStartupError struct");
        switch (err.type)
        {
          case ProcessStartupError::Type::FailedToChangeWorkingDirectory:
            xiiLog::Error("Failed to start process '{}' because the given working directory '{}' is invalid", opt.m_sProcess, opt.m_sWorkingDirectory);
            break;
          case ProcessStartupError::Type::FailedToExecv:
            xiiLog::Error("Failed to exec when starting process '{}' the error code is '{}'", opt.m_sProcess, err.errorCode);
            break;
        }
        return XII_FAILURE;
      }

      outPid = childPid;

      if (opt.m_onStdOut.IsValid())
      {
        outStdOutFd = std::move(stdoutPipe[0]);
      }

      if (opt.m_onStdError.IsValid())
      {
        outStdErrFd = std::move(stderrPipe[0]);
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
  pid_t childPid = 0;
  xiiFd stdoutFd;
  xiiFd stderrFd;
  if (xiiProcessImpl::StartChildProcess(opt, childPid, false, stdoutFd, stderrFd).Failed())
  {
    return XII_FAILURE;
  }

  xiiProcessImpl impl;
  if (stdoutFd.IsValid())
  {
    impl.AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    impl.AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (impl.GetNumStreams() > 0 && impl.StartStreamWatcher().Failed())
  {
    return XII_FAILURE;
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
    else if (WIFSIGNALED(childStatus))
    {
      *out_iExitCode = WTERMSIG(childStatus);
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

  xiiFd stdoutFd;
  xiiFd stderrFd;

  if (xiiProcessImpl::StartChildProcess(opt, m_pImpl->m_childPid, launchFlags.IsSet(xiiProcessLaunchFlags::Suspended), stdoutFd, stderrFd).Failed())
  {
    return XII_FAILURE;
  }

  m_pImpl->m_exitCodeAvailable = false;
  m_pImpl->m_processSuspended  = launchFlags.IsSet(xiiProcessLaunchFlags::Suspended);

  if (stdoutFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stdoutFd), opt.m_onStdOut);
  }

  if (stderrFd.IsValid())
  {
    m_pImpl->AddStream(std::move(stderrFd), opt.m_onStdError);
  }

  if (m_pImpl->GetNumStreams() > 0)
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

xiiResult xiiProcess::WaitToFinish(xiiTime timeout /*= xiiTime::MakeZero()*/)
{
  if (m_pImpl->m_exitCodeAvailable)
  {
    return XII_SUCCESS;
  }

  if (timeout.IsZero())
  {
    xiiInt32 childStatus = 0;
    xiiInt32 waitResult  = waitpid(m_pImpl->m_childPid, &childStatus, 0);
    if (waitResult > 0)
    {
      m_iExitCode                  = WEXITSTATUS(childStatus);
      m_pImpl->m_exitCodeAvailable = true;

      m_pImpl->StopStreamWatcher();

      return XII_SUCCESS;
    }
    return XII_FAILURE;
  }
  else
  {
    xiiTime startWait = xiiTime::Now();
    while (true)
    {
      const xiiProcessState state = GetState();
      switch (state)
      {
        case xiiProcessState::NotStarted:
          return XII_FAILURE;
        case xiiProcessState::Running:
          break;
        case xiiProcessState::Finished:
          return XII_SUCCESS;
      }

      xiiTime timeSpent = xiiTime::Now() - startWait;
      if (timeSpent > timeout)
      {
        return XII_FAILURE;
      }
      xiiThreadUtils::Sleep(xiiMath::Min(xiiTime::MakeFromMilliseconds(100.0), timeout - timeSpent));
    }
  }
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
