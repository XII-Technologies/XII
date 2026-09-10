/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

xiiUInt32 xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::Enum type)
{
  return s_pThreadState->m_uiMaxWorkersToUse[type];
}

xiiUInt32 xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::Enum type)
{
  return s_pThreadState->m_iAllocatedWorkers[type];
}

void xiiTaskSystem::SetWorkerThreadCount(xiiInt32 iShortTasks, xiiInt32 iLongTasks)
{
  xiiSystemInformation info = xiiSystemInformation::Get();

  // these settings are supposed to be a sensible default for most applications
  // an app can of course change that to optimize for its own usage
  //
  const xiiInt32 iCpuCores = info.GetCPUCoreCount();

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iShortTasks <= 0)
    iShortTasks = xiiMath::Clamp<xiiInt32>(iCpuCores - 2, 2, 8);

  // at least 2 threads, 4 on six cores, 6 on eight cores and up
  if (iLongTasks <= 0)
    iLongTasks = xiiMath::Clamp<xiiInt32>(iCpuCores - 2, 2, 8);

  // plus there is always one additional 'file access' thread
  // and the main thread, of course

  xiiUInt32 uiShortTasks = static_cast<xiiUInt32>(xiiMath::Max<xiiInt32>(iShortTasks, 1));
  xiiUInt32 uiLongTasks  = static_cast<xiiUInt32>(xiiMath::Max<xiiInt32>(iLongTasks, 1));

  // if nothing has changed, do nothing
  if (s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::ShortTasks] == uiShortTasks && s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::LongTasks] == uiLongTasks)
    return;

  xiiLog::Dev("CPU core count: {}", iCpuCores);
  xiiLog::Dev("Setting worker thread count to {} (short) / {} (long).", uiShortTasks, uiLongTasks);

  StopWorkerThreads();

  // this only allocates pointers, i.e. the maximum possible number of threads that we may be able to realloc at runtime
  s_pThreadState->m_Workers[xiiWorkerThreadType::ShortTasks].SetCount(1024);
  s_pThreadState->m_Workers[xiiWorkerThreadType::LongTasks].SetCount(1024);
  s_pThreadState->m_Workers[xiiWorkerThreadType::FileAccess].SetCount(128);

  s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::ShortTasks] = uiShortTasks;
  s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::LongTasks]  = uiLongTasks;
  s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::FileAccess] = 1;

  AllocateThreads(xiiWorkerThreadType::ShortTasks, s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::ShortTasks]);
  AllocateThreads(xiiWorkerThreadType::LongTasks, s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::LongTasks]);
  AllocateThreads(xiiWorkerThreadType::FileAccess, s_pThreadState->m_uiMaxWorkersToUse[xiiWorkerThreadType::FileAccess]);
}

void xiiTaskSystem::StopWorkerThreads()
{
  bool bWorkersStillRunning = true;

  // as long as any worker thread is still active, send the wake up signal
  while (bWorkersStillRunning)
  {
    bWorkersStillRunning = false;

    for (xiiUInt32 type = 0; type < xiiWorkerThreadType::ENUM_COUNT; ++type)
    {
      const xiiUInt32 uiNumThreads = s_pThreadState->m_iAllocatedWorkers[type];

      for (xiiUInt32 i = 0; i < uiNumThreads; ++i)
      {
        if (s_pThreadState->m_Workers[type][i]->DeactivateWorker().Failed())
        {
          bWorkersStillRunning = true;
        }
      }
    }

    // waste some time
    xiiThreadUtils::YieldTimeSlice();
  }

  for (xiiUInt32 type = 0; type < xiiWorkerThreadType::ENUM_COUNT; ++type)
  {
    const xiiUInt32 uiNumWorkers = s_pThreadState->m_iAllocatedWorkers[type];

    for (xiiUInt32 i = 0; i < uiNumWorkers; ++i)
    {
      s_pThreadState->m_Workers[type][i]->Join();
      XII_DEFAULT_DELETE(s_pThreadState->m_Workers[type][i]);
    }

    s_pThreadState->m_iAllocatedWorkers[type] = 0;
    s_pThreadState->m_uiMaxWorkersToUse[type] = 0;
    s_pThreadState->m_Workers[type].Clear();
  }
}

void xiiTaskSystem::AllocateThreads(xiiWorkerThreadType::Enum type, xiiUInt32 uiAddThreads)
{
  XII_ASSERT_DEBUG(uiAddThreads > 0, "Invalid number of threads to allocate");

  {
    // prevent concurrent thread allocation
    XII_LOCK(s_TaskSystemMutex);

    xiiUInt32 uiNextThreadIdx = s_pThreadState->m_iAllocatedWorkers[type];

    XII_ASSERT_ALWAYS(uiNextThreadIdx + uiAddThreads <= s_pThreadState->m_Workers[type].GetCount(), "Max number of worker threads ({}) exceeded.", s_pThreadState->m_Workers[type].GetCount());

    for (xiiUInt32 i = 0; i < uiAddThreads; ++i)
    {
      s_pThreadState->m_Workers[type][uiNextThreadIdx] = XII_DEFAULT_NEW(xiiTaskWorkerThread, (xiiWorkerThreadType::Enum)type, uiNextThreadIdx);
      s_pThreadState->m_Workers[type][uiNextThreadIdx]->Start();

      ++uiNextThreadIdx;
    }

    // let others access the new threads now
    s_pThreadState->m_iAllocatedWorkers[type] = uiNextThreadIdx;
  }

  xiiLog::Dev("Allocated {} additional '{}' worker threads ({} total)", uiAddThreads, xiiWorkerThreadType::GetThreadTypeName(type), s_pThreadState->m_iAllocatedWorkers[type]);
}

void xiiTaskSystem::WakeUpThreads(xiiWorkerThreadType::Enum type, xiiUInt32 uiNumThreadsToWakeUp)
{
  // together with xiiTaskWorkerThread::Run() this function will make sure to keep the number
  // of active threads close to m_uiMaxWorkersToUse
  //
  // threads that go into the 'blocked' state will raise the number of threads that get activated
  // and when they are unblocked, together they may exceed the 'maximum' number of active threads
  // but over time the threads at the end of the list will put themselves to sleep again

  auto* s = xiiTaskSystem::s_pThreadState.Borrow();

  const xiiUInt32 uiTotalThreads         = s_pThreadState->m_iAllocatedWorkers[type];
  xiiUInt32       uiAllowedActiveThreads = s_pThreadState->m_uiMaxWorkersToUse[type];

  for (xiiUInt32 threadIdx = 0; threadIdx < uiTotalThreads; ++threadIdx)
  {
    switch (s->m_Workers[type][threadIdx]->WakeUpIfIdle())
    {
      case xiiTaskWorkerState::Idle:
      {
        // was idle before -> now it is active
        if (--uiNumThreadsToWakeUp == 0)
          return;

        [[fallthrough]];
      }

      case xiiTaskWorkerState::Active:
      {
        // already active
        if (--uiAllowedActiveThreads == 0)
          return;

        break;
      }

      default:
        break;
    }
  }

  // if the loop above did not find enough threads to wake up
  if (uiNumThreadsToWakeUp > 0 && uiAllowedActiveThreads > 0)
  {
    // the new threads will start not-idle and take on some work
    AllocateThreads(type, xiiMath::Min(uiNumThreadsToWakeUp, uiAllowedActiveThreads));
  }
}

xiiWorkerThreadType::Enum xiiTaskSystem::GetCurrentThreadWorkerType()
{
  return tl_TaskWorkerInfo.m_WorkerType;
}

double xiiTaskSystem::GetThreadUtilization(xiiWorkerThreadType::Enum type, xiiUInt32 uiThreadIndex, xiiUInt32* pNumTasksExecuted /*= nullptr*/)
{
  return s_pThreadState->m_Workers[type][uiThreadIndex]->GetThreadUtilization(pNumTasksExecuted);
}

void xiiTaskSystem::DetermineTasksToExecuteOnThread(xiiTaskPriority::Enum& out_FirstPriority, xiiTaskPriority::Enum& out_LastPriority)
{
  switch (tl_TaskWorkerInfo.m_WorkerType)
  {
    case xiiWorkerThreadType::MainThread:
    {
      out_FirstPriority = xiiTaskPriority::ThisFrameMainThread;
      out_LastPriority  = xiiTaskPriority::SomeFrameMainThread;
      break;
    }

    case xiiWorkerThreadType::FileAccess:
    {
      out_FirstPriority = xiiTaskPriority::FileAccessHighPriority;
      out_LastPriority  = xiiTaskPriority::FileAccess;
      break;
    }

    case xiiWorkerThreadType::LongTasks:
    {
      out_FirstPriority = xiiTaskPriority::LongRunningHighPriority;
      out_LastPriority  = xiiTaskPriority::LongRunning;
      break;
    }

    case xiiWorkerThreadType::ShortTasks:
    {
      out_FirstPriority = xiiTaskPriority::EarlyThisFrame;
      out_LastPriority  = xiiTaskPriority::In9Frames;
      break;
    }

    case xiiWorkerThreadType::Unknown:
    {
      // probably a thread not launched through xii
      out_FirstPriority = xiiTaskPriority::EarlyThisFrame;
      out_LastPriority  = xiiTaskPriority::In9Frames;
      break;
    }

    default:
    {
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
    }
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemThreads);
