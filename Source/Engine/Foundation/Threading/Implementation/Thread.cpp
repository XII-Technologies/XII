#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Thread.h>

xiiEvent<const xiiThreadEvent&, xiiMutex> xiiThread::s_ThreadEvents;

xiiThread::xiiThread(xiiStringView sName /*= "xiiThread"*/, xiiUInt32 uiStackSize /*= 128 * 1024*/) :
  xiiOSThread(xiiThreadClassEntryPoint, this, sName, uiStackSize), m_sName(sName)
{
  xiiThreadEvent e;
  e.m_pThread = this;
  e.m_Type    = xiiThreadEvent::Type::ThreadCreated;
  xiiThread::s_ThreadEvents.Broadcast(e, 255);
}

xiiThread::~xiiThread()
{
  XII_ASSERT_DEV(!IsRunning(), "Thread deletion while still running detected!");

  xiiThreadEvent e;
  e.m_pThread = this;
  e.m_Type    = xiiThreadEvent::Type::ThreadDestroyed;
  xiiThread::s_ThreadEvents.Broadcast(e, 255);
}

// Deactivate Doxygen document generation for the following block.
/// \cond

xiiUInt32 RunThread(xiiThread* pThread)
{
  if (pThread == nullptr)
    return 0;

  xiiProfilingSystem::SetThreadName(pThread->m_sName.GetData());

  {
    xiiThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type    = xiiThreadEvent::Type::StartingExecution;
    xiiThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = xiiThread::Running;

  // Run the worker thread function
  xiiUInt32 uiReturnCode = pThread->Run();

  {
    xiiThreadEvent e;
    e.m_pThread = pThread;
    e.m_Type    = xiiThreadEvent::Type::FinishedExecution;
    xiiThread::s_ThreadEvents.Broadcast(e, 255);
  }

  pThread->m_ThreadStatus = xiiThread::Finished;

  xiiProfilingSystem::RemoveThread();

  return uiReturnCode;
}

/// \endcond

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/Thread_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/Thread_posix.h>
#else
#  error "Runnable thread entry functions are not implemented on current platform"
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Thread);
