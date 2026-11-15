/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/TcpSocket.h>

/// Owning TCP listener. Accepted sockets have independent ownership, explicit mode and no inheritance.
class XII_FOUNDATION_DLL xiiTcpListener final : public xiiNetworkSocket
{
public:
  ~xiiTcpListener()                                                        = default;
  xiiTcpListener(const xiiTcpListener&)                                    = delete;
  xiiTcpListener& operator=(const xiiTcpListener&)                         = delete;
  xiiTcpListener()                                                         = default;
  xiiTcpListener(xiiTcpListener&& other) noexcept                          = default;
  xiiTcpListener&               operator=(xiiTcpListener&& other) noexcept = default;
  [[nodiscard]] xiiNetworkError Open(xiiNetworkAddressFamily family, bool bNonBlocking = true);
  [[nodiscard]] xiiNetworkError Bind(const xiiNetworkEndpoint& endpoint);
  [[nodiscard]] xiiNetworkError Listen(xiiUInt32 uiBacklog = 128);
  /// Output must be closed; unchanged on failure. Peer endpoint optional. Does not queue/buffer clients.
  [[nodiscard]] xiiNetworkError Accept(xiiTcpSocket& out_socket, xiiNetworkEndpoint* out_pPeer = nullptr, bool bNonBlocking = true);
};
