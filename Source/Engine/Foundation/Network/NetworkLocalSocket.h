/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/Strings/String.h>

namespace xiiNetworkIOInternal
{
  struct LocalSocketAccess;
}

/// Exclusive ownership of a local stream returned by Accept. Register it with the accepting
/// I/O context before submitting I/O. Destroying an unregistered stream closes it.
class XII_FOUNDATION_DLL xiiNetworkLocalSocket
{
public:
  xiiNetworkLocalSocket() = default;
  ~xiiNetworkLocalSocket();
  xiiNetworkLocalSocket(xiiNetworkLocalSocket&& other) noexcept;
  xiiNetworkLocalSocket& operator=(xiiNetworkLocalSocket&& other) noexcept;
  xiiNetworkLocalSocket(const xiiNetworkLocalSocket&)            = delete;
  xiiNetworkLocalSocket& operator=(const xiiNetworkLocalSocket&) = delete;

  [[nodiscard]] bool                           IsOpen() const { return m_bOpen; }
  [[nodiscard]] const xiiNetworkLocalEndpoint& GetEndpoint() const { return m_Endpoint; }
  void                                         Close();

private:
  friend struct xiiNetworkIOInternal::LocalSocketAccess;

  static constexpr xiiUInt64 InvalidHandle = ~xiiUInt64{0};
  xiiNetworkLocalEndpoint    m_Endpoint;
  xiiString                  m_sPeerIdentity;
  xiiUInt64                  m_uiNativeHandle    = InvalidHandle;
  xiiUInt64                  m_uiContextId       = 0;
  xiiUInt64                  m_uiDirectoryHandle = InvalidHandle;
  xiiUInt64                  m_uiSocketDevice    = 0;
  xiiUInt64                  m_uiSocketInode     = 0;
  xiiUInt32                  m_uiWorker          = 0;
  bool                       m_bOpen             = false;
  bool                       m_bListener         = false;
  bool                       m_bConnected        = false;
};
