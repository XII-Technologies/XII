/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <JoltPlugin/System/Implementation/JoltJobSystem.h>

#include <Jolt/Core/FPFlushDenormals.h>
#include <Jolt/Physics/PhysicsSettings.h>

xiiJoltJobSystem::xiiJoltJobSystem() :
  JPH::JobSystemWithBarrier(JPH::cMaxPhysicsBarriers),
  m_bUseTaskSystem(!xiiStartup::HasApplicationTag("NoTaskSystem"))
{
  m_Jobs.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsJobs);
  if (m_bUseTaskSystem)
  {
    m_Tasks.SetCount(JPH::cMaxPhysicsJobs);
    for (auto& pTask : m_Tasks)
    {
      pTask = XII_DEFAULT_NEW(JobTask);
    }
  }
}

void xiiJoltJobSystem::DrainTasks()
{
  // A Jolt barrier can execute a job before its XII task runs. Wait until the
  // scheduler has released every task before destroying the job pool or unloading.
  for (const auto& pTask : m_Tasks)
  {
    while (pTask->GetRefCount() > 1)
    {
      xiiThreadUtils::YieldTimeSlice();
    }
  }
}

int xiiJoltJobSystem::GetMaxConcurrency() const
{
  return m_bUseTaskSystem ? 1 + static_cast<int>(xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::ShortTasks)) : 1;
}

JPH::JobHandle xiiJoltJobSystem::CreateJob(const char* szName, JPH::ColorArg color, const JobFunction& function, JPH::uint32 uiDependencies)
{
  auto uiIndex = m_Jobs.ConstructObject(szName, color, this, function, uiDependencies);
  while (uiIndex == decltype(m_Jobs)::cInvalidObjectIndex)
  {
    xiiThreadUtils::YieldTimeSlice();
    uiIndex = m_Jobs.ConstructObject(szName, color, this, function, uiDependencies);
  }

  Job*           pJob = &m_Jobs.Get(uiIndex);
  JPH::JobHandle job(pJob);
  if (uiDependencies == 0)
  {
    QueueJob(pJob);
  }
  return job;
}

void xiiJoltJobSystem::CreateJobs(const char* szName, JPH::ColorArg color, const JobFunction& function, JPH::JobHandle* pJobs, JPH::uint32 uiCount, JPH::uint32 uiDependencies)
{
  for (JPH::uint32 i = 0; i < uiCount; ++i)
  {
    pJobs[i] = CreateJob(szName, color, function, uiDependencies);
  }
}

void xiiJoltJobSystem::FreeJob(Job* pJob)
{
  m_Jobs.DestructObject(pJob);
}

void xiiJoltJobSystem::QueueJob(Job* pJob)
{
  if (!m_bUseTaskSystem)
    return; // The calling thread executes ready jobs while waiting on the barrier.

  const auto& pTask = m_Tasks[m_Jobs.GetObjectIndex(pJob)];
  pJob->AddRef();
  pTask->m_pJob = pJob;
  xiiTaskSystem::StartSingleTask(pTask, xiiTaskPriority::EarlyThisFrame);
}

void xiiJoltJobSystem::QueueJobs(Job** pJobs, JPH::uint uiCount)
{
  for (JPH::uint i = 0; i < uiCount; ++i)
  {
    QueueJob(pJobs[i]);
  }
}

xiiJoltJobSystem::JobTask::JobTask()
{
  ConfigureTask("Jolt Physics", xiiTaskNesting::Never, [](const xiiSharedPtr<xiiTask>& pTask) {
    auto* pJobTask   = static_cast<JobTask*>(pTask.Borrow());
    Job*  pJob       = pJobTask->m_pJob;
    pJobTask->m_pJob = nullptr;

    // Keep the pool slot reserved until XII marks the task finished and it can be reused.
    pJob->Release();
  });
}

void xiiJoltJobSystem::JobTask::Execute()
{
  [[maybe_unused]] const JPH::FPFlushDenormals flushDenormals;
  m_pJob->Execute();
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_Implementation_JoltJobSystem);
