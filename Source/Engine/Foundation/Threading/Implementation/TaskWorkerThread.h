/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>

/// \internal Internal task worker thread class.
class xiiTaskWorkerThread final : public xiiThread
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiTaskWorkerThread);

  /// \name Execution
  ///@{

public:
  /// Tells the worker thread what tasks to execute and which thread index it has.
  xiiTaskWorkerThread(xiiWorkerThreadType::Enum threadType, xiiUInt32 uiThreadNumber);
  ~xiiTaskWorkerThread();

  /// Deactivates the thread. Returns failure, if the thread is currently still running.
  xiiResult DeactivateWorker();

  /// Broadcasts xiiThreadEvent::ClearThreadLocals on this thread.
  void BroadcastClearThreadLocalsEvent();

  void WaitForBroadcastClearTLS();

private:
  // Which types of tasks this thread should work on.
  xiiWorkerThreadType::Enum m_WorkerType;

  // Whether the thread is supposed to continue running.
  volatile bool m_bActive                 = true;
  bool          m_bClearThreadLocalsEvent = false;

  // For display purposes.
  xiiUInt16 m_uiWorkerThreadNumber = 0xFFFF;

  ///@}

  /// \name Thread Utilization
  ///@{

public:
  /// Returns the last utilization value (0 - 1 range). Optionally returns how many tasks it executed recently.
  double GetThreadUtilization(xiiUInt32* pNumTasksExecuted = nullptr);

  /// Computes the thread utilization by dividing the thread active time by the time that has passed since the last update.
  void UpdateThreadUtilization(xiiTime timePassed);

private:
  bool      m_bExecutingTask         = false;
  xiiUInt16 m_uiLastNumTasksExecuted = 0;
  xiiUInt16 m_uiNumTasksExecuted     = 0;
  xiiTime   m_StartedWorkingTime;
  xiiTime   m_ThreadActiveTime;
  double    m_fLastThreadUtilization = 0.0;

  ///@}

  /// \name Idle State
  ///@{

public:
  /// If the thread is currently idle, this will wake it up and return XII_SUCCESS.
  xiiTaskWorkerState WakeUpIfIdle();

private:
  // Puts the thread to sleep (idle state)
  void WaitForWork();

  virtual xiiUInt32 Run() override;

  // used to wake up idle threads, see m_WorkerState
  xiiThreadSignal m_WakeUpSignal;

  // used to indicate whether this thread is currently idle
  // if so, it can be woken up using m_WakeUpSignal
  // xiiAtomicBool m_bIsIdle = false;
  xiiAtomicInteger32 m_iWorkerState; // xiiTaskWorkerState

  ///@}
};

/// \internal Thread local state used by the task system (and for better debugging)
struct xiiTaskWorkerInfo
{
  xiiWorkerThreadType::Enum m_WorkerType        = xiiWorkerThreadType::Unknown;
  bool                      m_bAllowNestedTasks = true;
  xiiInt32                  m_iWorkerIndex      = -1;
  const char*               m_szTaskName        = nullptr;
  xiiAtomicInteger32*       m_pWorkerState      = nullptr;
};

extern thread_local xiiTaskWorkerInfo tl_TaskWorkerInfo;
