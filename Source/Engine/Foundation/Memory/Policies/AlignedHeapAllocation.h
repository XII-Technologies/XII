#pragma once

#include <Foundation/Basics.h>

namespace xiiMemoryPolicies
{
  /// \brief Aligned Heap memory allocation policy.
  ///
  /// \see xiiAllocator
  class xiiAlignedHeapAllocation
  {
  public:
    XII_ALWAYS_INLINE xiiAlignedHeapAllocation(xiiAllocatorBase* pParent) { XII_IGNORE_UNUSED(pParent); }
    XII_ALWAYS_INLINE ~xiiAlignedHeapAllocation() = default;

    void* Allocate(size_t uiSize, size_t uiAlign);
    void  Deallocate(void* pPtr);

    XII_ALWAYS_INLINE xiiAllocatorBase* GetParent() const { return nullptr; }
  };

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/AlignedHeapAllocation_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Platform/Implementation/Posix/AlignedHeapAllocation_posix.h>
#else
#  error "xiiAlignedHeapAllocation is not implemented on current platform"
#endif
} // namespace xiiMemoryPolicies
