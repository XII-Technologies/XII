/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/SharedPtr.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystemWithBarrier.h>

/// Jolt owns the job graph and barrier. XII's short-task workers execute its ready jobs.
class xiiJoltJobSystem final : public JPH::JobSystemWithBarrier
{
public:
  xiiJoltJobSystem();

  /// Waits for XII to release the queued tasks after the last physics update has returned.
  void DrainTasks();

  int GetMaxConcurrency() const override;

  JPH::JobHandle CreateJob(const char* szName, JPH::ColorArg color, const JobFunction& function, JPH::uint32 uiDependencies) override;
  void           CreateJobs(const char* szName, JPH::ColorArg color, const JobFunction& function, JPH::JobHandle* pJobs, JPH::uint32 uiCount, JPH::uint32 uiDependencies) override;

private:
  class JobTask final : public xiiTask
  {
  public:
    JobTask();

    Job* m_pJob = nullptr;

  private:
    void Execute() override;
  };

  void FreeJob(Job* pJob) override;
  void QueueJob(Job* pJob) override;
  void QueueJobs(Job** pJobs, JPH::uint uiCount) override;

  JPH::FixedSizeFreeList<Job>            m_Jobs;
  xiiDynamicArray<xiiSharedPtr<JobTask>> m_Tasks;
  const bool                             m_bUseTaskSystem;
};
