/// Copyright (c) Theophilus Eriata. All Rights Reserved.

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::xiiGALVertexBufferPool(xiiSharedPtr<xiiGALDevice> pDevice, xiiStringView sName, xiiAllocator* pAllocator, xiiUInt32 uiInitialChunkSize, xiiUInt32 uiExpansionFactor) :
  m_Allocator(sName.IsEmpty() ? "VertexBufferPool" : sName, pAllocator), m_pDevice(std::move(pDevice)), m_uiInitialChunkSize(uiInitialChunkSize), m_uiExpansionFactor(uiExpansionFactor)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "A valid device is required.");
  XII_ASSERT_DEV(uiInitialChunkSize > 0, "Initial chunk size must be greater than zero.");
  XII_ASSERT_DEV(uiExpansionFactor > 0, "Expansion factor must be greater than zero.");

  // Create the first memory chunk.
  m_Chunks.PushBack(Chunk(&m_Allocator, uiInitialChunkSize, m_pDevice));
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::~xiiGALVertexBufferPool() = default;

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE typename xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::AllocationHandle xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::Allocate(xiiUInt32 uiCount)
{
  XII_ASSERT_DEV(uiCount > 0U, "Allocation size must be greater than zero.");

  XII_LOCK(m_Mutex);

  xiiUInt32 uiChunkIndex = 0U;
  for (; uiChunkIndex < m_Chunks.GetCount(); ++uiChunkIndex)
  {
    const Chunk& chunk = m_Chunks[uiChunkIndex];
    if (chunk.GetCapacity() - chunk.m_uiUsageCount >= uiCount)
      break;
  }

  if (uiChunkIndex == m_Chunks.GetCount())
  {
    const xiiUInt64 uiExpandedCapacity = m_Chunks.IsEmpty() ? m_uiInitialChunkSize : m_Chunks.PeekBack().GetCapacity() * m_uiExpansionFactor;
    const xiiUInt32 uiNewCapacity      = static_cast<xiiUInt32>(xiiMath::Max<xiiUInt64>(uiCount, uiExpandedCapacity));
    m_Chunks.PushBack(Chunk(&m_Allocator, uiNewCapacity, m_pDevice));
  }

  Chunk& chunk = m_Chunks[uiChunkIndex];

  AllocationHandle handle;
  handle.m_uiChunkIndex = uiChunkIndex;
  handle.m_uiOffset     = static_cast<xiiUInt32>(chunk.m_uiUsageCount);
  handle.m_uiCount      = uiCount;

  chunk.m_uiUsageCount += uiCount;
  m_Allocations.PushBack(handle);
  return handle;
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE void xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::Update(const AllocationHandle& handle, xiiArrayPtr<const VertexType> pData, xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  XII_ASSERT_DEV(pData.GetCount() == handle.m_uiCount, "The data size does not match the allocation count.");

  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(handle.m_uiChunkIndex < m_Chunks.GetCount(), "Invalid chunk index in allocation handle.");

  auto& chunk = m_Chunks[handle.m_uiChunkIndex];

  XII_ASSERT_DEV(handle.m_uiOffset + handle.m_uiCount <= chunk.m_uiUsageCount, "Chunk allocation handle exceeds the used size of the chunk.");

  // Copy data into the CPU-side storage.
  xiiMemoryUtils::Copy(chunk.m_Vertices.GetData() + handle.m_uiOffset, pData.GetPtr(), pData.GetCount());

  XII_ASSERT_DEV(pCommandList != nullptr, "A valid command list is required to update the GPU buffer.");
  const xiiArrayPtr<const xiiUInt8> sourceBytes(reinterpret_cast<const xiiUInt8*>(pData.GetPtr()), pData.GetCount() * sizeof(VertexType));
  pCommandList->UpdateBuffer(chunk.m_pBuffer.Borrow(), handle.m_uiOffset * sizeof(VertexType), sourceBytes);
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiArrayPtr<const VertexType> xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::GetAllocationPointer(const AllocationHandle& handle) const
{
  XII_ASSERT_DEV(handle.m_uiChunkIndex < m_Chunks.GetCount(), "Invalid chunk index in the chunk allocation handle.");

  const auto& chunk = m_Chunks[handle.m_uiChunkIndex];
  XII_IGNORE_UNUSED(chunk);

  XII_ASSERT_DEV(handle.m_uiOffset + handle.m_uiCount <= chunk.m_uiUsageCount, "Chunk allocation handle exceeds the used size of the chunk.");

  return chunk.m_Vertices.GetArrayPtr().GetSubArray(handle.m_uiOffset, handle.m_uiCount);
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::GetGPUBuffer(const AllocationHandle& handle) const
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(handle.m_uiChunkIndex < m_Chunks.GetCount(), "Invalid chunk index in allocation handle.");

  return m_Chunks[handle.m_uiChunkIndex].m_pBuffer;
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE void xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::Reset()
{
  XII_LOCK(m_Mutex);

  m_Chunks.Clear();
  m_Allocations.Clear();

  m_Chunks.PushBack(Chunk(&m_Allocator, static_cast<xiiUInt32>(m_uiInitialChunkSize), m_pDevice));
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::UsageStatistics xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::GetUsageStatistics() const
{
  XII_LOCK(m_Mutex);

  UsageStatistics stats{
    .m_uiTotalCapacity   = 0U,
    .m_uiUsageCount      = 0U,
    .m_uiAllocationCount = m_Allocations.GetCount(),
  };

  for (const Chunk& chunk : m_Chunks)
  {
    stats.m_uiTotalCapacity += chunk.GetCapacity();
    stats.m_uiUsageCount += chunk.m_uiUsageCount;
  }

  return stats;
}
