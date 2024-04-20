#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

thread_local xiiTaskWorkerInfo tl_TaskWorkerInfo;

static const char* GenerateThreadName(xiiWorkerThreadType::Enum threadType, xiiUInt32 uiThreadNumber)
{
  static xiiStringBuilder sTemp;
  sTemp.SetFormat("{} {}", xiiWorkerThreadType::GetThreadTypeName(threadType), uiThreadNumber);
  return sTemp.GetData();
}

xiiTaskWorkerThread::xiiTaskWorkerThread(xiiWorkerThreadType::Enum threadType, xiiUInt32 uiThreadNumber)
  // We need at least 256 kb of stack size, otherwise the shader compilation tasks will run out of stack space.
  :
  xiiThread(GenerateThreadName(threadType, uiThreadNumber), 256 * 1024)
{
  m_WorkerType           = threadType;
  m_uiWorkerThreadNumber = uiThreadNumber & 0xFFFF;
}

xiiTaskWorkerThread::~xiiTaskWorkerThread() = default;

xiiResult xiiTaskWorkerThread::DeactivateWorker()
{
  m_bActive = false;

  if (GetThreadStatus() != xiiThread::Finished)
  {
    // if necessary, wake this thread up
    WakeUpIfIdle();

    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiUInt32 xiiTaskWorkerThread::Run()
{
  XII_ASSERT_DEBUG(m_WorkerType != xiiWorkerThreadType::Unknown && m_WorkerType != xiiWorkerThreadType::MainThread, "Worker threads cannot use this type");
  XII_ASSERT_DEBUG(m_WorkerType < xiiWorkerThreadType::ENUM_COUNT, "Worker Thread Type is invalid: {0}", m_WorkerType);

  // Once this thread is running, store the worker type in the thread_local variable
  // Such that the xiiTaskSystem is able to look this up (e.g. in WaitForGroup) to know which types of tasks to help with
  tl_TaskWorkerInfo.m_WorkerType   = m_WorkerType;
  tl_TaskWorkerInfo.m_iWorkerIndex = m_uiWorkerThreadNumber;
  tl_TaskWorkerInfo.m_pWorkerState = &m_iWorkerState;

  const bool bIsReserve = m_uiWorkerThreadNumber >= xiiTaskSystem::s_pThreadState->m_uiMaxWorkersToUse[m_WorkerType];

  xiiTaskPriority::Enum FirstPriority;
  xiiTaskPriority::Enum LastPriority;
  xiiTaskSystem::DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  m_bExecutingTask = false;

  while (m_bActive)
  {
    if (!m_bExecutingTask)
    {
      m_bExecutingTask     = true;
      m_StartedWorkingTime = xiiTime::Now();
    }

    if (!xiiTaskSystem::ExecuteTask(FirstPriority, LastPriority, false, xiiTaskGroupID(), &m_iWorkerState))
    {
      WaitForWork();
    }
    else
    {
      ++m_uiNumTasksExecuted;

      if (bIsReserve)
      {
        XII_VERIFY(m_iWorkerState.Set((xiiInt32)xiiTaskWorkerState::Idle) == (xiiInt32)xiiTaskWorkerState::Active, "Corrupt worker state");

        // if this thread is part of the reserve, then don't continue to process tasks indefinitely
        // instead, put this thread to sleep and wake up someone else
        // that someone else may be a thread at the front of the queue, it may also turn out to be this thread again
        // either way, if at some point we woke up more threads than the maximum desired, this will move the active threads
        // to the front of the list, because of the way xiiTaskSystem::WakeUpThreads() works
        xiiTaskSystem::WakeUpThreads(m_WorkerType, 1);

        WaitForWork();
      }
    }
  }

  return 0;
}

void xiiTaskWorkerThread::WaitForWork()
{
  // m_bIsIdle usually will be true here, but may also already have been reset to false
  // in that case m_WakeUpSignal will be raised already and the code below will just run through and continue

  m_ThreadActiveTime += xiiTime::Now() - m_StartedWorkingTime;
  m_bExecutingTask = false;
  m_WakeUpSignal.WaitForSignal();
  XII_ASSERT_DEBUG(m_iWorkerState == (xiiInt32)xiiTaskWorkerState::Active, "Worker state should have been reset to 'active'");
}

xiiTaskWorkerState xiiTaskWorkerThread::WakeUpIfIdle()
{
  xiiTaskWorkerState prev = (xiiTaskWorkerState)m_iWorkerState.CompareAndSwap((xiiInt32)xiiTaskWorkerState::Idle, (xiiInt32)xiiTaskWorkerState::Active);
  if (prev == xiiTaskWorkerState::Idle) // was idle before
  {
    m_WakeUpSignal.RaiseSignal();
  }

  return static_cast<xiiTaskWorkerState>(prev);
}

void xiiTaskWorkerThread::UpdateThreadUtilization(xiiTime timePassed)
{
  xiiTime tActive = m_ThreadActiveTime;

  // The thread keeps track of how much time it spends executing tasks.
  // Here we retrieve that time and resets it to zero.
  {
    m_ThreadActiveTime = xiiTime::Zero();

    if (m_bExecutingTask)
    {
      const xiiTime tNow = xiiTime::Now();
      tActive += tNow - m_StartedWorkingTime;
      m_StartedWorkingTime = tNow;
    }
  }

  m_fLastThreadUtilization = tActive.GetSeconds() / timePassed.GetSeconds();
  m_uiLastNumTasksExecuted = m_uiNumTasksExecuted;
  m_uiNumTasksExecuted     = 0;
}

double xiiTaskWorkerThread::GetThreadUtilization(xiiUInt32* pNumTasksExecuted /*= nullptr*/)
{
  if (pNumTasksExecuted)
  {
    *pNumTasksExecuted = m_uiLastNumTasksExecuted;
  }

  return m_fLastThreadUtilization;
}


XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskWorkerThread);
