
template <typename T, xiiUInt32 SizeInBytes>
XII_ALWAYS_INLINE xiiDataBlock<T, SizeInBytes>::xiiDataBlock(T* pData, xiiUInt32 uiCount)
{
  m_pData   = pData;
  m_uiCount = uiCount;
}

template <typename T, xiiUInt32 SizeInBytes>
XII_FORCE_INLINE T* xiiDataBlock<T, SizeInBytes>::ReserveBack()
{
  XII_ASSERT_DEV(m_uiCount < CAPACITY, "Block is full.");
  return m_pData + m_uiCount++;
}

template <typename T, xiiUInt32 SizeInBytes>
XII_FORCE_INLINE T* xiiDataBlock<T, SizeInBytes>::PopBack()
{
  XII_ASSERT_DEV(m_uiCount > 0, "Block is empty");
  --m_uiCount;
  return m_pData + m_uiCount;
}

template <typename T, xiiUInt32 SizeInBytes>
XII_ALWAYS_INLINE bool xiiDataBlock<T, SizeInBytes>::IsEmpty() const
{
  return m_uiCount == 0;
}

template <typename T, xiiUInt32 SizeInBytes>
XII_ALWAYS_INLINE bool xiiDataBlock<T, SizeInBytes>::IsFull() const
{
  return m_uiCount == CAPACITY;
}

template <typename T, xiiUInt32 SizeInBytes>
XII_FORCE_INLINE T& xiiDataBlock<T, SizeInBytes>::operator[](xiiUInt32 uiIndex) const
{
  XII_ASSERT_DEV(uiIndex < m_uiCount, "Out of bounds access. Data block has {0} elements, trying to access element at index {1}.", m_uiCount, uiIndex);
  return m_pData[uiIndex];
}

///////////////////////////////////////////////////////////////////////////////////////////////////

template <xiiUInt32 BlockSize>
xiiLargeBlockAllocator<BlockSize>::xiiLargeBlockAllocator(xiiStringView sName, xiiAllocatorBase* pParent, xiiAllocatorTrackingMode mode) :
  m_TrackingMode(mode), m_SuperBlocks(pParent), m_FreeBlocks(pParent)
{
  static_assert(BlockSize >= 4096, "Block size must be 4096 or bigger");

  m_Id       = xiiMemoryTracker::RegisterAllocator(sName, mode, xiiPageAllocator::GetId());
  m_ThreadID = xiiThreadUtils::GetCurrentThreadID();

  const xiiUInt32 uiPageSize = xiiSystemInformation::Get().GetMemoryPageSize();
  XII_IGNORE_UNUSED(uiPageSize);
  XII_ASSERT_DEV(uiPageSize <= BlockSize, "Memory Page size is bigger than block size.");
  XII_ASSERT_DEV(BlockSize % uiPageSize == 0, "Blocksize ({0}) must be a multiple of page size ({1})", BlockSize, uiPageSize);
}

template <xiiUInt32 BlockSize>
xiiLargeBlockAllocator<BlockSize>::~xiiLargeBlockAllocator()
{
  XII_ASSERT_RELEASE(m_ThreadID == xiiThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");
  xiiMemoryTracker::DeregisterAllocator(m_Id);

  for (xiiUInt32 i = 0; i < m_SuperBlocks.GetCount(); ++i)
  {
    xiiPageAllocator::DeallocatePage(m_SuperBlocks[i].m_pBasePtr);
  }
}

template <xiiUInt32 BlockSize>
template <typename T>
XII_FORCE_INLINE xiiDataBlock<T, BlockSize> xiiLargeBlockAllocator<BlockSize>::AllocateBlock()
{
  struct Helper
  {
    enum
    {
      BLOCK_CAPACITY = xiiDataBlock<T, BlockSize>::CAPACITY
    };
  };

  static_assert(Helper::BLOCK_CAPACITY >= 1, "Type is too big for block allocation. Consider using regular heap allocation instead or increase the block size.");

  xiiDataBlock<T, BlockSize> block(static_cast<T*>(Allocate(XII_ALIGNMENT_OF(T))), 0);
  return block;
}

template <xiiUInt32 BlockSize>
template <typename T>
XII_FORCE_INLINE void xiiLargeBlockAllocator<BlockSize>::DeallocateBlock(xiiDataBlock<T, BlockSize>& ref_block)
{
  Deallocate(ref_block.m_pData);
  ref_block.m_pData   = nullptr;
  ref_block.m_uiCount = 0;
}

template <xiiUInt32 BlockSize>
XII_ALWAYS_INLINE xiiStringView xiiLargeBlockAllocator<BlockSize>::GetName() const
{
  return xiiMemoryTracker::GetAllocatorName(m_Id);
}

template <xiiUInt32 BlockSize>
XII_ALWAYS_INLINE xiiAllocatorId xiiLargeBlockAllocator<BlockSize>::GetId() const
{
  return m_Id;
}

template <xiiUInt32 BlockSize>
XII_ALWAYS_INLINE const xiiAllocatorBase::Stats& xiiLargeBlockAllocator<BlockSize>::GetStats() const
{
  return xiiMemoryTracker::GetAllocatorStats(m_Id);
}

template <xiiUInt32 BlockSize>
void* xiiLargeBlockAllocator<BlockSize>::Allocate(size_t uiAlign)
{
  XII_ASSERT_RELEASE(xiiMath::IsPowerOf2((xiiUInt32)uiAlign), "Alignment must be power of two");

  xiiTime fAllocationTime = xiiTime::Now();

  XII_LOCK(m_Mutex);

  void* ptr = nullptr;

  if (!m_FreeBlocks.IsEmpty())
  {
    // Re-use a super block
    xiiUInt32 uiFreeBlockIndex = m_FreeBlocks.PeekBack();
    m_FreeBlocks.PopBack();

    const xiiUInt32 uiSuperBlockIndex = uiFreeBlockIndex / SuperBlock::NUM_BLOCKS;
    const xiiUInt32 uiInnerBlockIndex = uiFreeBlockIndex & (SuperBlock::NUM_BLOCKS - 1);
    SuperBlock&     superBlock        = m_SuperBlocks[uiSuperBlockIndex];
    ++superBlock.m_uiUsedBlocks;

    ptr = xiiMemoryUtils::AddByteOffset(superBlock.m_pBasePtr, uiInnerBlockIndex * BlockSize);
  }
  else
  {
    // Allocate a new super block
    void* pMemory = xiiPageAllocator::AllocatePage(SuperBlock::SIZE_IN_BYTES);
    XII_CHECK_ALIGNMENT(pMemory, uiAlign);

    SuperBlock superBlock;
    superBlock.m_pBasePtr     = pMemory;
    superBlock.m_uiUsedBlocks = 1;

    m_SuperBlocks.PushBack(superBlock);

    const xiiUInt32 uiBlockBaseIndex = (m_SuperBlocks.GetCount() - 1) * SuperBlock::NUM_BLOCKS;
    for (xiiUInt32 i = SuperBlock::NUM_BLOCKS - 1; i > 0; --i)
    {
      m_FreeBlocks.PushBack(uiBlockBaseIndex + i);
    }

    ptr = pMemory;
  }

  if (m_TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::AddAllocation(m_Id, m_TrackingMode, ptr, BlockSize, uiAlign, xiiTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <xiiUInt32 BlockSize>
void xiiLargeBlockAllocator<BlockSize>::Deallocate(void* ptr)
{
  XII_LOCK(m_Mutex);

  if (m_TrackingMode >= xiiAllocatorTrackingMode::AllocationStats)
  {
    xiiMemoryTracker::RemoveAllocation(m_Id, ptr);
  }

  // find super block
  bool           bFound            = false;
  xiiUInt32      uiSuperBlockIndex = m_SuperBlocks.GetCount();
  std::ptrdiff_t diff              = 0;

  for (; uiSuperBlockIndex-- > 0;)
  {
    diff = (char*)ptr - (char*)m_SuperBlocks[uiSuperBlockIndex].m_pBasePtr;
    if (diff >= 0 && diff < SuperBlock::SIZE_IN_BYTES)
    {
      bFound = true;
      break;
    }
  }

  XII_IGNORE_UNUSED(bFound);
  XII_ASSERT_DEV(bFound, "'{0}' was not allocated with this allocator", xiiArgP(ptr));

  SuperBlock& superBlock = m_SuperBlocks[uiSuperBlockIndex];
  --superBlock.m_uiUsedBlocks;

  if (superBlock.m_uiUsedBlocks == 0 && m_FreeBlocks.GetCount() > SuperBlock::NUM_BLOCKS * 4)
  {
    // give memory back
    xiiPageAllocator::DeallocatePage(superBlock.m_pBasePtr);

    m_SuperBlocks.RemoveAtAndSwap(uiSuperBlockIndex);
    const xiiUInt32 uiLastSuperBlockIndex = m_SuperBlocks.GetCount();

    // patch free list
    for (xiiUInt32 i = 0; i < m_FreeBlocks.GetCount(); ++i)
    {
      const xiiUInt32 uiIndex   = m_FreeBlocks[i];
      const xiiUInt32 uiSBIndex = uiIndex / SuperBlock::NUM_BLOCKS;

      if (uiSBIndex == uiSuperBlockIndex)
      {
        // points to the block we just removed
        m_FreeBlocks.RemoveAtAndSwap(i);
        --i;
      }
      else if (uiSBIndex == uiLastSuperBlockIndex)
      {
        // points to the block we just swapped
        m_FreeBlocks[i] = uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + (uiIndex & (SuperBlock::NUM_BLOCKS - 1));
      }
    }
  }
  else
  {
    // add block to free list
    const xiiUInt32 uiInnerBlockIndex = (xiiUInt32)(diff / BlockSize);
    m_FreeBlocks.PushBack(uiSuperBlockIndex * SuperBlock::NUM_BLOCKS + uiInnerBlockIndex);
  }
}
