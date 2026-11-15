/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkSocket.h>

/// Owning TCP stream. Open defaults to nonblocking; Connect never performs hidden resolution/retries.
class XII_FOUNDATION_DLL xiiTcpSocket final : public xiiNetworkSocket
{
public:
  ~xiiTcpSocket()                                                        = default;
  xiiTcpSocket(const xiiTcpSocket&)                                      = delete;
  xiiTcpSocket& operator=(const xiiTcpSocket&)                           = delete;
  xiiTcpSocket()                                                         = default;
  xiiTcpSocket(xiiTcpSocket&& other) noexcept                            = default;
  xiiTcpSocket&                 operator=(xiiTcpSocket&& other) noexcept = default;
  [[nodiscard]] xiiNetworkError Open(xiiNetworkAddressFamily family, bool bNonBlocking = true);
  [[nodiscard]] xiiNetworkError Bind(const xiiNetworkEndpoint& endpoint);
  /// InProgress is expected for nonblocking connect. Failed connects require Close and Open before retry.
  [[nodiscard]] xiiNetworkError Connect(const xiiNetworkEndpoint& endpoint);
  /// Polls readiness before reading SO_ERROR. InProgress means pending; final errors are retained.
  [[nodiscard]] xiiNetworkError FinishConnect();
  /// Each operation may transfer fewer bytes than requested. Empty TCP buffers succeed with zero bytes.
  /// A nonempty receive returning ConnectionClosed means orderly EOF, distinct from an empty receive.
  [[nodiscard]] xiiNetworkOperationResult Send(xiiArrayPtr<const xiiUInt8> bytes);
  [[nodiscard]] xiiNetworkOperationResult Receive(xiiArrayPtr<xiiUInt8> out_bytes);
  /// Up to 64 spans per call, with aggregate size <= INT_MAX. No allocation or coalescing.
  [[nodiscard]] xiiNetworkOperationResult SendGather(xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> buffers);
  [[nodiscard]] xiiNetworkOperationResult ReceiveScatter(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>> out_buffers);
  [[nodiscard]] xiiNetworkError           Shutdown(xiiNetworkShutdown direction);
  [[nodiscard]] xiiNetworkError           SetNoDelay(bool bEnable);
  [[nodiscard]] xiiNetworkError           SetKeepAlive(bool bEnable);
  /// Positive whole seconds. UnsupportedOperation if the backend cannot configure all three values.
  /// Multiple native options are applied in order; failure can leave earlier options applied.
  [[nodiscard]] xiiNetworkError SetKeepAliveTiming(xiiTime idleTime, xiiTime interval, xiiUInt32 uiProbeCount);
  /// true selects abortive close (discard queued output); false restores ordinary OS-managed close.
  /// Positive linger timeouts are excluded to keep nonblocking close safe and destruction bounded.
  [[nodiscard]] xiiNetworkError SetLinger(bool bDiscardPendingData);
};
