/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkIOBuffer.h>
#include <Foundation/Types/SharedPtr.h>

/// Zero entries terminate the strictly increasing size classes. Larger requests use uncached blocks.
struct XII_FOUNDATION_DLL xiiNetworkBufferPoolConfiguration
{
  xiiUInt32 m_uiSizeClasses[8] = {256, 1024, 4096, 16384, 65536, 0, 0, 0};
  xiiUInt64 m_uiMemoryBudget   = 64ULL * 1024 * 1024;
};

struct XII_FOUNDATION_DLL xiiNetworkBufferPoolStatistics
{
  xiiUInt64 m_uiCommittedBytes     = 0;
  xiiUInt64 m_uiLeasedBytes        = 0;
  xiiUInt64 m_uiAllocations        = 0;
  xiiUInt64 m_uiReuses             = 0;
  xiiUInt64 m_uiActiveBuffers      = 0;
  xiiUInt64 m_uiBackpressureEvents = 0;
};

/// Independently owned, thread-safe buffer pool using a Foundation allocator and intrusive free lists.
/// Initialization/destruction must not race other calls. Acquire/release/statistics/Trim may be concurrent.
/// Destroying the facade leaves existing leases valid; the shared pool state owns their backing memory.
class XII_FOUNDATION_DLL xiiNetworkBufferPool
{
public:
  xiiNetworkBufferPool();
  ~xiiNetworkBufferPool();
  xiiNetworkBufferPool(xiiNetworkBufferPool&&)                         = delete;
  xiiNetworkBufferPool& operator=(xiiNetworkBufferPool&&)              = delete;
  xiiNetworkBufferPool(const xiiNetworkBufferPool&)                    = delete;
  xiiNetworkBufferPool&         operator=(const xiiNetworkBufferPool&) = delete;
  [[nodiscard]] xiiNetworkError Initialize(const xiiNetworkBufferPoolConfiguration& configuration = {}, xiiAllocator* pAllocator = nullptr);
  /// Allocates at least one byte for an empty datagram lease; its selected count remains zero.
  /// Backpressure leaves out_buffer unchanged. Cached blocks count against the hard memory budget.
  [[nodiscard]] xiiNetworkError                Acquire(xiiUInt32 uiCount, xiiNetworkIOBuffer& out_buffer);
  void                                         Trim();
  [[nodiscard]] xiiNetworkBufferPoolStatistics GetStatistics() const;

private:
  friend class xiiNetworkIOBufferAccess;
  xiiSharedPtr<xiiNetworkBufferPoolState> m_pState;
};
