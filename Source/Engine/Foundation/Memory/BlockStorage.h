/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

/// Defines storage strategies for block-based container management.
struct xiiBlockStorageType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Compact = 0U, ///< Maintains elements in contiguous memory by moving last element to fill gaps.
    FreeList      ///< Uses a free list to track available slots, preserving element positions.
  };
};

/// High-performance container for objects with pluggable storage strategies.
///
/// This container manages objects in blocks of memory, using different strategies for handling gaps when objects are removed.
/// It's designed for scenarios where you need fast allocation and deallocation of many objects, with the choice between compact memory layout or stable object addressing.
///
/// Storage strategies:
/// - Compact: Moves the last element to fill gaps when objects are deleted, maintaining contiguous memory but invalidating iterators and pointers to moved objects
/// - FreeList: Uses a free list to reuse deleted slots, preserving object positions but potentially creating memory fragmentation.
template <typename T, xiiUInt32 BlockSizeInByte, xiiBlockStorageType::Enum StorageType>
class xiiBlockStorage
{
public:
  class ConstIterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();
    bool IsValid() const;

    void operator++();

  protected:
    friend class xiiBlockStorage<T, BlockSizeInByte, StorageType>;

    ConstIterator(const xiiBlockStorage<T, BlockSizeInByte, StorageType>& storage, xiiUInt32 uiStartIndex, xiiUInt32 uiCount);

    T& CurrentElement() const;

    const xiiBlockStorage<T, BlockSizeInByte, StorageType>& m_Storage;
    xiiUInt32                                               m_uiCurrentIndex;
    xiiUInt32                                               m_uiEndIndex;
  };

  class Iterator : public ConstIterator
  {
  public:
    T& operator*();
    T* operator->();

    operator T*();

  private:
    friend class xiiBlockStorage<T, BlockSizeInByte, StorageType>;

    Iterator(const xiiBlockStorage<T, BlockSizeInByte, StorageType>& storage, xiiUInt32 uiStartIndex, xiiUInt32 uiCount);
  };

  xiiBlockStorage(xiiLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, xiiAllocator* pAllocator);
  ~xiiBlockStorage();

  /// Removes all objects and deallocates all blocks.
  void Clear();

  /// Creates a new object and returns a pointer to it.
  ///
  /// The object is default-constructed. Returns nullptr if allocation fails.
  T* Create();

  /// Deletes the specified object.
  void Delete(T* pObject);

  /// Deletes the specified object and reports any moved object.
  ///
  /// For Compact storage, if another object is moved to fill the gap,
  /// out_pMovedObject will point to the moved object's new location.
  /// For FreeList storage, out_pMovedObject is always set to nullptr.
  void Delete(T* pObject, T*& out_pMovedObject);

  /// Returns the total number of objects currently stored.
  xiiUInt32 GetCount() const;

  /// Returns an iterator for traversing objects in a specified range.
  Iterator GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiInvalidIndex);

  /// Returns a const iterator for traversing objects in a specified range.
  ConstIterator GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiInvalidIndex) const;

private:
  void Delete(T* pObject, T*& out_pMovedObject, xiiTraitInt<xiiBlockStorageType::Compact>);
  void Delete(T* pObject, T*& out_pMovedObject, xiiTraitInt<xiiBlockStorageType::FreeList>);

  xiiLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  xiiDynamicArray<xiiDataBlock<T, BlockSizeInByte>> m_Blocks;
  xiiUInt32                                         m_uiCount = 0;

  xiiUInt32 m_uiFreelistStart = xiiInvalidIndex;

  xiiDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>
