#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>

xiiAllocatorId xiiPageAllocator::GetId()
{
  static xiiAllocatorId id;

  if (id.IsInvalidated())
  {
    id = xiiMemoryTracker::RegisterAllocator("Page", xiiAllocatorTrackingMode::Default, xiiAllocatorId());
  }

  return id;
}
