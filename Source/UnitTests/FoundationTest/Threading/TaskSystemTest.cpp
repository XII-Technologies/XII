#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Utilities/DGMLWriter.h>

class xiiTestTask final : public xiiTask
{
public:
  xiiUInt32    m_uiIterations;
  xiiTestTask* m_pDependency;
  bool         m_bSupportCancel;
  xiiInt32     m_iTaskID;

  xiiTestTask()
  {
    m_uiIterations   = 50;
    m_pDependency    = nullptr;
    m_bStarted       = false;
    m_bDone          = false;
    m_bSupportCancel = false;
    m_iTaskID        = -1;

    ConfigureTask("xiiTestTask", xiiTaskNesting::Never);
  }

  bool IsStarted() const { return m_bStarted; }
  bool IsDone() const { return m_bDone; }
  bool IsMultiplicityDone() const { return m_iMultiplicityCount == (int)GetMultiplicity(); }

private:
  bool                       m_bStarted;
  bool                       m_bDone;
  mutable xiiAtomicInteger32 m_iMultiplicityCount;

  virtual void ExecuteWithMultiplicity(xiiUInt32 uiInvocation) const override { m_iMultiplicityCount.Increment(); }

  virtual void Execute() override
  {
    if (m_iTaskID >= 0)
      xiiLog::Printf("Starting Task %i at %.4f\n", m_iTaskID, xiiTime::Now().GetSeconds());

    m_bStarted = true;

    XII_TEST_BOOL(m_pDependency == nullptr || m_pDependency->IsTaskFinished());

    for (xiiUInt32 obst = 0; obst < m_uiIterations; ++obst)
    {
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
      xiiTime::Now();

      if (HasBeenCanceled() && m_bSupportCancel)
      {
        if (m_iTaskID >= 0)
          xiiLog::Printf("Canceling Task %i at %.4f\n", m_iTaskID, xiiTime::Now().GetSeconds());
        return;
      }
    }

    m_bDone = true;

    if (m_iTaskID >= 0)
      xiiLog::Printf("Finishing Task %i at %.4f\n", m_iTaskID, xiiTime::Now().GetSeconds());
  }
};

class TaskCallbacks
{
public:
  void TaskFinished(const xiiSharedPtr<xiiTask>& pTask) { m_pInt->Increment(); }

  void TaskGroupFinished(xiiTaskGroupID id) { m_pInt->Increment(); }

  xiiAtomicInteger32* m_pInt;
};

XII_CREATE_SIMPLE_TEST(Threading, TaskSystem)
{
  xiiInt8 iWorkersShort = 4;
  xiiInt8 iWorkersLong  = 4;

  xiiTaskSystem::SetWorkerThreadCount(iWorkersShort, iWorkersLong);
  xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(500));

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Single Tasks")
  {
    xiiSharedPtr<xiiTestTask> t[3];

    t[0] = XII_DEFAULT_NEW(xiiTestTask);
    t[1] = XII_DEFAULT_NEW(xiiTestTask);
    t[2] = XII_DEFAULT_NEW(xiiTestTask);

    t[0]->ConfigureTask("Task 0", xiiTaskNesting::Never);
    t[1]->ConfigureTask("Task 1", xiiTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", xiiTaskNesting::Never);

    auto tg0 = xiiTaskSystem::StartSingleTask(t[0], xiiTaskPriority::LateThisFrame);
    auto tg1 = xiiTaskSystem::StartSingleTask(t[1], xiiTaskPriority::ThisFrame);
    auto tg2 = xiiTaskSystem::StartSingleTask(t[2], xiiTaskPriority::EarlyThisFrame);

    xiiTaskSystem::WaitForGroup(tg0);
    xiiTaskSystem::WaitForGroup(tg1);
    xiiTaskSystem::WaitForGroup(tg2);

    XII_TEST_BOOL(t[0]->IsDone());
    XII_TEST_BOOL(t[1]->IsDone());
    XII_TEST_BOOL(t[2]->IsDone());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Single Tasks with Dependencies")
  {
    xiiSharedPtr<xiiTestTask> t[4];

    t[0] = XII_DEFAULT_NEW(xiiTestTask);
    t[1] = XII_DEFAULT_NEW(xiiTestTask);
    t[2] = XII_DEFAULT_NEW(xiiTestTask);
    t[3] = XII_DEFAULT_NEW(xiiTestTask);

    xiiTaskGroupID g[4];

    t[0]->ConfigureTask("Task 0", xiiTaskNesting::Never);
    t[1]->ConfigureTask("Task 1", xiiTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", xiiTaskNesting::Never);
    t[3]->ConfigureTask("Task 3", xiiTaskNesting::Maybe);

    g[0] = xiiTaskSystem::StartSingleTask(t[0], xiiTaskPriority::LateThisFrame);
    g[1] = xiiTaskSystem::StartSingleTask(t[1], xiiTaskPriority::ThisFrame, g[0]);
    g[2] = xiiTaskSystem::StartSingleTask(t[2], xiiTaskPriority::EarlyThisFrame, g[1]);
    g[3] = xiiTaskSystem::StartSingleTask(t[3], xiiTaskPriority::EarlyThisFrame, g[0]);

    xiiTaskSystem::WaitForGroup(g[2]);
    xiiTaskSystem::WaitForGroup(g[3]);

    XII_TEST_BOOL(t[0]->IsDone());
    XII_TEST_BOOL(t[1]->IsDone());
    XII_TEST_BOOL(t[2]->IsDone());
    XII_TEST_BOOL(t[3]->IsDone());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Grouped Tasks / TaskFinished Callback / GroupFinished Callback")
  {
    xiiSharedPtr<xiiTestTask> t[8];

    xiiTaskGroupID     g[4];
    xiiAtomicInteger32 GroupsFinished;
    xiiAtomicInteger32 TasksFinished;

    TaskCallbacks callbackGroup;
    callbackGroup.m_pInt = &GroupsFinished;

    TaskCallbacks callbackTask;
    callbackTask.m_pInt = &TasksFinished;

    g[0] = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::ThisFrame, xiiMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[1] = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::ThisFrame, xiiMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[2] = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::ThisFrame, xiiMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[3] = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::ThisFrame, xiiMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));

    for (int i = 0; i < 4; ++i)
      XII_TEST_BOOL(!xiiTaskSystem::IsTaskGroupFinished(g[i]));

    xiiTaskSystem::AddTaskGroupDependency(g[1], g[0]);
    xiiTaskSystem::AddTaskGroupDependency(g[2], g[0]);
    xiiTaskSystem::AddTaskGroupDependency(g[3], g[1]);

    for (int i = 0; i < 8; ++i)
    {
      t[i] = XII_DEFAULT_NEW(xiiTestTask);
      t[i]->ConfigureTask("Test Task", xiiTaskNesting::Maybe, xiiMakeDelegate(&TaskCallbacks::TaskFinished, &callbackTask));
    }

    xiiTaskSystem::AddTaskToGroup(g[0], t[0]);
    xiiTaskSystem::AddTaskToGroup(g[1], t[1]);
    xiiTaskSystem::AddTaskToGroup(g[1], t[2]);
    xiiTaskSystem::AddTaskToGroup(g[2], t[3]);
    xiiTaskSystem::AddTaskToGroup(g[2], t[4]);
    xiiTaskSystem::AddTaskToGroup(g[2], t[5]);
    xiiTaskSystem::AddTaskToGroup(g[3], t[6]);
    xiiTaskSystem::AddTaskToGroup(g[3], t[7]);

    for (int i = 0; i < 8; ++i)
    {
      XII_TEST_BOOL(!t[i]->IsTaskFinished());
      XII_TEST_BOOL(!t[i]->IsDone());
    }

    // do a snapshot
    // we don't validate it, just make sure it doesn't crash
    xiiDGMLGraph graph;
    xiiTaskSystem::WriteStateSnapshotToDGML(graph);

    xiiTaskSystem::StartTaskGroup(g[3]);
    xiiTaskSystem::StartTaskGroup(g[2]);
    xiiTaskSystem::StartTaskGroup(g[1]);
    xiiTaskSystem::StartTaskGroup(g[0]);

    xiiTaskSystem::WaitForGroup(g[3]);
    xiiTaskSystem::WaitForGroup(g[2]);
    xiiTaskSystem::WaitForGroup(g[1]);
    xiiTaskSystem::WaitForGroup(g[0]);

    XII_TEST_INT(TasksFinished, 8);

    // It is not guaranteed that group finished callback is called after WaitForGroup returned so we need to wait a bit here.
    for (int i = 0; i < 10; i++)
    {
      if (GroupsFinished == 4)
      {
        break;
      }
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
    }
    XII_TEST_INT(GroupsFinished, 4);

    for (int i = 0; i < 4; ++i)
      XII_TEST_BOOL(xiiTaskSystem::IsTaskGroupFinished(g[i]));

    for (int i = 0; i < 8; ++i)
    {
      XII_TEST_BOOL(t[i]->IsTaskFinished());
      XII_TEST_BOOL(t[i]->IsDone());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "This Frame Tasks / Next Frame Tasks")
  {
    const xiiUInt32           uiNumTasks = 20;
    xiiSharedPtr<xiiTestTask> t[uiNumTasks];
    xiiTaskGroupID            tg[uiNumTasks];
    bool                      finished[uiNumTasks];

    for (xiiUInt32 i = 0; i < uiNumTasks; i += 2)
    {
      finished[i]     = false;
      finished[i + 1] = false;

      t[i]     = XII_DEFAULT_NEW(xiiTestTask);
      t[i + 1] = XII_DEFAULT_NEW(xiiTestTask);

      t[i]->m_uiIterations     = 10;
      t[i + 1]->m_uiIterations = 20;

      tg[i]     = xiiTaskSystem::StartSingleTask(t[i], xiiTaskPriority::ThisFrame);
      tg[i + 1] = xiiTaskSystem::StartSingleTask(t[i + 1], xiiTaskPriority::NextFrame);
    }

    // 'finish' the first frame
    xiiTaskSystem::FinishFrameTasks();

    {
      xiiUInt32 uiNotAllThisTasksFinished = 0;
      xiiUInt32 uiNotAllNextTasksFinished = 0;

      for (xiiUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          XII_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          XII_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // up to the number of worker threads tasks can still be active
      XII_TEST_BOOL(uiNotAllThisTasksFinished <= xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::ShortTasks));
      XII_TEST_BOOL(uiNotAllNextTasksFinished <= uiNumTasks);
    }


    // 'finish' the second frame
    xiiTaskSystem::FinishFrameTasks();

    {
      xiiUInt32 uiNotAllThisTasksFinished = 0;
      xiiUInt32 uiNotAllNextTasksFinished = 0;

      for (int i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          XII_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          XII_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      XII_TEST_BOOL(uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::ShortTasks));
    }

    // 'finish' all frames
    xiiTaskSystem::FinishFrameTasks();

    {
      xiiUInt32 uiNotAllThisTasksFinished = 0;
      xiiUInt32 uiNotAllNextTasksFinished = 0;

      for (xiiUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          XII_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          XII_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // even after finishing multiple frames, the previous frame tasks may still be in execution
      // since no N+x tasks enforce their completion in this test
      XII_TEST_BOOL(uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::ShortTasks));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Main Thread Tasks")
  {
    const xiiUInt32           uiNumTasks = 20;
    xiiSharedPtr<xiiTestTask> t[uiNumTasks];

    for (xiiUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t[i]                 = XII_DEFAULT_NEW(xiiTestTask);
      t[i]->m_uiIterations = 10;

      xiiTaskSystem::StartSingleTask(t[i], xiiTaskPriority::ThisFrameMainThread);
    }

    xiiTaskSystem::FinishFrameTasks();

    for (xiiUInt32 i = 0; i < uiNumTasks; ++i)
    {
      XII_TEST_BOOL(t[i]->IsTaskFinished());
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Canceling Tasks")
  {
    const xiiUInt32           uiNumTasks = 20;
    xiiSharedPtr<xiiTestTask> t[uiNumTasks];
    xiiTaskGroupID            tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i]                 = XII_DEFAULT_NEW(xiiTestTask);
      t[i]->m_uiIterations = 50;

      tg[i] = xiiTaskSystem::StartSingleTask(t[i], xiiTaskPriority::ThisFrame);
    }

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

    xiiUInt32 uiCanceled = 0;

    for (xiiUInt32 i0 = uiNumTasks; i0 > 0; --i0)
    {
      const xiiUInt32 i = i0 - 1;

      if (xiiTaskSystem::CancelTask(t[i], xiiOnTaskRunning::ReturnWithoutBlocking) == XII_SUCCESS)
        ++uiCanceled;
    }

    xiiUInt32 uiDone    = 0;
    xiiUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      xiiTaskSystem::WaitForGroup(tg[i]);
      XII_TEST_BOOL(t[i]->IsTaskFinished());

      if (t[i]->IsDone())
        ++uiDone;
      if (t[i]->IsStarted())
        ++uiStarted;
    }

    // at least one task should have run and thus be 'done'
    XII_TEST_BOOL(uiDone > 0);
    XII_TEST_BOOL(uiDone < uiNumTasks);

    XII_TEST_BOOL(uiStarted > 0);
    XII_TEST_BOOL_MSG(uiStarted <= xiiTaskSystem::GetNumAllocatedWorkerThreads(xiiWorkerThreadType::ShortTasks),
                      "This test can fail when the PC is under heavy load."); // should not have managed to start more tasks than there are threads
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Canceling Tasks (forcefully)")
  {
    const xiiUInt32           uiNumTasks = 20;
    xiiSharedPtr<xiiTestTask> t[uiNumTasks];
    xiiTaskGroupID            tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i]                   = XII_DEFAULT_NEW(xiiTestTask);
      t[i]->m_uiIterations   = 50;
      t[i]->m_bSupportCancel = true;

      tg[i] = xiiTaskSystem::StartSingleTask(t[i], xiiTaskPriority::ThisFrame);
    }

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

    xiiUInt32 uiCanceled = 0;

    for (int i = uiNumTasks - 1; i >= 0; --i)
    {
      if (xiiTaskSystem::CancelTask(t[i], xiiOnTaskRunning::ReturnWithoutBlocking) == XII_SUCCESS)
        ++uiCanceled;
    }

    xiiUInt32 uiDone    = 0;
    xiiUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      xiiTaskSystem::WaitForGroup(tg[i]);
      XII_TEST_BOOL(t[i]->IsTaskFinished());

      if (t[i]->IsDone())
        ++uiDone;
      if (t[i]->IsStarted())
        ++uiStarted;
    }

    // not a single thread should have finished the execution
    if (XII_TEST_BOOL_MSG(uiDone == 0, "This test can fail when the PC is under heavy load."))
    {
      XII_TEST_BOOL(uiStarted > 0);
      XII_TEST_BOOL(uiStarted <= xiiTaskSystem::GetNumAllocatedWorkerThreads(
                                   xiiWorkerThreadType::ShortTasks)); // should not have managed to start more tasks than there are threads
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Canceling Group")
  {
    const xiiUInt32           uiNumTasks = 4;
    xiiSharedPtr<xiiTestTask> t1[uiNumTasks];
    xiiSharedPtr<xiiTestTask> t2[uiNumTasks];

    xiiTaskGroupID g1, g2;
    g1 = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::ThisFrame);
    g2 = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::ThisFrame);

    xiiTaskSystem::AddTaskGroupDependency(g2, g1);

    for (xiiUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t1[i] = XII_DEFAULT_NEW(xiiTestTask);
      t2[i] = XII_DEFAULT_NEW(xiiTestTask);

      xiiTaskSystem::AddTaskToGroup(g1, t1[i]);
      xiiTaskSystem::AddTaskToGroup(g2, t2[i]);
    }

    xiiTaskSystem::StartTaskGroup(g2);
    xiiTaskSystem::StartTaskGroup(g1);

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));

    XII_TEST_BOOL(xiiTaskSystem::CancelGroup(g2, xiiOnTaskRunning::WaitTillFinished) == XII_SUCCESS);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      XII_TEST_BOOL(!t2[i]->IsDone());
      XII_TEST_BOOL(t2[i]->IsTaskFinished());
    }

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));

    XII_TEST_BOOL(xiiTaskSystem::CancelGroup(g1, xiiOnTaskRunning::WaitTillFinished) == XII_FAILURE);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      XII_TEST_BOOL(!t2[i]->IsDone());

      XII_TEST_BOOL(t1[i]->IsTaskFinished());
      XII_TEST_BOOL(t2[i]->IsTaskFinished());
    }

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(100));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Tasks with Multiplicity")
  {
    xiiSharedPtr<xiiTestTask> t[3];
    xiiTaskGroupID            tg[3];

    t[0] = XII_DEFAULT_NEW(xiiTestTask);
    t[1] = XII_DEFAULT_NEW(xiiTestTask);
    t[2] = XII_DEFAULT_NEW(xiiTestTask);

    t[0]->ConfigureTask("Task 0", xiiTaskNesting::Maybe);
    t[1]->ConfigureTask("Task 1", xiiTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", xiiTaskNesting::Never);

    t[0]->SetMultiplicity(1);
    t[1]->SetMultiplicity(100);
    t[2]->SetMultiplicity(1000);

    tg[0] = xiiTaskSystem::StartSingleTask(t[0], xiiTaskPriority::LateThisFrame);
    tg[1] = xiiTaskSystem::StartSingleTask(t[1], xiiTaskPriority::ThisFrame);
    tg[2] = xiiTaskSystem::StartSingleTask(t[2], xiiTaskPriority::EarlyThisFrame);

    xiiTaskSystem::WaitForGroup(tg[0]);
    xiiTaskSystem::WaitForGroup(tg[1]);
    xiiTaskSystem::WaitForGroup(tg[2]);

    XII_TEST_BOOL(t[0]->IsMultiplicityDone());
    XII_TEST_BOOL(t[1]->IsMultiplicityDone());
    XII_TEST_BOOL(t[2]->IsMultiplicityDone());
  }

  // capture profiling info for testing
  /*xiiStringBuilder sOutputPath = xiiTestFramework::GetInstance()->GetAbsOutputPath();

  xiiFileSystem::AddDataDirectory(sOutputPath.GetData());

  xiiFileWriter fileWriter;
  if (fileWriter.Open("profiling.json") == XII_SUCCESS)
  {
  xiiProfilingSystem::Capture(fileWriter);
  }*/
}
