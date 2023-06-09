#pragma once

#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

template <typename ElemType>
class ArrayPtrTask final : public xiiTask
{
public:
  ArrayPtrTask(xiiArrayPtr<ElemType> payload, xiiParallelForFunction<ElemType> taskCallback, xiiUInt32 uiItemsPerInvocation) :
    m_Payload(payload), m_uiItemsPerInvocation(uiItemsPerInvocation), m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(0, m_Payload);
  }

  void ExecuteWithMultiplicity(xiiUInt32 uiInvocation) const override
  {
    const xiiUInt32 uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;

    const xiiUInt32 uiRemainingItems = uiSliceStartIndex > m_Payload.GetCount() ? 0 : m_Payload.GetCount() - uiSliceStartIndex;
    const xiiUInt32 uiSliceItemCount = xiiMath::Min(m_uiItemsPerInvocation, uiRemainingItems);

    if (uiSliceItemCount > 0)
    {
      // Run through the calculated slice.
      auto taskItemSlice = m_Payload.GetSubArray(uiSliceStartIndex, uiSliceItemCount);
      m_TaskCallback(uiSliceStartIndex, taskItemSlice);
    }
  }

private:
  xiiArrayPtr<ElemType>            m_Payload;
  xiiUInt32                        m_uiItemsPerInvocation;
  xiiParallelForFunction<ElemType> m_TaskCallback;
};

template <typename ElemType>
void xiiTaskSystem::ParallelForInternal(xiiArrayPtr<ElemType> taskItems, xiiParallelForFunction<ElemType> taskCallback, const char* taskName, const xiiParallelForParams& params)
{
  if (taskItems.GetCount() <= params.m_uiBinSize)
  {
    ArrayPtrTask<ElemType> arrayPtrTask(taskItems, std::move(taskCallback), taskItems.GetCount());
    arrayPtrTask.ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    XII_PROFILE_SCOPE(arrayPtrTask.m_sTaskName);
    arrayPtrTask.Execute();
  }
  else
  {
    xiiUInt32 uiMultiplicity;
    xiiUInt64 uiItemsPerInvocation;
    params.DetermineThreading(taskItems.GetCount(), uiMultiplicity, uiItemsPerInvocation);

    xiiAllocatorBase* pAllocator = (params.m_pTaskAllocator != nullptr) ? params.m_pTaskAllocator : xiiFoundation::GetDefaultAllocator();

    xiiSharedPtr<ArrayPtrTask<ElemType>> pArrayPtrTask = XII_NEW(pAllocator, ArrayPtrTask<ElemType>, taskItems, std::move(taskCallback), static_cast<xiiUInt32>(uiItemsPerInvocation));
    pArrayPtrTask->ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    pArrayPtrTask->SetMultiplicity(uiMultiplicity);
    xiiTaskGroupID taskGroupId = xiiTaskSystem::StartSingleTask(pArrayPtrTask, xiiTaskPriority::EarlyThisFrame);
    xiiTaskSystem::WaitForGroup(taskGroupId);
  }
}

template <typename ElemType, typename Callback>
void xiiTaskSystem::ParallelFor(xiiArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const xiiParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](
                           xiiUInt32 /*uiBaseIndex*/, xiiArrayPtr<ElemType> taskSlice) {
    taskCallback(taskSlice);
  };

  ParallelForInternal<ElemType>(
    taskItems, xiiParallelForFunction<ElemType>(std::move(wrappedCallback), xiiFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void xiiTaskSystem::ParallelForSingle(xiiArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const xiiParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](xiiUInt32 /*uiBaseIndex*/, xiiArrayPtr<ElemType> taskSlice) {
    // Handing in by non-const& allows to use callbacks with (non-)const& as well as value parameters.
    for (ElemType& taskItem : taskSlice)
    {
      taskCallback(taskItem);
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, xiiParallelForFunction<ElemType>(std::move(wrappedCallback), xiiFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void xiiTaskSystem::ParallelForSingleIndex(
  xiiArrayPtr<ElemType>       taskItems,
  Callback                    taskCallback,
  const char*                 szTaskName,
  const xiiParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](xiiUInt32 uiBaseIndex, xiiArrayPtr<ElemType> taskSlice) {
    for (xiiUInt32 uiIndex = 0; uiIndex < taskSlice.GetCount(); ++uiIndex)
    {
      // Handing in by dereferenced pointer allows to use callbacks with (non-)const& as well as value parameters.
      taskCallback(uiBaseIndex + uiIndex, *(taskSlice.GetPtr() + uiIndex));
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, xiiParallelForFunction<ElemType>(std::move(wrappedCallback), xiiFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}
