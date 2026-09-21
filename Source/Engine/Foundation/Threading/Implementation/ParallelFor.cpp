/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/TaskSystem.h>

/// This is a helper class that splits up task items via index ranges.
template <typename IndexType, typename Callback>
class IndexedTask final : public xiiTask
{
public:
  IndexedTask(IndexType uiStartIndex, IndexType uiNumItems, Callback taskCallback, IndexType uiItemsPerInvocation) :
    m_uiStartIndex(uiStartIndex), m_uiNumItems(uiNumItems), m_uiItemsPerInvocation(uiItemsPerInvocation), m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(m_uiStartIndex, m_uiStartIndex + m_uiNumItems);
  }

  void ExecuteWithMultiplicity(xiiUInt32 uiInvocation) const override
  {
    const IndexType uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;
    const IndexType uiSliceEndIndex   = xiiMath::Min(uiSliceStartIndex + m_uiItemsPerInvocation, m_uiStartIndex + m_uiNumItems);

    XII_ASSERT_DEV(uiSliceStartIndex < uiSliceEndIndex, "ParallelFor start/end indices given to index task are invalid: {} -> {}", uiSliceStartIndex, uiSliceEndIndex);

    // Run through the calculated slice, the end index is exclusive, i.e., should not be handled by this instance.
    m_TaskCallback(uiSliceStartIndex, uiSliceEndIndex);
  }

private:
  IndexType m_uiStartIndex;
  IndexType m_uiNumItems;
  IndexType m_uiItemsPerInvocation;
  Callback  m_TaskCallback;
};

template <typename IndexType, typename Callback>
void ParallelForIndexedInternal(IndexType uiStartIndex, IndexType uiNumItems, const Callback&& taskCallback, xiiStringView sTaskName, const xiiParallelForParams& params, xiiTaskNesting taskNesting)
{
  using Task = IndexedTask<IndexType, Callback>;

  if (sTaskName.IsEmpty())
  {
    sTaskName = "Generic Indexed Task";
  }

  if (uiNumItems <= params.m_uiBinSize)
  {
    // If we have not exceeded the threading threshold we use serial execution

    Task indexedTask(uiStartIndex, uiNumItems, std::move(taskCallback), uiNumItems);
    indexedTask.ConfigureTask(sTaskName, taskNesting);

    XII_PROFILE_SCOPE(sTaskName);
    indexedTask.Execute();
  }
  else
  {
    xiiUInt32 uiMultiplicity;
    xiiUInt64 uiItemsPerInvocation;
    params.DetermineThreading(uiNumItems, uiMultiplicity, uiItemsPerInvocation);

    xiiAllocator* pAllocator = (params.m_pTaskAllocator != nullptr) ? params.m_pTaskAllocator : xiiFoundation::GetDefaultAllocator();

    xiiSharedPtr<Task> pIndexedTask = XII_NEW(pAllocator, Task, uiStartIndex, uiNumItems, std::move(taskCallback), static_cast<IndexType>(uiItemsPerInvocation));
    pIndexedTask->ConfigureTask(sTaskName, taskNesting);

    pIndexedTask->SetMultiplicity(uiMultiplicity);
    xiiTaskGroupID taskGroupId = xiiTaskSystem::StartSingleTask(pIndexedTask, xiiTaskPriority::EarlyThisFrame);
    xiiTaskSystem::WaitForGroup(taskGroupId);
  }
}

void xiiParallelForParams::DetermineThreading(xiiUInt64 uiNumItemsToExecute, xiiUInt32& out_uiNumTasksToRun, xiiUInt64& out_uiNumItemsPerTask) const
{
  // We create a single task, but we set it's multiplicity to M (= out_uiNumTasksToRun)
  // so that it gets scheduled M times, which is effectively the same as creating M tasks

  const xiiUInt32 uiNumWorkerThreads      = xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::ShortTasks);
  const xiiUInt64 uiMaxTasksToUse         = uiNumWorkerThreads * m_uiMaxTasksPerThread;
  const xiiUInt64 uiMaxExecutionsRequired = xiiMath::Max(1llu, uiNumItemsToExecute / m_uiBinSize);

  if (uiMaxExecutionsRequired >= uiMaxTasksToUse)
  {
    // If we have more items to execute, than the upper limit of tasks that we want to spawn, clamp the number of tasks
    // and give each task more items to do
    out_uiNumTasksToRun = uiMaxTasksToUse & 0xFFFFFFFF;
  }
  else
  {
    // If we want to execute fewer items than we have tasks available, just run exactly as many tasks as we have items
    out_uiNumTasksToRun = uiMaxExecutionsRequired & 0xFFFFFFFF;
  }

  // Now that we determined the number of tasks to run, compute how much each task should do
  out_uiNumItemsPerTask = uiNumItemsToExecute / out_uiNumTasksToRun;

  // Due to rounding down in the line above, it can happen that we would execute too few tasks
  if (out_uiNumItemsPerTask * out_uiNumTasksToRun < uiNumItemsToExecute)
  {
    // To fix this, either do one more task invocation, or one more item per task
    if (out_uiNumItemsPerTask * (out_uiNumTasksToRun + 1) >= uiNumItemsToExecute)
    {
      ++out_uiNumTasksToRun;

      // Though with one more task we may execute too many items, so if possible reduce the number of items that each task executes
      while ((out_uiNumItemsPerTask - 1) * out_uiNumTasksToRun >= uiNumItemsToExecute)
      {
        --out_uiNumItemsPerTask;
      }
    }
    else
    {
      ++out_uiNumItemsPerTask;

      // Though if every task executes one more item, we may execute too many items, so if possible reduce the number of tasks again
      while (out_uiNumItemsPerTask * (out_uiNumTasksToRun - 1) >= uiNumItemsToExecute)
      {
        --out_uiNumTasksToRun;
      }
    }

    XII_ASSERT_DEV(out_uiNumItemsPerTask * out_uiNumTasksToRun >= uiNumItemsToExecute, "xiiParallelFor is missing invocations");
  }
}

void xiiTaskSystem::ParallelForIndexed(xiiUInt32 uiStartIndex, xiiUInt32 uiNumItems, xiiParallelForIndexedFunction32 taskCallback, xiiStringView sTaskName, xiiTaskNesting taskNesting, const xiiParallelForParams& params)
{
  ParallelForIndexedInternal<xiiUInt32, xiiParallelForIndexedFunction32>(uiStartIndex, uiNumItems, std::move(taskCallback), sTaskName, params, taskNesting);
}

void xiiTaskSystem::ParallelForIndexed(xiiUInt64 uiStartIndex, xiiUInt64 uiNumItems, xiiParallelForIndexedFunction64 taskCallback, xiiStringView sTaskName, xiiTaskNesting taskNesting, const xiiParallelForParams& params)
{
  ParallelForIndexedInternal<xiiUInt64, xiiParallelForIndexedFunction64>(uiStartIndex, uiNumItems, std::move(taskCallback), sTaskName, params, taskNesting);
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ParallelFor);
