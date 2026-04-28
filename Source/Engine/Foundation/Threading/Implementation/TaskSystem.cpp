/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/TaskSystem.h>

xiiMutex                               xiiTaskSystem::s_TaskSystemMutex;
xiiUniquePtr<xiiTaskSystemState>       xiiTaskSystem::s_pState;
xiiUniquePtr<xiiTaskSystemThreadState> xiiTaskSystem::s_pThreadState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TaskSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ThreadUtils",
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    if (xiiStartup::HasApplicationTag("NoTaskSystem"))
      return;

    xiiTaskSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiTaskSystem::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

void xiiTaskSystem::Startup()
{
  s_pThreadState = XII_DEFAULT_NEW(xiiTaskSystemThreadState);
  s_pState       = XII_DEFAULT_NEW(xiiTaskSystemState);

  tl_TaskWorkerInfo.m_WorkerType   = xiiWorkerThreadType::MainThread;
  tl_TaskWorkerInfo.m_iWorkerIndex = 0;

  // Initialize with the default number of worker threads
  SetWorkerThreadCount();
}

void xiiTaskSystem::Shutdown()
{
  if (s_pThreadState == nullptr)
    return;

  StopWorkerThreads();

  s_pState.Clear();
  s_pThreadState.Clear();
}

void xiiTaskSystem::SetTargetFrameTime(xiiTime targetFrameTime)
{
  s_pState->m_TargetFrameTime = targetFrameTime;
}

void xiiTaskSystem::BroadcastClearThreadLocalsEvent()
{
  for (xiiUInt32 i = 0; i < xiiWorkerThreadType::ENUM_COUNT; ++i)
  {
    for (xiiTaskWorkerThread* pWorker : s_pThreadState->m_Workers[i])
    {
      if (pWorker)
      {
        pWorker->BroadcastClearThreadLocalsEvent();
      }
    }
  }

  // make sure they have all sent the event
  for (xiiUInt32 i = 0; i < xiiWorkerThreadType::ENUM_COUNT; ++i)
  {
    for (xiiTaskWorkerThread* pWorker : s_pThreadState->m_Workers[i])
    {
      if (pWorker)
      {
        pWorker->WaitForBroadcastClearTLS();
      }
    }
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
