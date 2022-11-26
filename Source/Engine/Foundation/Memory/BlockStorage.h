#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Memory/LargeBlockAllocator.h>

struct xiiBlockStorageType
{
  enum Enum
  {
    Compact,
    FreeList
  };
};

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

  xiiBlockStorage(xiiLargeBlockAllocator<BlockSizeInByte>* pBlockAllocator, xiiAllocatorBase* pAllocator);
  ~xiiBlockStorage();

  void Clear();

  T*   Create();
  void Delete(T* pObject);
  void Delete(T* pObject, T*& out_pMovedObject);

  xiiUInt32     GetCount() const;
  Iterator      GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiInvalidIndex);
  ConstIterator GetIterator(xiiUInt32 uiStartIndex = 0, xiiUInt32 uiCount = xiiInvalidIndex) const;

private:
  void Delete(T* pObject, T*& out_pMovedObject, xiiTraitInt<xiiBlockStorageType::Compact>);
  void Delete(T* pObject, T*& out_pMovedObject, xiiTraitInt<xiiBlockStorageType::FreeList>);

  xiiLargeBlockAllocator<BlockSizeInByte>* m_pBlockAllocator;

  xiiDynamicArray<xiiDataBlock<T, BlockSizeInByte>> m_Blocks;
  xiiUInt32                                         m_uiCount;

  xiiUInt32 m_uiFreelistStart;

  xiiDynamicBitfield m_UsedEntries;
};

#include <Foundation/Memory/Implementation/BlockStorage_inl.h>
