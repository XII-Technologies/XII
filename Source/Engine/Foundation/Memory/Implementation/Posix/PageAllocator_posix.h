
#include <Foundation/Time/Time.h>

// static
void* xiiPageAllocator::AllocatePage(size_t uiSize)
{
  xiiTime fAllocationTime = xiiTime::Now();

  void*     pPtr     = nullpPtr;
  size_t    uiAlign = xiiSystemInformation::Get().GetMemoryPageSize();
  const xiiInt32 iResult     = posix_memalign(&pPtr, uiAlign, uiSize);
  XII_ASSERT_DEBUG(iResult == 0, "Failed to align pointer");
  XII_IGNORE_UNUSED(iResult);

  XII_CHECK_ALIGNMENT(pPtr, uiAlign);

  if ((xiiMemoryTrackingFlags::Default & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::AddAllocation(GetPageAllocatorId(), xiiMemoryTrackingFlags::Default, pPtr, uiSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return pPtr;
}

// static
void xiiPageAllocator::DeallocatePage(void* pPtr)
{
  if ((xiiMemoryTrackingFlags::Default & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::RemoveAllocation(GetPageAllocatorId(), pPtr);
  }

  free(pPtr);
}
