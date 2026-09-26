/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Resources/Buffer.h>

/// A templated vertex buffer pool that manages dynamic vertex allocations.
/// This divides its memory into one or more chunks to minimize reallocation and supports thread-safe allocation, update, and tracking of vertex usage.
template <typename VertexType, typename MutexType = xiiNoMutex, typename AllocatorWrapper = xiiDefaultAllocatorWrapper>
class xiiGALVertexBufferPool
{
public:
  /// A handle for a vertex allocation.
  struct AllocationHandle
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiChunkIndex = 0; ///< Index into the pool's internal chunks.
    xiiUInt32 m_uiOffset     = 0; ///< Offset within the chunk.
    xiiUInt32 m_uiCount      = 0; ///< Number of vertices allocated.
  };

  /// Aggregated usage statistics for the pool.
  struct UsageStatistics
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiTotalCapacity   = 0; ///< Total vertices allocated across all chunks.
    xiiUInt64 m_uiUsageCount      = 0; ///< Total vertices currently in use.
    xiiUInt32 m_uiAllocationCount = 0; ///< The number of active allocations.
  };

  /// Constructs the vertex buffer pool.
  ///
  /// \param uiInitialChunkSize - The number of vertices in the first (and minimum) chunk.
  /// \param uiExpansionFactor  - Determines the size of new chunks (e.g. 2 means double the previous chunk).
  xiiGALVertexBufferPool(xiiSharedPtr<xiiGALDevice> pDevice, xiiStringView sName = {}, xiiAllocator* pAllocator = AllocatorWrapper::GetAllocator(), xiiUInt32 uiInitialChunkSize = 1024, xiiUInt32 uiExpansionFactor = 2U);
  ~xiiGALVertexBufferPool();

  /// Allocates a block of vertices from the pool.
  ///
  /// \param uiCount - The number of vertices requested.
  ///
  /// \return An AllocationHandle that describes where the vertices live.
  AllocationHandle Allocate(xiiUInt32 uiCount);

  /// Updates the data stored in a given allocation.
  ///
  /// \param handle - The allocation handle previously returned by Allocate().
  /// \param pData  - An array containing the new vertex data.
  void Update(const AllocationHandle& handle, xiiArrayPtr<const VertexType> pData, xiiSharedPtr<xiiGALCommandList> pCommandList);

  /// Returns a read-only pointer to the allocated vertex data.
  ///
  /// \note Use with caution – modifications via the pointer are not protected by the pool’s mutex.
  xiiArrayPtr<const VertexType> GetAllocationPointer(const AllocationHandle& handle) const;

  // Retrieve a GPU buffer pointer to allow binding to a pipeline. For a specific allocation, you can query the chunk’s buffer.
  xiiSharedPtr<xiiGALBuffer> GetGPUBuffer(const AllocationHandle& handle) const;

  void Reset();

  xiiGALVertexBufferPool::UsageStatistics GetUsageStatistics() const;

private:
  /// Represents a single memory chunk in the pool.
  struct Chunk
  {
    explicit Chunk(xiiAllocator* pAllocator, xiiUInt32 uiCapacity, xiiSharedPtr<xiiGALDevice> pDevice) :
      m_Vertices(pAllocator), m_uiUsageCount(0)
    {
      m_Vertices.SetCount(uiCapacity);

      xiiGALBufferCreationDescription description;
      description.m_uiSize    = uiCapacity * sizeof(VertexType);
      description.m_BindFlags = xiiGALBindFlags::VertexBuffer;
      description.m_Usage     = xiiGALResourceUsage::Mutable;

      m_pBuffer = pDevice->CreateBuffer(description);
    }

    XII_ALWAYS_INLINE xiiUInt32 GetCapacity() const { return m_Vertices.GetCount(); }

    xiiDynamicArray<VertexType> m_Vertices;     ///< Storage for vertices.
    xiiUInt64                   m_uiUsageCount; ///< Number of vertices currently allocated.
    xiiSharedPtr<xiiGALBuffer>  m_pBuffer;      ///< The GPU bufffer associated with this chunk.
  };

  mutable MutexType m_Mutex;

  xiiProxyAllocator m_Allocator;

  xiiSharedPtr<xiiGALDevice> m_pDevice;

  xiiDynamicArray<Chunk>            m_Chunks;      ///< All chunks managed by the pool.
  xiiDynamicArray<AllocationHandle> m_Allocations; ///< Tracking of active allocations.

  xiiUInt64 m_uiInitialChunkSize; ///< Starting capacity for a new chunk.
  xiiUInt64 m_uiExpansionFactor;  ///< Factor by which new chunks expand relative to the previous chunk.
};

#include <GraphicsFoundation/Tools/Implementation/VertexBufferPool_inl.h>
