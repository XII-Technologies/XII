
template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::ConstIterator(
  const xiiBlockStorage<T, BlockSize, StorageType>& storage,
  xiiUInt32                                         uiStartIndex,
  xiiUInt32                                         uiCount) :
  m_Storage(storage)
{
  m_uiCurrentIndex = uiStartIndex;
  m_uiEndIndex     = xiiMath::Max(uiStartIndex + uiCount, uiCount);

  if (StorageType == xiiBlockStorageType::FreeList)
  {
    xiiUInt32 uiEndIndex = xiiMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsBitSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE T& xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::CurrentElement() const
{
  const xiiUInt32 uiBlockIndex = m_uiCurrentIndex / xiiDataBlock<T, BlockSize>::CAPACITY;
  const xiiUInt32 uiInnerIndex = m_uiCurrentIndex - uiBlockIndex * xiiDataBlock<T, BlockSize>::CAPACITY;
  return m_Storage.m_Blocks[uiBlockIndex][uiInnerIndex];
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE const T& xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator*() const
{
  return CurrentElement();
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE const T* xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator->() const
{
  return &CurrentElement();
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator const T*() const
{
  return &CurrentElement();
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE void xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::Next()
{
  ++m_uiCurrentIndex;

  if (StorageType == xiiBlockStorageType::FreeList)
  {
    xiiUInt32 uiEndIndex = xiiMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
    while (m_uiCurrentIndex < uiEndIndex && !m_Storage.m_UsedEntries.IsBitSet(m_uiCurrentIndex))
    {
      ++m_uiCurrentIndex;
    }
  }
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE bool xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::IsValid() const
{
  return m_uiCurrentIndex < xiiMath::Min(m_uiEndIndex, m_Storage.m_uiCount);
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE void xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator::operator++()
{
  Next();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE xiiBlockStorage<T, BlockSize, StorageType>::Iterator::Iterator(
  const xiiBlockStorage<T, BlockSize, StorageType>& storage,
  xiiUInt32                                         uiStartIndex,
  xiiUInt32                                         uiCount) :
  ConstIterator(storage, uiStartIndex, uiCount)
{
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE T& xiiBlockStorage<T, BlockSize, StorageType>::Iterator::operator*()
{
  return this->CurrentElement();
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE T* xiiBlockStorage<T, BlockSize, StorageType>::Iterator::operator->()
{
  return &(this->CurrentElement());
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE xiiBlockStorage<T, BlockSize, StorageType>::Iterator::operator T*()
{
  return &(this->CurrentElement());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE xiiBlockStorage<T, BlockSize, StorageType>::xiiBlockStorage(
  xiiLargeBlockAllocator<BlockSize>* pBlockAllocator,
  xiiAllocatorBase*                  pAllocator) :
  m_pBlockAllocator(pBlockAllocator), m_Blocks(pAllocator), m_uiCount(0), m_uiFreelistStart(xiiInvalidIndex)
{
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
xiiBlockStorage<T, BlockSize, StorageType>::~xiiBlockStorage()
{
  Clear();
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
void xiiBlockStorage<T, BlockSize, StorageType>::Clear()
{
  for (xiiUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    xiiDataBlock<T, BlockSize>& block = m_Blocks[uiBlockIndex];

    if (StorageType == xiiBlockStorageType::Compact)
    {
      xiiMemoryUtils::Destruct(block.m_pData, block.m_uiCount);
    }
    else
    {
      for (xiiUInt32 uiInnerIndex = 0; uiInnerIndex < block.m_uiCount; ++uiInnerIndex)
      {
        xiiUInt32 uiIndex = uiBlockIndex * xiiDataBlock<T, BlockSize>::CAPACITY + uiInnerIndex;
        if (m_UsedEntries.IsBitSet(uiIndex))
        {
          xiiMemoryUtils::Destruct(&block.m_pData[uiInnerIndex], 1);
        }
      }
    }

    m_pBlockAllocator->DeallocateBlock(block);
  }

  m_Blocks.Clear();
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
T* xiiBlockStorage<T, BlockSize, StorageType>::Create()
{
  T*        pNewObject = nullptr;
  xiiUInt32 uiNewIndex = xiiInvalidIndex;

  if (StorageType == xiiBlockStorageType::FreeList && m_uiFreelistStart != xiiInvalidIndex)
  {
    uiNewIndex = m_uiFreelistStart;

    const xiiUInt32 uiBlockIndex = uiNewIndex / xiiDataBlock<T, BlockSize>::CAPACITY;
    const xiiUInt32 uiInnerIndex = uiNewIndex - uiBlockIndex * xiiDataBlock<T, BlockSize>::CAPACITY;

    pNewObject = &(m_Blocks[uiBlockIndex][uiInnerIndex]);

    m_uiFreelistStart = *reinterpret_cast<xiiUInt32*>(pNewObject);
  }
  else
  {
    xiiDataBlock<T, BlockSize>* pBlock = nullptr;

    if (m_Blocks.GetCount() > 0)
    {
      pBlock = &m_Blocks.PeekBack();
    }

    if (pBlock == nullptr || pBlock->IsFull())
    {
      m_Blocks.PushBack(m_pBlockAllocator->template AllocateBlock<T>());
      pBlock = &m_Blocks.PeekBack();
    }

    pNewObject = pBlock->ReserveBack();
    uiNewIndex = m_uiCount;

    ++m_uiCount;
  }

  xiiMemoryUtils::Construct(pNewObject, 1);

  if (StorageType == xiiBlockStorageType::FreeList)
  {
    m_UsedEntries.SetCount(m_uiCount);
    m_UsedEntries.SetBit(uiNewIndex);
  }

  return pNewObject;
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE void xiiBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject)
{
  T* pDummy;
  Delete(pObject, pDummy);
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
void xiiBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject)
{
  Delete(pObject, out_pMovedObject, xiiTraitInt<StorageType>());
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE xiiUInt32 xiiBlockStorage<T, BlockSize, StorageType>::GetCount() const
{
  return m_uiCount;
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE typename xiiBlockStorage<T, BlockSize, StorageType>::Iterator xiiBlockStorage<T, BlockSize, StorageType>::GetIterator(
  xiiUInt32 uiStartIndex /*= 0*/,
  xiiUInt32 uiCount /*= xiiInvalidIndex*/)
{
  return Iterator(*this, uiStartIndex, uiCount);
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_ALWAYS_INLINE typename xiiBlockStorage<T, BlockSize, StorageType>::ConstIterator xiiBlockStorage<T, BlockSize, StorageType>::GetIterator(
  xiiUInt32 uiStartIndex /*= 0*/,
  xiiUInt32 uiCount /*= xiiInvalidIndex*/) const
{
  return ConstIterator(*this, uiStartIndex, uiCount);
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE void xiiBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, xiiTraitInt<xiiBlockStorageType::Compact>)
{
  xiiDataBlock<T, BlockSize>& lastBlock = m_Blocks.PeekBack();
  T*                          pLast     = lastBlock.PopBack();

  --m_uiCount;
  if (pObject != pLast)
  {
    xiiMemoryUtils::Relocate(pObject, pLast, 1);
  }
  else
  {
    xiiMemoryUtils::Destruct(pLast, 1);
  }

  out_pMovedObject = pLast;

  if (lastBlock.IsEmpty())
  {
    m_pBlockAllocator->DeallocateBlock(lastBlock);
    m_Blocks.PopBack();
  }
}

template <typename T, xiiUInt32 BlockSize, xiiBlockStorageType::Enum StorageType>
XII_FORCE_INLINE void xiiBlockStorage<T, BlockSize, StorageType>::Delete(T* pObject, T*& out_pMovedObject, xiiTraitInt<xiiBlockStorageType::FreeList>)
{
  xiiUInt32 uiIndex = xiiInvalidIndex;
  for (xiiUInt32 uiBlockIndex = 0; uiBlockIndex < m_Blocks.GetCount(); ++uiBlockIndex)
  {
    ptrdiff_t diff = pObject - m_Blocks[uiBlockIndex].m_pData;
    if (diff >= 0 && diff < xiiDataBlock<T, BlockSize>::CAPACITY)
    {
      uiIndex = uiBlockIndex * xiiDataBlock<T, BlockSize>::CAPACITY + (xiiInt32)diff;
      break;
    }
  }

  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "Invalid object {0} was not found in block storage.", xiiArgP(pObject));

  m_UsedEntries.ClearBit(uiIndex);

  out_pMovedObject = pObject;
  xiiMemoryUtils::Destruct(pObject, 1);

  *reinterpret_cast<xiiUInt32*>(pObject) = m_uiFreelistStart;
  m_uiFreelistStart                      = uiIndex;
}
