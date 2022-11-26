
#include <Foundation/Time/Time.h>

// static
void* xiiPageAllocator::AllocatePage(size_t uiSize)
{
  xiiTime fAllocationTime = xiiTime::Now();

  void*     ptr     = nullptr;
  size_t    uiAlign = xiiSystemInformation::Get().GetMemoryPageSize();
  const int res     = posix_memalign(&ptr, uiAlign, uiSize);
  XII_ASSERT_DEBUG(res == 0, "Failed to align pointer");
  XII_IGNORE_UNUSED(res);

  XII_CHECK_ALIGNMENT(ptr, uiAlign);

  if ((xiiMemoryTrackingFlags::Default & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::AddAllocation(GetPageAllocatorId(), xiiMemoryTrackingFlags::Default, ptr, uiSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return ptr;
}

// static
void xiiPageAllocator::DeallocatePage(void* ptr)
{
  if ((xiiMemoryTrackingFlags::Default & xiiMemoryTrackingFlags::EnableAllocationTracking) != 0)
  {
    xiiMemoryTracker::RemoveAllocation(GetPageAllocatorId(), ptr);
  }

  free(ptr);
}
