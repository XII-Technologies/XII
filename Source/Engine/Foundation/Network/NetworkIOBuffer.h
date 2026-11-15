/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Network/NetworkError.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/SharedPtr.h>

class xiiNetworkBufferPool;
class xiiNetworkBufferPoolState;
class xiiNetworkIOBufferAccess;

/// Move-only pooled memory lease. Moving the lease never moves payload storage.
/// Borrowed spans expire when the lease is released or transferred to an asynchronous operation.
/// Do not retain or access spans after submission, including while cancellation is pending.
/// A lease may outlive its pool/context; its allocator must outlive every outstanding lease.
class XII_FOUNDATION_DLL xiiNetworkIOBuffer
{
public:
  xiiNetworkIOBuffer();
  ~xiiNetworkIOBuffer();
  xiiNetworkIOBuffer(const xiiNetworkIOBuffer&)            = delete;
  xiiNetworkIOBuffer& operator=(const xiiNetworkIOBuffer&) = delete;
  xiiNetworkIOBuffer(xiiNetworkIOBuffer&& other) noexcept;
  xiiNetworkIOBuffer& operator=(xiiNetworkIOBuffer&& other) noexcept;

  [[nodiscard]] bool      IsValid() const { return m_pData != nullptr; }
  [[nodiscard]] xiiUInt32 GetCapacity() const { return m_uiCapacity; }
  [[nodiscard]] xiiUInt32 GetCount() const { return m_uiCount; }
  /// Selects the payload/receive capacity, never exceeding GetCapacity(). Unchanged on failure.
  [[nodiscard]] xiiNetworkError             SetCount(xiiUInt32 uiCount);
  [[nodiscard]] xiiArrayPtr<xiiUInt8>       GetBytes() { return {m_pData, m_uiCount}; }
  [[nodiscard]] xiiArrayPtr<const xiiUInt8> GetBytes() const { return {m_pData, m_uiCount}; }
  void                                      Clear();

private:
  friend class xiiNetworkBufferPool;
  friend class xiiNetworkIOBufferAccess;
  xiiSharedPtr<xiiNetworkBufferPoolState> m_pState;
  xiiUInt8*                               m_pData      = nullptr;
  xiiUInt32                               m_uiCapacity = 0;
  xiiUInt32                               m_uiCount    = 0;
  xiiUInt32                               m_uiBucket   = 0;
};

/// Up to 64 buffer leases form one logical transfer; common vectors use inline descriptor storage.
using xiiNetworkIOBuffers = xiiHybridArray<xiiNetworkIOBuffer, 8>;
