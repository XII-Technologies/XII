#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Win/Platform_win.h>
#include <Foundation/Time/Time.h>

// static
void* xiiPageAllocator::AllocatePage(size_t uiSize)
{
  xiiTime fAllocationTime = xiiTime::Now();

  void* ptr = ::VirtualAlloc(nullptr, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  XII_ASSERT_DEV(ptr != nullptr, "Could not allocate memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));

  size_t uiAlign = xiiSystemInformation::Get().GetMemoryPageSize();
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

  XII_VERIFY(::VirtualFree(ptr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));
}
