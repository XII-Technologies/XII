/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Time.h>

// static
void* xiiPageAllocator::AllocatePage(size_t uiSize)
{
  xiiTime fAllocationTime = xiiTime::Now();

  void* pPtr = ::VirtualAlloc(nullptr, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  XII_ASSERT_DEV(pPtr != nullptr, "Could not allocate memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));

  size_t uiAlign = xiiSystemInformation::Get().GetMemoryPageSize();
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

  XII_VERIFY(::VirtualFree(pPtr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '{0}'", xiiArgErrorCode(::GetLastError()));
}
