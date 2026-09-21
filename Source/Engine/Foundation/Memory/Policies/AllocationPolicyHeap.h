/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// Default heap memory allocation policy.
///
/// \see xiiAllocatorWithPolicy
class xiiAllocationPolicyHeap
{
public:
  XII_ALWAYS_INLINE xiiAllocationPolicyHeap(xiiAllocator* pParent) { XII_IGNORE_UNUSED(pParent); }
  XII_ALWAYS_INLINE ~xiiAllocationPolicyHeap() = default;

  XII_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    XII_IGNORE_UNUSED(uiAlign);

    // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
    // if these asserts fail, you need to check what container made the allocation and change it
    // to use an aligned allocator, e.g. xiiAlignedAllocatorWrapper

    // unfortunately using XII_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
    // alignment interestingly, the code that does that, seems to work fine anyway
    XII_ASSERT_DEBUG(uiAlign <= 8, "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

    void* ptr = malloc(uiSize);
    XII_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  XII_FORCE_INLINE void* Reallocate(void* pCurrentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    XII_IGNORE_UNUSED(uiCurrentSize);
    XII_IGNORE_UNUSED(uiAlign);

    void* ptr = realloc(pCurrentPtr, uiNewSize);
    XII_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  XII_ALWAYS_INLINE void Deallocate(void* pPtr)
  {
    free(pPtr);
  }

  XII_ALWAYS_INLINE xiiAllocator* GetParent() const { return nullptr; }
};
