/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadUtils.h>

/// Represents a typed block of memory with fixed size, typically used for bulk allocations.
///
/// This wrapper provides type-safe access to a block of memory that can hold multiple elements
/// of type T. The block has a fixed capacity determined by SizeInBytes and sizeof(T).
/// It tracks the current count of used elements and provides stack-like operations for
/// efficient allocation/deallocation within the block.
template <typename T, xiiUInt32 SizeInBytes>
struct xiiDataBlock
{
  XII_DECLARE_POD_TYPE();

  enum
  {
    SIZE_IN_BYTES = SizeInBytes,
    CAPACITY      = SIZE_IN_BYTES / sizeof(T)
  };

  /// Constructs a data block wrapping the given memory region.
  xiiDataBlock(T* pData, xiiUInt32 uiCount);

  /// Reserves space for one element at the end of the block.
  ///
  /// Returns pointer to the reserved element, or nullptr if the block is full.
  T* ReserveBack();

  /// Removes and returns pointer to the last element in the block.
  ///
  /// Returns nullptr if the block is empty.
  T* PopBack();

  bool IsEmpty() const;
  bool IsFull() const;

  /// Provides access to elements by index within the used range.
  T& operator[](xiiUInt32 uiIndex) const;

  T*        m_pData;
  xiiUInt32 m_uiCount;
};

/// Specialized allocator for fixed-size memory blocks, optimized for bulk allocations.
///
/// This allocator manages memory in large chunks called "SuperBlocks" (16 blocks each) and
/// provides individual blocks of the specified size on demand. It's designed for scenarios
/// where you need many identically-sized allocations with good spatial locality.
///
/// SuperBlock strategy reduces fragmentation and improves cache performance by grouping
/// related allocations together. When blocks are freed, they're added to a free list for
/// immediate reuse without returning memory to the OS.
///
/// Best used for:
/// - Object pools where objects have uniform size
/// - Bulk allocations for data structures like arrays or strings
/// - Memory regions that benefit from spatial locality
template <xiiUInt32 BlockSizeInByte>
class xiiLargeBlockAllocator
{
public:
  xiiLargeBlockAllocator(xiiStringView sName, xiiAllocator* pParent, xiiAllocatorTrackingMode mode = xiiAllocatorTrackingMode::Default);
  ~xiiLargeBlockAllocator();

  /// Allocates a new typed block capable of holding elements of type T.
  ///
  /// Returns a typed wrapper around a raw memory block. The block can hold
  /// BlockSizeInByte / sizeof(T) elements. If allocation fails, returns an
  /// invalid block (check with IsEmpty()).
  template <typename T>
  xiiDataBlock<T, BlockSizeInByte> AllocateBlock();

  /// Deallocates a previously allocated block.
  template <typename T>
  void DeallocateBlock(xiiDataBlock<T, BlockSizeInByte>& ref_block);

  /// Returns the name of this allocator instance.
  xiiStringView GetName() const;

  /// Returns the unique identifier for this allocator instance.
  xiiAllocatorId GetId() const;

  const xiiAllocator::Stats& GetStats() const;

private:
  void* Allocate(size_t uiAlign);
  void  Deallocate(void* pPtr);

  xiiAllocatorId           m_Id;
  xiiAllocatorTrackingMode m_TrackingMode;

  xiiMutex m_Mutex;

  struct SuperBlock
  {
    XII_DECLARE_POD_TYPE();

    enum
    {
      NUM_BLOCKS    = 16,
      SIZE_IN_BYTES = BlockSizeInByte * NUM_BLOCKS
    };

    void* m_pBasePtr;

    xiiUInt32 m_uiUsedBlocks;
  };

  xiiDynamicArray<SuperBlock> m_SuperBlocks;
  xiiDynamicArray<xiiUInt32>  m_FreeBlocks;
};

#include <Foundation/Memory/Implementation/LargeBlockAllocator_inl.h>
