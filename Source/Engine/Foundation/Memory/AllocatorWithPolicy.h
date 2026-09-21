/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

XII_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, xiiHasReallocate);

#include <Foundation/Memory/Implementation/AllocatorMixin_inl.h>

/// Policy-based allocator that combines allocation strategies with tracking modes.
///
/// AllocationPolicy defines how the actual memory is allocated.
/// TrackingFlags defines how stats about allocations are tracked.
template <typename AllocationPolicy, xiiAllocatorTrackingMode TrackingMode = xiiAllocatorTrackingMode::Default>
class xiiAllocatorWithPolicy : public xiiInternal::xiiAllocatorMixinReallocate<AllocationPolicy, TrackingMode, xiiHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>
{
public:
  xiiAllocatorWithPolicy(xiiStringView sName, xiiAllocator* pParent = nullptr) :
    xiiInternal::xiiAllocatorMixinReallocate<AllocationPolicy, TrackingMode, xiiHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>(sName, pParent)
  {
  }
};
