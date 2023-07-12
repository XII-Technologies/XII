#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/StackTracer.h>

#include <csignal>
#include <cxxabi.h>
#include <unistd.h>

static void xiiCrashHandlerFunc() noexcept
{
  if (xiiCrashHandler::GetCrashHandler() != nullptr)
  {
    xiiCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // Restore the original signal handler for the abort signal and raise one so the kernel can do a core dump.
  std::signal(SIGABRT, SIG_DFL);
  std::raise(SIGABRT);
}

static void xiiSignalHandler(int signum)
{
  xiiLog::Printf("***Unhandled Signal:***\n");
  switch (signum)
  {
    case SIGINT:
      xiiLog::Printf("Signal SIGINT: Interrupt\n");
      break;
    case SIGILL:
      xiiLog::Printf("Signal SIGILL: Illegal Instruction - Invalid Function Image\n");
      break;
    case SIGFPE:
      xiiLog::Printf("Signal SIGFPE: Floating Point WException\n");
      break;
    case SIGSEGV:
      xiiLog::Printf("Signal SIGSEGV: Segment Violation\n");
      break;
    case SIGTERM:
      xiiLog::Printf("Signal SIGTERM: Software Termination Signal From Kill\n");
      break;
    case SIGABRT:
      xiiLog::Printf("Signal SIGABRT: Abnormal Termination Triggered By Abort Call\n");
      break;
    default:
      xiiLog::Printf("Signal %i: Unknown Signal\n", signal);
      break;
  }

  if (xiiCrashHandler::GetCrashHandler() != nullptr)
  {
    xiiCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // forward the signal back to the OS so that it can write a core dump
  std::signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void xiiCrashHandler::SetCrashHandler(xiiCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    std::signal(SIGINT, xiiSignalHandler);
    std::signal(SIGILL, xiiSignalHandler);
    std::signal(SIGFPE, xiiSignalHandler);
    std::signal(SIGSEGV, xiiSignalHandler);
    std::signal(SIGTERM, xiiSignalHandler);
    std::signal(SIGABRT, xiiSignalHandler);
    std::set_terminate(xiiCrashHandlerFunc);
  }
  else
  {
    std::signal(SIGINT, nullptr);
    std::signal(SIGILL, nullptr);
    std::signal(SIGFPE, nullptr);
    std::signal(SIGSEGV, nullptr);
    std::signal(SIGTERM, nullptr);
    std::signal(SIGABRT, nullptr);
    std::set_terminate(nullptr);
  }
}

bool xiiCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
  return false;
}

void xiiCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  xiiLog::Printf("***Unhandled Exception:***\n");

  // xiiLog::Printf exception type
  if (std::type_info* type = abi::__cxa_current_exception_type())
  {
    if (const char* szName = type->name())
    {
      xiiInt32 status = -1;
      // Try to print nice name
      if (char* szNiceName = abi::__cxa_demangle(szName, 0, 0, &status))
        xiiLog::Printf("Exception: %s\n", szNiceName);
      else
        xiiLog::Printf("Exception: %s\n", szName);
    }
  }

  {
    xiiLog::Printf("\n\n***Stack Trace:***\n");

    void*              pBuffer[64];
    xiiArrayPtr<void*> tempTrace(pBuffer);
    const xiiUInt32    uiNumTraces = xiiStackTracer::GetStackTrace(tempTrace);

    xiiStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}
