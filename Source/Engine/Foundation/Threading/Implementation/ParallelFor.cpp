#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/TaskSystem.h>

/// \brief This is a helper class that splits up task items via index ranges.
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
void ParallelForIndexedInternal(IndexType uiStartIndex, IndexType uiNumItems, const Callback& taskCallback, const char* taskName, const xiiParallelForParams& params)
{
  typedef IndexedTask<IndexType, Callback> Task;

  if (!taskName)
  {
    taskName = "Generic Indexed Task";
  }

  const xiiUInt32 uiMultiplicity       = params.DetermineMultiplicity(uiNumItems);
  const IndexType uiItemsPerInvocation = params.DetermineItemsPerInvocation(uiNumItems, uiMultiplicity);

  if (uiMultiplicity == 0)
  {
    Task indexedTask(uiStartIndex, uiNumItems, std::move(taskCallback), uiItemsPerInvocation);
    indexedTask.ConfigureTask(taskName, xiiTaskNesting::Never);

    XII_PROFILE_SCOPE(taskName);
    indexedTask.Execute();
  }
  else
  {
    xiiAllocatorBase* pAllocator = (params.pTaskAllocator != nullptr) ? params.pTaskAllocator : xiiFoundation::GetDefaultAllocator();

    xiiSharedPtr<Task> pIndexedTask = XII_NEW(pAllocator, Task, uiStartIndex, uiNumItems, std::move(taskCallback), uiItemsPerInvocation);
    pIndexedTask->ConfigureTask(taskName, xiiTaskNesting::Never);

    pIndexedTask->SetMultiplicity(uiMultiplicity);
    xiiTaskGroupID taskGroupId = xiiTaskSystem::StartSingleTask(pIndexedTask, xiiTaskPriority::EarlyThisFrame);
    xiiTaskSystem::WaitForGroup(taskGroupId);
  }
}

xiiUInt32 xiiParallelForParams::DetermineMultiplicity(xiiUInt64 uiNumTaskItems) const
{
  // If we have not exceeded the threading threshold we will indicate to use serial execution.
  if (uiNumTaskItems < uiBinSize)
  {
    return 0;
  }

  const xiiUInt32 uiNumWorkers = xiiTaskSystem::GetWorkerThreadCount(xiiWorkerThreadType::ShortTasks);
  // The slice size gives the number of items that can be processed when giving exactly uiBinSize
  // task items to each worker.
  const xiiUInt32 uiSliceSize = uiNumWorkers * uiBinSize;

  // Needing at most #_workers threads.
  if (uiNumTaskItems <= uiSliceSize)
  {
    // Fill up each thread with at most uiBinSize task items.
    const xiiUInt64 numThreads = (uiNumTaskItems + uiBinSize - 1) / uiBinSize;
    XII_ASSERT_DEV(numThreads <= uiNumWorkers, "");
    XII_ASSERT_DEV(numThreads < (1ull << 32), "");
    return (xiiUInt32)numThreads;
  }
  // Needing at most #_workers * threading_factor threads.
  else if (uiNumTaskItems <= uiSliceSize * uiMaxTasksPerThread)
  {
    const xiiUInt64 uiNumSlices = (uiNumTaskItems + uiSliceSize - 1) / uiSliceSize;
    XII_ASSERT_DEV(uiNumSlices <= uiMaxTasksPerThread, "");
    const xiiUInt64 result = uiNumSlices * uiNumWorkers;
    XII_ASSERT_DEV(result < (1ull << 32), "");
    return (xiiUInt32)result;
  }
  // Needing more than #_workers * threading_factor threads --> clamp to that number.
  else
  {
    const xiiUInt64 result = uiNumWorkers * uiMaxTasksPerThread;
    XII_ASSERT_DEV(result < (1ull << 32), "");
    return (xiiUInt32)result;
  }
}

xiiUInt64 xiiParallelForParams::DetermineItemsPerInvocation(xiiUInt64 uiNumTaskItems, xiiUInt32 uiMultiplicity) const
{
  if (uiMultiplicity == 0)
  {
    return uiNumTaskItems;
  }

  const xiiUInt64 uiItemsPerInvocation = (uiNumTaskItems + uiMultiplicity - 1) / uiMultiplicity;
  return uiItemsPerInvocation;
}

xiiUInt32 xiiParallelForParams::DetermineItemsPerInvocation(xiiUInt32 uiNumTaskItems, xiiUInt32 uiMultiplicity) const
{
  const xiiUInt64 result = DetermineItemsPerInvocation(xiiUInt64(uiNumTaskItems), uiMultiplicity);
  XII_ASSERT_DEV(result < (1ull << 32), "");
  return (xiiUInt32)result;
}

void xiiTaskSystem::ParallelForIndexed(
  xiiUInt32                       uiStartIndex,
  xiiUInt32                       uiNumItems,
  xiiParallelForIndexedFunction32 taskCallback,
  const char*                     taskName,
  const xiiParallelForParams&     params)
{
  ParallelForIndexedInternal<xiiUInt32, xiiParallelForIndexedFunction32>(uiStartIndex, uiNumItems, taskCallback, taskName, params);
}

void xiiTaskSystem::ParallelForIndexed(
  xiiUInt64                       uiStartIndex,
  xiiUInt64                       uiNumItems,
  xiiParallelForIndexedFunction64 taskCallback,
  const char*                     taskName,
  const xiiParallelForParams&     params)
{
  ParallelForIndexedInternal<xiiUInt64, xiiParallelForIndexedFunction64>(uiStartIndex, uiNumItems, taskCallback, taskName, params);
}

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ParallelFor);
