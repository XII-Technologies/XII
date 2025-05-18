
template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::xiiGALVertexBufferPool(xiiStringView sName, xiiAllocatorBase* pAllocator, xiiUInt32 uiInitialChunkSize, xiiUInt32 uiExpansionFactor) :
  m_Allocator(sName.IsEmpty() ? "VertexBufferPool" : sName, pAllocator), m_uiInitialChunkSize(uiInitialChunkSize), m_uiExpansionFactor(uiExpansionFactor)
{
  XII_ASSERT_DEV(uiInitialChunkSize > 0, "Initial chunk size must be greater than zero.");

  // Create the first memory chunk.
  m_Chunks.PushBack(xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::Chunk(&m_Allocator, uiInitialChunkSize, nullptr));
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::~xiiGALVertexBufferPool() = default;

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE void xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::Update(const AllocationHandle& handle, xiiArrayPtr<const VertexType> pData, xiiSharedPtr<xiiGALCommandList> pCommandList)
{
  XII_ASSERT_DEV(pData.GetCount() == handle.m_uiCount, "The data size does not match the allocation count.");

  XII_LOCK(m_Mutex);

  XII_ASSERT_DEV(handle.m_uiChunkIndex < m_Chunks.GetCount(), "Invalid chunk index in allocation handle.");

  const auto& chunk = m_Chunks[handle.m_uiChunkIndex];
  XII_IGNORE_UNUSED(chunk);

  XII_ASSERT_DEV(handle.m_uiOffset + handle.m_uiCount < chunk.m_uiCount, "Chunk allocation handle exceeds the used size of the chunk.");

  // Copy data into the CPU-side storage.
  memcpy(chunk.m_Vertices.GetData() + handle.m_uiOffset, pData.GetPtr(), pData.GetCount());

  // Update the GPU buffer
}

template <typename VertexType, typename MutexType, typename AllocatorWrapper>
XII_ALWAYS_INLINE xiiArrayPtr<const VertexType> xiiGALVertexBufferPool<VertexType, MutexType, AllocatorWrapper>::GetAllocationPointer(const AllocationHandle& handle) const
{
  XII_ASSERT_DEV(handle.m_uiChunkIndex < m_Chunks.GetCount(), "Invalid chunk index in the chunk allocation handle.");

  const auto& chunk = m_Chunks[handle.m_uiChunkIndex];
  XII_IGNORE_UNUSED(chunk);

  XII_ASSERT_DEV(handle.m_uiOffset + handle.m_uiCount < chunk.m_uiCount, "Chunk allocation handle exceeds the used size of the chunk.");

  return chunk.m_Vertices.GetArrayPtr().GetSubArray(handle.m_uiOffset);
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

  m_Chunks.PushBack(Chunk(&m_Allocator, m_uiInitialChunkSize));
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

  for (xiiUInt32 i = 0; i < m_Allocations.GetCount(); ++i)
  {
    stats.m_uiTotalCapacity += m_Allocations[i].GetCapacity();
    stats.m_uiUsageCount += m_Allocations[i].m_uiUsageCount;
  }

  return stats;
}
