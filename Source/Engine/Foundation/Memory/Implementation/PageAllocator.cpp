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

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/PageAllocator_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/PageAllocator_posix.h>
#else
#  error "xiiPageAllocator is not implemented on current platform"
#endif
