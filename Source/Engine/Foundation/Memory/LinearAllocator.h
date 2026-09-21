/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/AllocatorWithPolicy.h>
#include <Foundation/Memory/Policies/AllocationPolicyLinear.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// Linear allocator that allocates memory sequentially.
///
/// This allocator allocates memory by simply advancing a pointer within pre-allocated chunks.
/// Individual deallocations are not supported - the entire allocator must be reset at once.
/// Very efficient for short lived allocations.
///
/// Template parameters:
/// - TrackingMode: Controls allocation tracking and debugging features
/// - OverwriteMemoryOnReset: If true, fills memory with debug pattern on reset (debug builds only)
///
/// Performance characteristics:
/// - Allocation: O(1) - just pointer arithmetic
/// - Deallocation: Not supported individually
/// - Reset: O(1) - just resets pointer
/// - Memory overhead: Minimal
///
/// Use when:
/// - Temporary allocations with predictable lifetime
/// - Stack-like allocation pattern
/// - High allocation frequency (parsing, temporary buffers)
/// - Frame-based or scope-based memory management
template <xiiAllocatorTrackingMode TrackingMode = xiiAllocatorTrackingMode::Default, bool OverwriteMemoryOnReset = false>
class xiiLinearAllocator : public xiiAllocatorWithPolicy<xiiAllocationPolicyLinear<OverwriteMemoryOnReset>, TrackingMode>
{
  using SUPER        = xiiAllocatorWithPolicy<xiiAllocationPolicyLinear<OverwriteMemoryOnReset>, TrackingMode>;
  using PolicyLinear = xiiAllocationPolicyLinear<OverwriteMemoryOnReset>;

public:
  xiiLinearAllocator(xiiStringView sName, xiiAllocator* pParent, xiiUInt32 uiInitialSize);
  ~xiiLinearAllocator();

  virtual void* Allocate(size_t uiSize, size_t uiAlign, xiiMemoryUtils::DestructorFunction destructorFunc) override;
  virtual void  Deallocate(void* pPtr) override;

  /// Resets the allocator, freeing all memory and calling destructors.
  ///
  /// This resets the allocation pointer to the beginning and calls destructors for all
  /// objects that were allocated with destructor functions. After reset, all previously
  /// allocated pointers become invalid and must not be used.
  void Reset();

private:
  struct DestructData
  {
    XII_DECLARE_POD_TYPE();

    xiiMemoryUtils::DestructorFunction m_Func;
    void*                              m_Ptr;
  };

  xiiMutex                       m_Mutex;
  xiiDynamicArray<DestructData>  m_DestructData;
  xiiHashTable<void*, xiiUInt32> m_PtrToDestructDataIndexTable;
};

#include <Foundation/Memory/Implementation/LinearAllocator_inl.h>
