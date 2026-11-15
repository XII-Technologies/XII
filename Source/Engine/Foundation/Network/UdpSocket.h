/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkSocket.h>

/// Owning datagram socket. Each Send is exactly one datagram, including empty sends.
/// Each Receive consumes one datagram; Truncated preserves the copied count and sender, discarding its tail.
/// Memberships belong to the native socket and are dropped on Close (or explicitly with LeaveMulticast).
class XII_FOUNDATION_DLL xiiUdpSocket final : public xiiNetworkSocket
{
public:
  ~xiiUdpSocket()                                                        = default;
  xiiUdpSocket(const xiiUdpSocket&)                                      = delete;
  xiiUdpSocket& operator=(const xiiUdpSocket&)                           = delete;
  xiiUdpSocket()                                                         = default;
  xiiUdpSocket(xiiUdpSocket&& other) noexcept                            = default;
  xiiUdpSocket&                 operator=(xiiUdpSocket&& other) noexcept = default;
  [[nodiscard]] xiiNetworkError Open(xiiNetworkAddressFamily family, bool bNonBlocking = true);
  [[nodiscard]] xiiNetworkError Bind(const xiiNetworkEndpoint& endpoint);
  /// Selects a peer and filters incoming datagrams. Close/Open to disconnect portably.
  [[nodiscard]] xiiNetworkError Connect(const xiiNetworkEndpoint& endpoint);
  /// Completes a Connect that returned InProgress.
  [[nodiscard]] xiiNetworkError           FinishConnect();
  [[nodiscard]] xiiNetworkOperationResult SendTo(const xiiNetworkEndpoint& destination, xiiArrayPtr<const xiiUInt8> bytes);
  [[nodiscard]] xiiNetworkOperationResult SendToGather(const xiiNetworkEndpoint& destination, xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> buffers);
  [[nodiscard]] xiiNetworkOperationResult Send(xiiArrayPtr<const xiiUInt8> bytes);
  [[nodiscard]] xiiNetworkOperationResult ReceiveFrom(xiiArrayPtr<xiiUInt8> out_bytes, xiiNetworkEndpoint& out_sender);
  [[nodiscard]] xiiNetworkOperationResult ReceiveFromScatter(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>> out_buffers, xiiNetworkEndpoint& out_sender);
  [[nodiscard]] xiiNetworkError           SetBroadcast(bool bEnable);
  /// IPv4 interface is a local IPv4 address (wildcard selects default). IPv6 uses an interface index.
  [[nodiscard]] xiiNetworkError JoinMulticast(const xiiNetworkAddress& group, const xiiNetworkAddress& ipv4Interface = xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily::Ipv4), xiiUInt32 uiIpv6InterfaceIndex = 0);
  [[nodiscard]] xiiNetworkError LeaveMulticast(const xiiNetworkAddress& group, const xiiNetworkAddress& ipv4Interface = xiiNetworkAddress::MakeWildcard(xiiNetworkAddressFamily::Ipv4), xiiUInt32 uiIpv6InterfaceIndex = 0);
  [[nodiscard]] xiiNetworkError SetMulticastInterface(const xiiNetworkAddress& ipv4Interface, xiiUInt32 uiIpv6InterfaceIndex = 0);
  [[nodiscard]] xiiNetworkError SetMulticastHopLimit(xiiUInt8 uiHops);
  [[nodiscard]] xiiNetworkError SetMulticastLoopback(bool bEnable);
};
