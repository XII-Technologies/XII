#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/Implementation/TaskWorkerThread.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>


xiiTaskGroupID xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::Enum Priority, xiiOnTaskGroupFinishedCallback callback)
{
  XII_LOCK(s_TaskSystemMutex);

  xiiUInt32 i = 0;

  // this search could be speed up with a stack of free groups
  for (; i < s_pState->m_TaskGroups.GetCount(); ++i)
  {
    if (!s_pState->m_TaskGroups[i].m_bInUse)
    {
      goto foundtaskgroup;
    }
  }

  // no free group found, create a new one
  s_pState->m_TaskGroups.ExpandAndGetRef();
  s_pState->m_TaskGroups[i].m_uiTaskGroupIndex = static_cast<xiiUInt16>(i);

foundtaskgroup:

  s_pState->m_TaskGroups[i].Reuse(Priority, callback);

  xiiTaskGroupID id;
  id.m_pTaskGroup     = &s_pState->m_TaskGroups[i];
  id.m_uiGroupCounter = s_pState->m_TaskGroups[i].m_uiGroupCounter;
  return id;
}

void xiiTaskSystem::AddTaskToGroup(xiiTaskGroupID groupID, const xiiSharedPtr<xiiTask>& pTask)
{
  XII_ASSERT_DEBUG(pTask != nullptr, "Cannot add nullptr tasks.");
  XII_ASSERT_DEV(pTask->IsTaskFinished(), "The given task is not finished! Cannot reuse a task before it is done.");
  XII_ASSERT_DEBUG(!pTask->m_sTaskName.IsEmpty(), "Every task should have a name");

  xiiTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  pTask->Reset();
  pTask->m_BelongsToGroup = groupID;
  groupID.m_pTaskGroup->m_Tasks.PushBack(pTask);
}

void xiiTaskSystem::AddTaskGroupDependency(xiiTaskGroupID groupID, xiiTaskGroupID DependsOn)
{
  XII_ASSERT_DEBUG(DependsOn.IsValid(), "Invalid dependency");
  XII_ASSERT_DEBUG(groupID.m_pTaskGroup != DependsOn.m_pTaskGroup || groupID.m_uiGroupCounter != DependsOn.m_uiGroupCounter, "Group cannot depend on itselfs");

  xiiTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  groupID.m_pTaskGroup->m_DependsOnGroups.PushBack(DependsOn);
}

void xiiTaskSystem::AddTaskGroupDependencyBatch(xiiArrayPtr<const xiiTaskGroupDependency> batch)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // lock here once to reduce the overhead of xiiTaskGroup::DebugCheckTaskGroup inside AddTaskGroupDependency
  XII_LOCK(s_TaskSystemMutex);
#endif

  for (const xiiTaskGroupDependency& dep : batch)
  {
    AddTaskGroupDependency(dep.m_TaskGroup, dep.m_DependsOn);
  }
}

void xiiTaskSystem::StartTaskGroup(xiiTaskGroupID groupID)
{
  XII_ASSERT_DEV(s_pThreadState->m_Workers[xiiWorkerThreadType::ShortTasks].GetCount() > 0, "No worker threads started.");

  xiiTaskGroup::DebugCheckTaskGroup(groupID, s_TaskSystemMutex);

  xiiInt32 iActiveDependencies = 0;

  {
    XII_LOCK(s_TaskSystemMutex);

    xiiTaskGroup& tg = *groupID.m_pTaskGroup;

    tg.m_bStartedByUser = true;

    for (xiiUInt32 i = 0; i < tg.m_DependsOnGroups.GetCount(); ++i)
    {
      if (!IsTaskGroupFinished(tg.m_DependsOnGroups[i]))
      {
        xiiTaskGroup& Dependency = *tg.m_DependsOnGroups[i].m_pTaskGroup;

        // add this task group to the list of dependencies, such that when that group finishes, this task group can get woken up
        Dependency.m_OthersDependingOnMe.PushBack(groupID);

        // count how many other groups need to finish before this task group can be executed
        ++iActiveDependencies;
      }
    }

    if (iActiveDependencies != 0)
    {
      // atomic integers are quite slow, so do not use them in the loop, where they are not yet needed
      tg.m_iNumActiveDependencies = iActiveDependencies;
    }
  }

  if (iActiveDependencies == 0)
  {
    ScheduleGroupTasks(groupID.m_pTaskGroup, false);
  }
}

void xiiTaskSystem::StartTaskGroupBatch(xiiArrayPtr<const xiiTaskGroupID> batch)
{
  XII_LOCK(s_TaskSystemMutex);

  for (const xiiTaskGroupID& group : batch)
  {
    StartTaskGroup(group);
  }
}

bool xiiTaskSystem::IsTaskGroupFinished(xiiTaskGroupID Group)
{
  // if the counters differ, the task group has been reused since the GroupID was created, so that group has finished
  return (Group.m_pTaskGroup == nullptr) || (Group.m_pTaskGroup->m_uiGroupCounter != Group.m_uiGroupCounter);
}

void xiiTaskSystem::ScheduleGroupTasks(xiiTaskGroup* pGroup, bool bHighPriority)
{
  if (pGroup->m_Tasks.IsEmpty())
  {
    pGroup->m_iNumRemainingTasks = 1;

    // "finish" one task -> will finish the task group and kick off dependent groups
    TaskHasFinished(nullptr, pGroup);
    return;
  }

  xiiInt32 iRemainingTasks = 0;

  // add all the tasks to the task list, so that they will be processed
  {
    XII_LOCK(s_TaskSystemMutex);


    // store how many tasks from this groups still need to be processed

    for (auto pTask : pGroup->m_Tasks)
    {
      iRemainingTasks += xiiMath::Max(1u, pTask->m_uiMultiplicity);
      pTask->m_iRemainingRuns = xiiMath::Max(1u, pTask->m_uiMultiplicity);
    }

    pGroup->m_iNumRemainingTasks = iRemainingTasks;


    for (xiiUInt32 task = 0; task < pGroup->m_Tasks.GetCount(); ++task)
    {
      auto& pTask = pGroup->m_Tasks[task];

      for (xiiUInt32 mult = 0; mult < xiiMath::Max(1u, pTask->m_uiMultiplicity); ++mult)
      {
        TaskData td;
        td.m_pBelongsToGroup           = pGroup;
        td.m_pTask                     = pTask;
        td.m_pTask->m_bTaskIsScheduled = true;
        td.m_uiInvocation              = mult;

        if (bHighPriority)
          s_pState->m_Tasks[pGroup->m_Priority].PushFront(td);
        else
          s_pState->m_Tasks[pGroup->m_Priority].PushBack(td);
      }
    }

    // send the proper thread signal, to make sure one of the correct worker threads is awake
    switch (pGroup->m_Priority)
    {
      case xiiTaskPriority::EarlyThisFrame:
      case xiiTaskPriority::ThisFrame:
      case xiiTaskPriority::LateThisFrame:
      case xiiTaskPriority::EarlyNextFrame:
      case xiiTaskPriority::NextFrame:
      case xiiTaskPriority::LateNextFrame:
      case xiiTaskPriority::In2Frames:
      case xiiTaskPriority::In3Frames:
      case xiiTaskPriority::In4Frames:
      case xiiTaskPriority::In5Frames:
      case xiiTaskPriority::In6Frames:
      case xiiTaskPriority::In7Frames:
      case xiiTaskPriority::In8Frames:
      case xiiTaskPriority::In9Frames:
      {
        WakeUpThreads(xiiWorkerThreadType::ShortTasks, iRemainingTasks);
        break;
      }

      case xiiTaskPriority::LongRunning:
      case xiiTaskPriority::LongRunningHighPriority:
      {
        WakeUpThreads(xiiWorkerThreadType::LongTasks, iRemainingTasks);
        break;
      }

      case xiiTaskPriority::FileAccess:
      case xiiTaskPriority::FileAccessHighPriority:
      {
        WakeUpThreads(xiiWorkerThreadType::FileAccess, iRemainingTasks);
        break;
      }

      case xiiTaskPriority::SomeFrameMainThread:
      case xiiTaskPriority::ThisFrameMainThread:
      case xiiTaskPriority::ENUM_COUNT:
        // nothing to do for these enum values
        break;
    }
  }
}

void xiiTaskSystem::DependencyHasFinished(xiiTaskGroup* pGroup)
{
  // remove one dependency from the group
  if (pGroup->m_iNumActiveDependencies.Decrement() == 0)
  {
    // if there are no remaining dependencies, kick off all tasks in this group
    ScheduleGroupTasks(pGroup, true);
  }
}

xiiResult xiiTaskSystem::CancelGroup(xiiTaskGroupID Group, xiiOnTaskRunning::Enum OnTaskRunning)
{
  if (xiiTaskSystem::IsTaskGroupFinished(Group))
    return XII_SUCCESS;

  XII_PROFILE_SCOPE("CancelGroup");

  XII_LOCK(s_TaskSystemMutex);

  xiiResult res = XII_SUCCESS;

  auto TasksCopy = Group.m_pTaskGroup->m_Tasks;

  // first cancel ALL the tasks in the group, without waiting for anything
  for (xiiUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
  {
    if (CancelTask(TasksCopy[task], xiiOnTaskRunning::ReturnWithoutBlocking) == XII_FAILURE)
    {
      res = XII_FAILURE;
    }
  }

  // if all tasks could be removed without problems, we do not need to try it again with blocking

  if (OnTaskRunning == xiiOnTaskRunning::WaitTillFinished && res == XII_FAILURE)
  {
    // now cancel the tasks in the group again, this time wait for those that are already running
    for (xiiUInt32 task = 0; task < TasksCopy.GetCount(); ++task)
    {
      CancelTask(TasksCopy[task], xiiOnTaskRunning::WaitTillFinished).IgnoreResult();
    }
  }

  return res;
}

void xiiTaskSystem::WaitForGroup(xiiTaskGroupID Group)
{
  XII_PROFILE_SCOPE("WaitForGroup");

  XII_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep    = ThreadTaskType != xiiWorkerThreadType::MainThread;

  while (!xiiTaskSystem::IsTaskGroupFinished(Group))
  {
    if (!HelpExecutingTasks(Group))
    {
      if (bAllowSleep)
      {
        const xiiWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == xiiWorkerThreadType::Unknown) ? xiiWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          XII_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)xiiTaskWorkerState::Blocked) == (int)xiiTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        Group.m_pTaskGroup->WaitForFinish(Group);

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          XII_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)xiiTaskWorkerState::Active) == (int)xiiTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
  }
}

void xiiTaskSystem::WaitForCondition(xiiDelegate<bool()> condition)
{
  XII_PROFILE_SCOPE("WaitForCondition");

  XII_ASSERT_DEV(tl_TaskWorkerInfo.m_bAllowNestedTasks, "The executing task '{}' is flagged to never wait for other tasks but does so anyway. Remove the flag or remove the wait-dependency.", tl_TaskWorkerInfo.m_szTaskName);

  const auto ThreadTaskType = tl_TaskWorkerInfo.m_WorkerType;
  const bool bAllowSleep    = ThreadTaskType != xiiWorkerThreadType::MainThread;

  while (!condition())
  {
    if (!HelpExecutingTasks(xiiTaskGroupID()))
    {
      if (bAllowSleep)
      {
        const xiiWorkerThreadType::Enum typeToWakeUp = (ThreadTaskType == xiiWorkerThreadType::Unknown) ? xiiWorkerThreadType::ShortTasks : ThreadTaskType;

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          XII_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)xiiTaskWorkerState::Blocked) == (int)xiiTaskWorkerState::Active, "Corrupt worker state");
        }

        WakeUpThreads(typeToWakeUp, 1);

        while (!condition())
        {
          // TODO: busy loop for now
          xiiThreadUtils::YieldTimeSlice();
        }

        if (tl_TaskWorkerInfo.m_pWorkerState)
        {
          XII_VERIFY(tl_TaskWorkerInfo.m_pWorkerState->Set((int)xiiTaskWorkerState::Active) == (int)xiiTaskWorkerState::Blocked, "Corrupt worker state");
        }

        break;
      }
      else
      {
        xiiThreadUtils::YieldTimeSlice();
      }
    }
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemGroups);
