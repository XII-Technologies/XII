
#include <Foundation/Time/Time.h>

// static
void* xiiPageAllocator::AllocatePage(size_t uiSize)
{
  xiiTime fAllocationTime = xiiTime::Now();

  void*          pPtr    = nullptr;
  size_t         uiAlign = xiiSystemInformation::Get().GetMemoryPageSize();
  const xiiInt32 iResult = posix_memalign(&pPtr, uiAlign, uiSize);
  XII_ASSERT_DEBUG(iResult == 0, "Failed to align pointer");
  XII_IGNORE_UNUSED(iResult);

  XII_CHECK_ALIGNMENT(pPtr, uiAlign);

  if constexpr (xiiAllocatorTrackingMode::Default >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::AddAllocation(xiiPageAllocator::GetId(), xiiAllocatorTrackingMode::Default, pPtr, uiSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return pPtr;
}

// static
void xiiPageAllocator::DeallocatePage(void* pPtr)
{
  if constexpr (xiiAllocatorTrackingMode::Default >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::RemoveAllocation(xiiPageAllocator::GetId(), pPtr);
  }

  free(pPtr);
}
