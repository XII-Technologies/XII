/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Network/NetworkIOBuffer.h>
#include <Foundation/Network/NetworkIOOperation.h>
#include <Foundation/Network/TcpSocket.h>

/// Move-only completion record, delivered only through caller-driven dequeue operations.
/// Native access to buffer payloads has ended before delivery. DNS cancellation may be delivered
/// while a bounded resolver worker finishes an abandoned OS query; it cannot access this record.
/// Cancellation can have partial wire effects: byte counts describe known native progress only.
struct XII_FOUNDATION_DLL xiiNetworkIOResult
{
  xiiNetworkIOResult()                                         = default;
  ~xiiNetworkIOResult()                                        = default;
  xiiNetworkIOResult(const xiiNetworkIOResult&)                = delete;
  xiiNetworkIOResult& operator=(const xiiNetworkIOResult&)     = delete;
  xiiNetworkIOResult(xiiNetworkIOResult&&) noexcept            = default;
  xiiNetworkIOResult& operator=(xiiNetworkIOResult&&) noexcept = default;

  xiiNetworkIOOperation               m_Operation;
  xiiNetworkIORegistration            m_Registration;
  xiiNetworkIOOperationKind           m_Kind = xiiNetworkIOOperationKind::Receive;
  xiiNetworkError                     m_Error;
  xiiUInt64                           m_uiUserData         = 0;
  xiiUInt32                           m_uiBytesTransferred = 0;
  xiiNetworkIOBuffers                 m_Buffers;
  xiiNetworkEndpoint                  m_Endpoint;
  xiiTcpSocket                        m_AcceptedSocket;
  xiiDynamicArray<xiiNetworkEndpoint> m_ResolvedEndpoints;
  [[nodiscard]] bool                  Succeeded() const { return m_Error.Succeeded(); }
  [[nodiscard]] bool                  Failed() const { return m_Error.Failed(); }
};
