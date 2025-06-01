#include <Foundation/FoundationPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>

xiiTaskGroupID xiiTaskSystem::StartSingleTask(const xiiSharedPtr<xiiTask>& pTask, xiiTaskPriority::Enum priority, xiiTaskGroupID dependency, xiiOnTaskGroupFinishedCallback callback /*= xiiOnTaskGroupFinishedCallback()*/)
{
  xiiTaskGroupID Group = CreateTaskGroup(priority, callback);
  AddTaskGroupDependency(Group, dependency);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

xiiTaskGroupID xiiTaskSystem::StartSingleTask(const xiiSharedPtr<xiiTask>& pTask, xiiTaskPriority::Enum priority, xiiOnTaskGroupFinishedCallback callback /*= xiiOnTaskGroupFinishedCallback()*/)
{
  xiiTaskGroupID Group = CreateTaskGroup(priority, callback);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

void xiiTaskSystem::TaskHasFinished(xiiSharedPtr<xiiTask>&& pTask, xiiTaskGroup* pGroup)
{
  // call task finished callback and deallocate the task (if last reference)
  if (pTask && pTask->m_iRemainingRuns == 0)
  {
    if (pTask->m_OnTaskFinished.IsValid())
    {
      pTask->m_OnTaskFinished(pTask);
    }

    // make sure to clear the task sharedptr BEFORE we mark the task (group) as finished,
    // so that if this is the last reference, the task gets deallocated first
    pTask.Clear();
  }

  if (pGroup->m_iNumRemainingTasks.Decrement() == 0)
  {
    // If this was the last task that had to be finished from this group, make sure all dependent groups are started

    xiiUInt32 uiGroupCounter = 0;
    {
      // see xiiTaskGroup::WaitForFinish() for why we need this lock here
      // without it, there would be a race condition between these two places, reading and writing m_uiGroupCounter and waiting/signaling
      // m_CondVarGroupFinished
      XII_LOCK(pGroup->m_CondVarGroupFinished);

      uiGroupCounter = pGroup->m_uiGroupCounter;

      // set this task group to be finished such that no one tries to append further dependencies
      pGroup->m_uiGroupCounter += 2;
    }

    {
      XII_LOCK(s_TaskSystemMutex);

      // unless an outside reference is held onto a task, this will deallocate the tasks
      pGroup->m_Tasks.Clear();

      for (xiiUInt32 dep = 0; dep < pGroup->m_OthersDependingOnMe.GetCount(); ++dep)
      {
        DependencyHasFinished(pGroup->m_OthersDependingOnMe[dep].m_pTaskGroup);
      }
    }

    // wake up all threads that are waiting for this group
    pGroup->m_CondVarGroupFinished.SignalAll();

    if (pGroup->m_OnFinishedCallback.IsValid())
    {
      xiiTaskGroupID id;
      id.m_pTaskGroup     = pGroup;
      id.m_uiGroupCounter = uiGroupCounter;
      pGroup->m_OnFinishedCallback(id);
    }

    // set this task available for reuse
    pGroup->m_bInUse = false;
  }
}

xiiTaskSystem::TaskData xiiTaskSystem::GetNextTask(xiiTaskPriority::Enum FirstPriority, xiiTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const xiiTaskGroupID& WaitingForGroup, xiiAtomicInteger32* pWorkerState)
{
  // this is the central function that selects tasks for the worker threads to work on

  XII_ASSERT_DEV(FirstPriority >= xiiTaskPriority::EarlyThisFrame && LastPriority < xiiTaskPriority::ENUM_COUNT, "Priority Range is invalid: {0} to {1}", FirstPriority, LastPriority);

  XII_LOCK(s_TaskSystemMutex);

  // go through all the task lists that this thread is willing to work on
  for (xiiUInt32 prio = FirstPriority; prio <= (xiiUInt32)LastPriority; ++prio)
  {
    for (auto it = s_pState->m_Tasks[prio].GetIterator(); it.IsValid(); ++it)
    {
      if (!bOnlyTasksThatNeverWait || (it->m_pTask->m_NestingMode == xiiTaskNesting::Never) || it->m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup)
      {
        TaskData td = *it;

        s_pState->m_Tasks[prio].Remove(it);
        return td;
      }
    }
  }

  if (pWorkerState)
  {
    XII_VERIFY(pWorkerState->Set((xiiInt32)xiiTaskWorkerState::Idle) == (xiiInt32)xiiTaskWorkerState::Active, "Corrupt Worker State");
  }

  return TaskData();
}

bool xiiTaskSystem::ExecuteTask(xiiTaskPriority::Enum FirstPriority, xiiTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const xiiTaskGroupID& WaitingForGroup, xiiAtomicInteger32* pWorkerState)
{
  // const xiiWorkerThreadType::Enum workerType = (tl_TaskWorkerInfo.m_WorkerType == xiiWorkerThreadType::Unknown) ? xiiWorkerThreadType::ShortTasks :
  // tl_TaskWorkerInfo.m_WorkerType;

  xiiTaskSystem::TaskData td = GetNextTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, pWorkerState);

  if (td.m_pTask == nullptr)
    return false;

  if (bOnlyTasksThatNeverWait && td.m_pTask->m_NestingMode != xiiTaskNesting::Never)
  {
    XII_ASSERT_DEV(td.m_pBelongsToGroup == WaitingForGroup.m_pTaskGroup, "");
  }

  tl_TaskWorkerInfo.m_bAllowNestedTasks = td.m_pTask->m_NestingMode != xiiTaskNesting::Never;
  tl_TaskWorkerInfo.m_szTaskName        = td.m_pTask->m_sTaskName;
  td.m_pTask->Run(td.m_uiInvocation);
  tl_TaskWorkerInfo.m_bAllowNestedTasks = true;
  tl_TaskWorkerInfo.m_szTaskName        = nullptr;

  // notify the group, that a task is finished, which might trigger other tasks to be executed
  TaskHasFinished(std::move(td.m_pTask), td.m_pBelongsToGroup);

  return true;
}


xiiResult xiiTaskSystem::CancelTask(const xiiSharedPtr<xiiTask>& pTask, xiiOnTaskRunning::Enum onTaskRunning)
{
  if (pTask->IsTaskFinished())
    return XII_SUCCESS;

  // pTask may actually finish between here and the lock below
  // in that case we will return failure, as in we had to 'wait' for a task,
  // but it will be handled correctly

  XII_PROFILE_SCOPE("CancelTask");

  // we set the cancel flag, to make sure that tasks that support canceling will terminate asap
  pTask->m_bCancelExecution = true;

  {
    XII_LOCK(s_TaskSystemMutex);

    // if the task is still in the queue of its group, it had not yet been scheduled
    if (!pTask->m_bTaskIsScheduled && pTask->m_BelongsToGroup.m_pTaskGroup->m_Tasks.RemoveAndSwap(pTask))
    {
      // we set the task to finished, even though it was not executed
      pTask->m_iRemainingRuns = 0;
      return XII_SUCCESS;
    }

    // check if the task has already been scheduled for execution
    // if so, remove it from the work queue
    {
      for (xiiUInt32 i = 0; i < xiiTaskPriority::ENUM_COUNT; ++i)
      {
        auto it = s_pState->m_Tasks[i].GetIterator();

        while (it.IsValid())
        {
          if (it->m_pTask == pTask)
          {
            // we set the task to finished, even though it was not executed
            pTask->m_iRemainingRuns = 0;

            // tell the system that one task of that group is 'finished', to ensure its dependencies will get scheduled
            TaskHasFinished(std::move(it->m_pTask), it->m_pBelongsToGroup);

            s_pState->m_Tasks[i].Remove(it);
            return XII_SUCCESS;
          }

          ++it;
        }
      }
    }
  }

  // if we made it here, the task was already running
  // thus we just wait for it to finish

  if (onTaskRunning == xiiOnTaskRunning::WaitTillFinished)
  {
    WaitForCondition([pTask]() { return pTask->IsTaskFinished(); });
  }

  return XII_FAILURE;
}


bool xiiTaskSystem::HelpExecutingTasks(const xiiTaskGroupID& WaitingForGroup)
{
  const bool bOnlyTasksThatNeverWait = tl_TaskWorkerInfo.m_WorkerType != xiiWorkerThreadType::MainThread;

  xiiTaskPriority::Enum FirstPriority;
  xiiTaskPriority::Enum LastPriority;
  DetermineTasksToExecuteOnThread(FirstPriority, LastPriority);

  return ExecuteTask(FirstPriority, LastPriority, bOnlyTasksThatNeverWait, WaitingForGroup, nullptr);
}

void xiiTaskSystem::ReprioritizeFrameTasks()
{
  // There should usually be no 'this frame tasks' left at this time
  // however, while we waited to enter the lock, such tasks might have appeared
  // In this case we move them into the highest-priority 'this frame' queue, to ensure they will be executed asap
  for (xiiUInt32 i = (xiiUInt32)xiiTaskPriority::ThisFrame; i <= (xiiUInt32)xiiTaskPriority::LateThisFrame; ++i)
  {
    auto it = s_pState->m_Tasks[i].GetIterator();

    // move all 'this frame' tasks into the 'early this frame' queue
    while (it.IsValid())
    {
      s_pState->m_Tasks[xiiTaskPriority::EarlyThisFrame].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_pState->m_Tasks[i].Clear();
  }

  for (xiiUInt32 i = (xiiUInt32)xiiTaskPriority::EarlyNextFrame; i <= (xiiUInt32)xiiTaskPriority::LateNextFrame; ++i)
  {
    auto it = s_pState->m_Tasks[i].GetIterator();

    // move all 'next frame' tasks into the 'this frame' queues
    while (it.IsValid())
    {
      s_pState->m_Tasks[i - 3].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_pState->m_Tasks[i].Clear();
  }

  for (xiiUInt32 i = (xiiUInt32)xiiTaskPriority::In2Frames; i <= (xiiUInt32)xiiTaskPriority::In9Frames; ++i)
  {
    auto it = s_pState->m_Tasks[i].GetIterator();

    // move all 'in N frames' tasks into the 'in N-1 frames' queues
    // moves 'In2Frames' into 'LateNextFrame'
    while (it.IsValid())
    {
      s_pState->m_Tasks[i - 1].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_pState->m_Tasks[i].Clear();
  }
}

void xiiTaskSystem::ExecuteSomeFrameTasks(xiiTime smoothFrameTime)
{
  XII_PROFILE_SCOPE("ExecuteSomeFrameTasks");

  // 'SomeFrameMainThread' tasks are usually used to upload resources that have been loaded in the background they do not need to be executed right away, but the earlier, the better.

  // As long as the frame time is short enough, execute tasks that need to be done on the main thread on fast machines that means that these tasks are finished as soon as possible and users will see the results quickly.

  // If the frame time spikes, we can skip this a few times, to try to prevent further slow downs however in such instances, the 'frame time threshold' will increase and thus the chance that we skip this entirely becomes lower over time that guarantees some progress, even if the frame rate is constantly low.

  static xiiTime s_FrameTimeThreshold = smoothFrameTime;
  static xiiTime s_LastExecution; // initializes to zero -> very large frame time difference at first

  xiiTime CurTime  = xiiTime::Now();
  xiiTime LastTime = s_LastExecution;
  s_LastExecution  = CurTime;

  // As long as we have a smooth frame rate, execute as many of these tasks, as possible.
  while (CurTime - LastTime < smoothFrameTime)
  {
    if (!ExecuteTask(xiiTaskPriority::SomeFrameMainThread, xiiTaskPriority::SomeFrameMainThread, false, xiiTaskGroupID(), nullptr))
    {
      // nothing left to do, reset the threshold
      s_FrameTimeThreshold = smoothFrameTime;
      return;
    }

    CurTime = xiiTime::Now();
  }

  xiiUInt32 uiPendingTaskCount = 0U;

  {
    XII_LOCK(s_TaskSystemMutex);

    uiPendingTaskCount = s_pState->m_Tasks[xiiTaskPriority::SomeFrameMainThread].GetCount();
  }

  if (uiPendingTaskCount == 0)
    return;

  if (CurTime - LastTime < s_FrameTimeThreshold) // the accumulating threshold has caught up with us
  {
    // don't reset the threshold, from now on we execute at least one task per frame

    ExecuteTask(xiiTaskPriority::SomeFrameMainThread, xiiTaskPriority::SomeFrameMainThread, false, xiiTaskGroupID(), nullptr);
  }
  else
  {
    // Increase the threshold slightly every time we skip the work.
    // This means that when the frame rate is too low, we can ignore these tasks for a few frames and thus prevent decreasing the frame rate even further.
    // However, we increase the time threshold, at which we skip this, further and further therefore at some point we will start executing these tasks, no matter how low the frame rate is.
    //
    // This gives us some buffer to smooth out performance drops.
    s_FrameTimeThreshold += xiiTime::MakeFromMilliseconds(0.2);
  }

  // If the queue is really full, we have to guarantee more progress.
  {
    if (uiPendingTaskCount > 100)
      ExecuteTask(xiiTaskPriority::SomeFrameMainThread, xiiTaskPriority::SomeFrameMainThread, false, xiiTaskGroupID(), nullptr);

    if (uiPendingTaskCount > 75)
      ExecuteTask(xiiTaskPriority::SomeFrameMainThread, xiiTaskPriority::SomeFrameMainThread, false, xiiTaskGroupID(), nullptr);

    if (uiPendingTaskCount > 50)
      ExecuteTask(xiiTaskPriority::SomeFrameMainThread, xiiTaskPriority::SomeFrameMainThread, false, xiiTaskGroupID(), nullptr);
  }
}


void xiiTaskSystem::FinishFrameTasks()
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "This function must be executed on the main thread.");

  // make sure all 'main thread' and 'short' tasks are either finished or being worked on by other threads already
  {
    while (true)
    {
      // Prefer to work on main-thread tasks
      if (ExecuteTask(xiiTaskPriority::ThisFrameMainThread, xiiTaskPriority::ThisFrameMainThread, false, xiiTaskGroupID(), nullptr))
      {
        continue;
      }

      // if there are none, help out with the other tasks for this frame
      if (ExecuteTask(xiiTaskPriority::EarlyThisFrame, xiiTaskPriority::LateThisFrame, false, xiiTaskGroupID(), nullptr))
      {
        continue;
      }

      break;
    }
  }

  // all the important tasks for this frame should be finished or worked on by now
  // so we can now re-prioritize the tasks for the next frame
  {
    XII_LOCK(s_TaskSystemMutex);

    ReprioritizeFrameTasks();
  }

  ExecuteSomeFrameTasks(s_pState->m_TargetFrameTime);

  // Update the thread utilization
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    const xiiTime  tNow              = xiiTime::Now();
    static xiiTime s_LastFrameUpdate = tNow;
    const xiiTime  tDiff             = tNow - s_LastFrameUpdate;

    // prevent division by zero (inside ComputeThreadUtilization)
    if (tDiff > xiiTime::MakeFromSeconds(0.0))
    {
      s_LastFrameUpdate = tNow;

      for (xiiUInt32 type = 0; type < xiiWorkerThreadType::ENUM_COUNT; ++type)
      {
        const xiiUInt32 uiNumWorkers = s_pThreadState->m_iAllocatedWorkers[type];

        for (xiiUInt32 t = 0; t < uiNumWorkers; ++t)
        {
          s_pThreadState->m_Workers[type][t]->UpdateThreadUtilization(tDiff);
        }
      }
    }
#endif
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemTasks);
