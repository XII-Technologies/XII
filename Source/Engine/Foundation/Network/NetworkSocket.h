/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkEndpoint.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/ArrayPtr.h>

class xiiNetworkSocketAccess;
class xiiTcpListener;

enum class xiiNetworkSocketState : xiiUInt8
{
  Closed,
  Open,
  Connecting,
  Connected,
  Listening,
  Failed
};
enum class xiiNetworkSocketEvent : xiiUInt8
{
  Readable,
  Writable
};
enum class xiiNetworkShutdown : xiiUInt8
{
  Receive,
  Send,
  Both
};

/// Shared native socket ownership and typed configuration; no virtual dispatch or heap storage.
/// Sockets are move-only. Every access to the same object must be externally synchronized, including
/// I/O, moves, native borrowing, Close and destruction. Distinct objects may be used independently.
/// Never close/move a socket while an operation or external I/O registration still references it.
class XII_FOUNDATION_DLL xiiNetworkSocket
{
public:
  xiiNetworkSocket(const xiiNetworkSocket&)            = delete;
  xiiNetworkSocket& operator=(const xiiNetworkSocket&) = delete;

  /// Idempotent. Releases ownership, even on a native close error. No retry on POSIX EINTR (fd reuse).
  /// Windows close failures retain ownership for a subsequent retry; positive linger is never enabled.
  [[nodiscard]] xiiNetworkError         Close();
  [[nodiscard]] bool                    IsOpen() const { return m_uiNativeHandle != InvalidHandle; }
  [[nodiscard]] bool                    IsNonBlocking() const { return m_bNonBlocking; }
  [[nodiscard]] xiiNetworkAddressFamily GetFamily() const { return m_Family; }
  /// Local lifecycle state, not a probe of remote peer health. Receive reports remote half-close.
  [[nodiscard]] xiiNetworkSocketState GetState() const { return m_State; }
  [[nodiscard]] xiiNetworkError       GetLocalEndpoint(xiiNetworkEndpoint& out_endpoint) const;
  [[nodiscard]] xiiNetworkError       GetPeerEndpoint(xiiNetworkEndpoint& out_endpoint) const;
  [[nodiscard]] xiiNetworkError       SetNonBlocking(bool bNonBlocking);
  /// Set before bind. Windows reuse semantics differ: do not use this for exclusive server ownership.
  [[nodiscard]] xiiNetworkError SetAddressReuse(bool bEnable);
  /// Must be set before binding an IPv6 socket. The default at Open is IPv6-only on every platform.
  [[nodiscard]] xiiNetworkError SetIpv6Only(bool bEnable);
  [[nodiscard]] xiiNetworkError SetSendBufferSize(xiiUInt32 uiBytes);
  [[nodiscard]] xiiNetworkError SetReceiveBufferSize(xiiUInt32 uiBytes);
  [[nodiscard]] xiiNetworkError GetBufferSizes(xiiUInt32& out_uiSendBytes, xiiUInt32& out_uiReceiveBytes) const;
  /// Zero disables the timeout. Applies to blocking I/O only, not connect, DNS or Wait.
  /// A timeout reported as WouldBlock by the native backend remains WouldBlock.
  [[nodiscard]] xiiNetworkError SetSendTimeout(xiiTime timeout);
  [[nodiscard]] xiiNetworkError SetReceiveTimeout(xiiTime timeout);
  /// One bounded readiness wait. Zero polls; negative/nonfinite/over-INT_MAX milliseconds is invalid.
  /// Readiness includes hangup/error and does not guarantee a subsequent transfer will succeed.
  /// Interrupted is returned to the caller; no internal retry can extend the deadline.
  [[nodiscard]] xiiNetworkError Wait(xiiNetworkSocketEvent event, xiiTime timeout) const;

protected:
  xiiNetworkSocket() = default;
  ~xiiNetworkSocket();
  xiiNetworkSocket(xiiNetworkSocket&& other) noexcept;
  xiiNetworkSocket& operator=(xiiNetworkSocket&& other) noexcept;

  [[nodiscard]] xiiNetworkError           OpenSocket(xiiNetworkAddressFamily family, bool bDatagram, bool bNonBlocking);
  [[nodiscard]] xiiNetworkError           BindSocket(const xiiNetworkEndpoint& endpoint);
  [[nodiscard]] xiiNetworkError           ConnectSocket(const xiiNetworkEndpoint& endpoint);
  [[nodiscard]] xiiNetworkError           FinishConnection();
  [[nodiscard]] xiiNetworkOperationResult SendBuffers(xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> buffers, const xiiNetworkEndpoint* pDestination);
  [[nodiscard]] xiiNetworkOperationResult ReceiveBuffers(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>> buffers, xiiNetworkEndpoint* out_pSender);

  xiiNetworkSocketState m_State = xiiNetworkSocketState::Closed;
  xiiNetworkError       m_ConnectError;

private:
  friend class xiiNetworkSocketAccess;
  friend class xiiTcpListener;
  static constexpr xiiUInt64 InvalidHandle    = 0xFFFFFFFFFFFFFFFFULL;
  xiiUInt64                  m_uiNativeHandle = InvalidHandle;
  xiiNetworkAddressFamily    m_Family         = xiiNetworkAddressFamily::Unspecified;
  bool                       m_bNonBlocking   = false;
  bool                       m_bDatagram      = false;
};
