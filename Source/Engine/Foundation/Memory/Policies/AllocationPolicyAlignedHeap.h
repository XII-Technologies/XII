/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>

/// Aligned Heap memory allocation policy.
///
/// \see xiiAllocatorWithPolicy
class xiiAllocationPolicyAlignedHeap
{
public:
  XII_ALWAYS_INLINE xiiAllocationPolicyAlignedHeap(xiiAllocator* pParent) { XII_IGNORE_UNUSED(pParent); }
  XII_ALWAYS_INLINE ~xiiAllocationPolicyAlignedHeap() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void  Deallocate(void* pPtr);

  XII_ALWAYS_INLINE xiiAllocator* GetParent() const { return nullptr; }
};

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/AllocationPolicyAlignedHeap_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/AllocationPolicyAlignedHeap_posix.h>
#else
#  error "xiiAllocationPolicyAlignedHeap is not implemented on current platform."
#endif
