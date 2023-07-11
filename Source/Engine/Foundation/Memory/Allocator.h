#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorBase.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

XII_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, xiiHasReallocate);

#include <Foundation/Memory/Implementation/Allocator_inl.h>

/// \brief Policy based allocator implementation of the xiiAllocatorBase interface.
///
/// AllocationPolicy defines how the actual memory is allocated.\n
/// TrackingFlags defines how stats about allocations are tracked.\n
template <typename AllocationPolicy, xiiUInt32 TrackingFlags = xiiMemoryTrackingFlags::Default>
class xiiAllocator : public xiiInternal::xiiAllocatorMixinReallocate<AllocationPolicy, TrackingFlags, xiiHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>
{
public:
  xiiAllocator(xiiStringView sName, xiiAllocatorBase* pParent = nullptr) :
    xiiInternal::xiiAllocatorMixinReallocate<AllocationPolicy, TrackingFlags, xiiHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>(sName, pParent)
  {
  }
};
