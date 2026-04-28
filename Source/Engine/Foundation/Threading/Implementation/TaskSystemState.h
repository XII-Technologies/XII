/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Threading/TaskSystem.h>

class xiiTaskSystemThreadState
{
private:
  friend class xiiTaskSystem;
  friend class xiiTaskWorkerThread;

  // The arrays of all the active worker threads.
  xiiDynamicArray<xiiTaskWorkerThread*> m_Workers[xiiWorkerThreadType::ENUM_COUNT];

  // the number of allocated (non-null) worker threads in m_Workers
  xiiAtomicInteger32 m_iAllocatedWorkers[xiiWorkerThreadType::ENUM_COUNT];

  // the maximum number of worker threads that should be non-idle (and not blocked) at any time
  xiiUInt32 m_uiMaxWorkersToUse[xiiWorkerThreadType::ENUM_COUNT] = {};
};

class xiiTaskSystemState
{
private:
  friend class xiiTaskSystem;

  // The target frame time used by FinishFrameTasks()
  xiiTime m_TargetFrameTime = xiiTime::MakeFromSeconds(1.0 / 40.0); // => 25 ms

  // The deque can grow without relocating existing data, therefore the xiiTaskGroupID's can store pointers directly to the data
  xiiDeque<xiiTaskGroup> m_TaskGroups;

  // The lists of all scheduled tasks, for each priority.
  xiiList<xiiTaskSystem::TaskData> m_Tasks[xiiTaskPriority::ENUM_COUNT];
};
