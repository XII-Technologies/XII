/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Threading/Implementation/Task.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/SharedPtr.h>

/// This system allows to automatically distribute tasks onto a number of worker threads.
///
/// By deriving from xiiTask you can create your own task types. These can be executed through this task system.
/// You can run a single task using the 'StartSingleTask' function. For more complex setups, it is possible
/// to create groups of tasks, which can have interdependencies. This should be used to group all
/// tasks that belong to one system and need to be done before another system runs. For example you could group
/// all tasks to update particle systems, and then have another group for all tasks to update sound, which depends
/// on the first group, such that sound is only updated after all particle systems are done with.
///
/// Although it is possible to wait for tasks or to cancel them, it is generally advised to try to minimize their use.
/// Tasks that might need to be canceled regularly (e.g. path searches) should be implemented in a way that they
/// are aware of being canceled and will stop their work prematurely, instead of running through to the end.
///
/// Note that it is crucial to call 'FinishFrameTasks' once per frame, otherwise tasks that need to be executed on the
/// main thread are never executed.
class XII_FOUNDATION_DLL xiiTaskSystem
{
public:
  /// \name Managing Tasks
  ///@{

public:
  /// A helper function to insert a single task into the system and start it right away. Returns ID of the Group into which the task
  /// has been put.
  static xiiTaskGroupID StartSingleTask(const xiiSharedPtr<xiiTask>& pTask, xiiTaskPriority::Enum priority, xiiOnTaskGroupFinishedCallback callback = xiiOnTaskGroupFinishedCallback()); // [tested]

  /// A helper function to insert a single task into the system and start it right away. Returns ID of the Group into which the task
  /// has been put. This overload allows to additionally specify a single dependency.
  static xiiTaskGroupID StartSingleTask(const xiiSharedPtr<xiiTask>& pTask, xiiTaskPriority::Enum priority, xiiTaskGroupID dependency, xiiOnTaskGroupFinishedCallback callback = xiiOnTaskGroupFinishedCallback()); // [tested]

  /// Call this function once at the end of a frame. It will ensure that all tasks for 'this frame' get finished properly.
  ///
  /// Calling this function is crucial for several reasons. It is the central function to execute 'main thread' tasks.
  /// Otherwise these tasks might never get executed. It also changes the priority of all 'next frame' tasks to 'this frame',
  /// so that those tasks are guaranteed to get finished when 'FinishFrameTasks' is called the next time.
  ///
  /// Finally this function executes tasks with the priority 'SomeFrameMainThread' as long as the target frame time is not exceeded.
  /// You can configure this with SetTargetFrameTime(), which defines how long (in milliseconds) the frame is allowed to be. As long
  /// as that time is not exceeded, additional 'SomeFrameMainThread' tasks will be executed.
  /// If the frame time spikes for a few frames, no such tasks will be executed, to prevent making it worse. However, if the frame
  /// time stays high over a longer period, 'FinishFrameTasks' will execute 'SomeFrameMainThread' tasks every once in a while,
  /// to guarantee some progress.
  ///
  /// \note After this function returns all tasks of priority 'ThisFrameMainThread' are finished. All tasks of priority
  /// 'EarlyThisFrame' up to 'LateThisFrame' are either finished or currently running on some thread, so they will be finished soon.
  /// There is however no guarantee that they are indeed all finished, as that would introduce unnecessary stalls.
  static void FinishFrameTasks(); // [tested]

  /// This function will try to remove the given task from the work queue, to prevent it from being executed.
  ///
  /// The function will return XII_SUCCESS, if the task could be removed and thus its execution could be prevented.
  /// It will also return XII_SUCCESS, if the task was already finished and nothing needed to be done.
  /// Tasks that are removed without execution will still be marked as 'finished' and dependent tasks will be scheduled.
  ///
  /// XII_FAILURE is returned, if the task had already been started and thus could not be prevented from running.
  ///
  /// In case of failure, \a bWaitForIt determines whether 'WaitForTask' is called (with all its consequences),
  /// or whether the function will return immediately.
  ///
  /// The cancel flag is set on the task, such that tasks that support canceling might terminate earlier.
  /// However, there is no guarantee how long it takes for already running tasks to actually finish.
  /// Therefore when bWaitForIt is true, this function might block for a very long time.
  /// It is advised to implement tasks that need to be canceled regularly (e.g. path searches for units that might die)
  /// in a way that allows for quick canceling.
  static xiiResult CancelTask(const xiiSharedPtr<xiiTask>& pTask, xiiOnTaskRunning::Enum onTaskRunning = xiiOnTaskRunning::WaitTillFinished); // [tested]

  struct TaskData
  {
    xiiSharedPtr<xiiTask> m_pTask;
    xiiTaskGroup*         m_pBelongsToGroup = nullptr;
    xiiUInt32             m_uiInvocation    = 0;
  };

  /// Broadcasts xiiThreadEvent::ClearThreadLocals on all worker threads.
  static void BroadcastClearThreadLocalsEvent();

private:
  /// Searches for a task of priority between \a FirstPriority and \a LastPriority (inclusive).
  static TaskData GetNextTask(xiiTaskPriority::Enum FirstPriority, xiiTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const xiiTaskGroupID& WaitingForGroup, xiiAtomicInteger32* pWorkerState);

  /// Executes some task of priority between \a FirstPriority and \a LastPriority (inclusive). Returns true, if any such task was available.
  static bool ExecuteTask(xiiTaskPriority::Enum FirstPriority, xiiTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const xiiTaskGroupID& WaitingForGroup, xiiAtomicInteger32* pWorkerState);

  /// Called whenever a task has been finished/canceled. Makes sure that groups are marked as finished when all tasks are done.
  static void TaskHasFinished(xiiSharedPtr<xiiTask>&& pTask, xiiTaskGroup* pGroup);

  /// Moves all 'next frame' tasks into the 'this frame' queues.
  static void ReprioritizeFrameTasks();

  /// Executes tasks of priority 'SomeFrameMainThread', as long as the last duration between frames is no longer than fSmoothFrameMS.
  static void ExecuteSomeFrameTasks(xiiTime smoothFrameTime);


  /// Helps executing tasks that are suitable for the calling thread. Returns true if a task was found and executed.
  static bool HelpExecutingTasks(const xiiTaskGroupID& WaitingForGroup);

  ///@}

  /// \name Managing Task Groups
  ///@{

public:
  /// Creates a new task group for one-time use. Groups need to be recreated every time a task is supposed to be inserted into the
  /// system.
  ///
  /// All tasks that are added to this group will be run with the same given \a Priority.
  /// Once all tasks in the group are finished and thus the group is finished, an optional \a Callback can be executed.
  static xiiTaskGroupID CreateTaskGroup(xiiTaskPriority::Enum priority, xiiOnTaskGroupFinishedCallback callback = xiiOnTaskGroupFinishedCallback()); // [tested]

  /// Adds a task to the given task group. The group must not yet have been started.
  static void AddTaskToGroup(xiiTaskGroupID group, const xiiSharedPtr<xiiTask>& pTask); // [tested]

  /// Adds a dependency on another group to \a Group. This means \a Group will not be execute before \a DependsOn has finished.
  ///
  /// \note Be careful with dependencies and task priorities. A task that has to execute 'this frame' should never depend on a task
  /// that needs only finish 'next frame', this might introduce very long and unnecessary waits.
  /// A task that has priority 'this frame' or 'next frame' will actually not be executed in 'this frame' or 'next frame' until all
  /// its dependencies are fulfilled. So you might add a long running task and a short task which depends on it, but the system will
  /// not block at the end of the frame, to wait for the long running task (to finish the short task thereafter), as that short task
  /// won't get scheduled for execution, at all, until all its dependencies are actually finished.
  static void AddTaskGroupDependency(xiiTaskGroupID group, xiiTaskGroupID dependsOn); // [tested]

  /// Same as AddTaskGroupDependency() but batches multiple dependency additions
  static void AddTaskGroupDependencyBatch(xiiArrayPtr<const xiiTaskGroupDependency> batch);

  /// Starts the task group. After this no further modifications on the group (new tasks or dependencies) are allowed.
  static void StartTaskGroup(xiiTaskGroupID group); // [tested]

  /// Same as StartTaskGroup() but batches multiple actions
  static void StartTaskGroupBatch(xiiArrayPtr<const xiiTaskGroupID> batch);

  /// Returns whether the given \a Group id refers to a task group that has been finished already.
  ///
  /// There is no time frame in which group IDs are valid. You may call this function at any time, even 10 minutes later,
  /// and it will correctly determine the results.
  static bool IsTaskGroupFinished(xiiTaskGroupID group); // [tested]

  /// Cancels all the tasks in the given group.
  ///
  /// XII_SUCCESS is returned, if all tasks were already finished or could be removed without waiting for any of them.
  /// XII_FAILURE is returned, if at least one task was being processed by another thread and could not be removed without waiting.
  /// If bWaitForIt is false, the function cancels all tasks, but returns without blocking, even if not all tasks have been finished.
  /// If bWaitForIt is true, the function returns only after it is guaranteed that all tasks are properly terminated.
  static xiiResult CancelGroup(xiiTaskGroupID group, xiiOnTaskRunning::Enum onTaskRunning = xiiOnTaskRunning::WaitTillFinished); // [tested]

  /// Blocks until all tasks in the given group have finished.
  ///
  /// If you need to wait for some other task to finish, this should always be the preferred method to do so.
  /// WaitForGroup will put the current thread to sleep and use thread signals to only wake it up again once the group is indeed
  /// finished. This is the most efficient way to wait for a task.
  static void WaitForGroup(xiiTaskGroupID group); // [tested]

  /// Blocks the current thread until the given delegate returns true.
  ///
  /// If possible, prefer to use WaitForGroup() to wait for some task to finish, as that is the most efficient way.
  /// If not possible, prefer to use WaitForCondition() instead of rolling your own busy-loop for polling some state.
  /// WaitForCondition() will NOT put the current thread to sleep, but instead keep polling the delegate. However, in between,
  /// it will try to execute other tasks and if there are no tasks that it could take on, it will wake up another worker thread
  /// thus guaranteeing, that there are enough unblocked threads in the system to do all the work.
  static void WaitForCondition(xiiDelegate<bool()> condition);

private:
  /// Takes all the tasks in the given group and schedules them for execution, by inserting them into the proper task lists.
  static void ScheduleGroupTasks(xiiTaskGroup* pGroup, bool bHighPriority);

  /// Is called whenever a dependency of pGroup has finished. Once all dependencies are finished, the group's tasks will get scheduled.
  static void DependencyHasFinished(xiiTaskGroup* pGroup);

  ///@}

  /// \name Thread Management
  ///@{

public:
  /// Sets the number of threads to use for the different task categories.
  ///
  /// \a uiShortTasks and \a uiLongTasks must be at least 1 and should not exceed the number of available CPU cores.
  /// There will always be exactly one additional thread for file access tasks (xiiTaskPriority::FileAccess).
  ///
  /// If \a uiShortTasks or \a uiLongTasks is smaller than 1, a default number of threads will be used for that type of work.
  /// This number of threads depends on the number of available CPU cores.
  /// If SetWorkThreadCount is never called, at all, the first time any task is started the number of worker threads is set to
  /// this default configuration.
  /// Unless you have a good idea how to set up the number of worker threads to make good use of the available cores,
  /// it is a good idea to just use the default settings.
  static void SetWorkerThreadCount(xiiInt32 iShortTasks = -1, xiiInt32 iLongTasks = -1); // [tested]

  /// Returns the maximum number of threads that should work on the given type of task at the same time.
  static xiiUInt32 GetWorkerThreadCount(xiiWorkerThreadType::Enum type);

  /// Returns the number of threads that have been allocated to potentially work on the given type of task.
  ///
  /// CAREFUL! This is not the number of threads that will be active at the same time. Use GetWorkerThreadCount() for that.
  /// This is the maximum number of threads that may jump in, if too many threads are blocked. This number will change dynamically
  /// at runtime to prevent deadlocks and it can grow very, very large.
  static xiiUInt32 GetNumAllocatedWorkerThreads(xiiWorkerThreadType::Enum type);

  /// Returns the (thread local) type of tasks that would be executed on this thread
  static xiiWorkerThreadType::Enum GetCurrentThreadWorkerType();

  /// Returns the utilization (0.0 to 1.0) of the given thread. Note: This will only be valid, if FinishFrameTasks() is called once
  /// per frame.
  ///
  /// Also optionally returns the number of tasks that were finished during the last frame.
  static double GetThreadUtilization(xiiWorkerThreadType::Enum type, xiiUInt32 uiThreadIndex, xiiUInt32* pNumTasksExecuted = nullptr);

  /// [internal] Wakes up or allocates up to \a uiNumThreads, unless enough threads are currently active and not blocked
  static void WakeUpThreads(xiiWorkerThreadType::Enum type, xiiUInt32 uiNumThreads);

private:
  friend class xiiTaskWorkerThread;

  /// Allocates \a uiAddThreads additional threads of \a type
  static void AllocateThreads(xiiWorkerThreadType::Enum type, xiiUInt32 uiAddThreads);

  /// Shuts down all worker threads. Does NOT finish the remaining tasks that were not started yet. Does not clear them either, though.
  static void StopWorkerThreads();

  /// Uses a thread local variable to know the current thread type and to decide the range of task priorities that it may execute
  static void DetermineTasksToExecuteOnThread(xiiTaskPriority::Enum& out_FirstPriority, xiiTaskPriority::Enum& out_LastPriority);

private:
  static xiiUniquePtr<xiiTaskSystemThreadState> s_pThreadState;

  ///@}

  /// \name Parallel For
  ///@{

public:
  /// A helper function to process task items in a parallel fashion by having per-worker index ranges generated.
  static void ParallelForIndexed(xiiUInt32 uiStartIndex, xiiUInt32 uiNumItems, xiiParallelForIndexedFunction32 taskCallback, xiiStringView sTaskName = {}, xiiTaskNesting taskNesting = xiiTaskNesting::Never, const xiiParallelForParams& params = xiiParallelForParams());

  /// A helper function to process task items in a parallel fashion by having per-worker index ranges generated.
  static void ParallelForIndexed(xiiUInt64 uiStartIndex, xiiUInt64 uiNumItems, xiiParallelForIndexedFunction64 taskCallback, xiiStringView sTaskName = {}, xiiTaskNesting taskNesting = xiiTaskNesting::Never, const xiiParallelForParams& params = xiiParallelForParams());

  /// A helper function to process task items in a parallel fashion by generating per-worker sub-ranges
  /// from an initial item array pointer.
  /// Given an array pointer 'taskItems' with elements of type ElemType, the following invocations are possible:
  ///   - ParallelFor(taskItems, [](xiiArrayPtr<ElemType> taskItemSlice) { });
  template <typename ElemType, typename Callback>
  static void ParallelFor(xiiArrayPtr<ElemType> taskItems, Callback taskCallback, xiiStringView sTaskName = {}, const xiiParallelForParams& params = xiiParallelForParams());

  /// A helper function to process task items in a parallel fashion and one-by-one (without global index).
  /// Given an array pointer 'taskItems' with elements of type ElemType, the following invocations are possible:
  ///   - ParallelFor(taskItems, [](ElemType taskItem) { });
  ///   - ParallelFor(taskItems, [](ElemType& taskItem) { });
  ///   - ParallelFor(taskItems, [](const ElemType& taskItem) { });
  template <typename ElemType, typename Callback>
  static void ParallelForSingle(xiiArrayPtr<ElemType> taskItems, Callback taskCallback, xiiStringView sTaskName = {}, const xiiParallelForParams& params = xiiParallelForParams());

  /// A helper function to process task items in a parallel fashion and one-by-one (with global index).
  /// Given an array pointer 'taskItems' with elements of type ElemType, the following invocations are possible:
  ///   - ParallelFor(taskItems, [](xiiUInt32 globalTaskItemIndex, ElemType taskItem) { });
  ///   - ParallelFor(taskItems, [](xiiUInt32 globalTaskItemIndex, ElemType& taskItem) { });
  ///   - ParallelFor(taskItems, [](xiiUInt32 globalTaskItemIndex, const ElemType& taskItem) { });
  template <typename ElemType, typename Callback>
  static void ParallelForSingleIndex(xiiArrayPtr<ElemType> taskItems, Callback taskCallback, xiiStringView sTaskName = {}, const xiiParallelForParams& params = xiiParallelForParams());

private:
  template <typename ElemType>
  static void ParallelForInternal(xiiArrayPtr<ElemType> taskItems, xiiParallelForFunction<ElemType> taskCallback, xiiStringView sTaskName, const xiiParallelForParams& params);

  ///@}

  /// \name Utilities
  ///@{

public:
  /// Writes the internal state of the xiiTaskSystem as a DGML graph.
  static void WriteStateSnapshotToDGML(xiiDGMLGraph& ref_graph);

  /// Convenience function to write the task graph snapshot to a file. If no path is given, the file is written to
  /// ":appdata/TaskGraphs/__date__.dgml"
  static void WriteStateSnapshotToFile(xiiStringView sPath = {});

private:
  ///@}

  /// \name Misc
  ///@{

public:
  /// Sets the target frame time that is supposed to not be exceeded.
  ///
  /// \see FinishFrameTasks() for more details.
  static void SetTargetFrameTime(xiiTime targetFrameTime = xiiTime::MakeFromSeconds(1.0 / 40.0) /* 40 FPS -> 25 ms */);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TaskSystem);

  static void Startup();
  static void Shutdown();

private:
  /// One mutex to rule them all.
  static xiiMutex s_TaskSystemMutex;

  static xiiUniquePtr<xiiTaskSystemState> s_pState;

  ///@}
};

#include <Foundation/Threading/Implementation/ParallelFor_inl.h>
