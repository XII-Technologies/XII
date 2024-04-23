#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>

static xiiAllocatorId GetPageAllocatorId()
{
  static xiiAllocatorId id;

  if (id.IsInvalidated())
  {
    id = xiiMemoryTracker::RegisterAllocator("Page", xiiAllocatorTrackingMode::Default, xiiAllocatorId());
  }

  return id;
}

xiiAllocatorId xiiPageAllocator::GetId()
{
  return GetPageAllocatorId();
}

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Implementation/Win/PageAllocator_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Memory/Implementation/Posix/PageAllocator_posix.h>
#else
#  error "xiiPageAllocator is not implemented on current platform"
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_PageAllocator);
