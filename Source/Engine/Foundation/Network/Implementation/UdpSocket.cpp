/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/UdpSocket.h>

xiiNetworkError           xiiUdpSocket::Open(xiiNetworkAddressFamily family, bool bNonBlocking) { return OpenSocket(family, true, bNonBlocking); }
xiiNetworkError           xiiUdpSocket::Bind(const xiiNetworkEndpoint& endpoint) { return BindSocket(endpoint); }
xiiNetworkError           xiiUdpSocket::Connect(const xiiNetworkEndpoint& endpoint) { return ConnectSocket(endpoint); }
xiiNetworkOperationResult xiiUdpSocket::SendTo(const xiiNetworkEndpoint& destination, xiiArrayPtr<const xiiUInt8> bytes) { return SendToGather(destination, xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>>(&bytes, 1)); }
xiiNetworkOperationResult xiiUdpSocket::SendToGather(const xiiNetworkEndpoint& destination, xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> buffers) { return SendBuffers(buffers, &destination); }
xiiNetworkOperationResult xiiUdpSocket::Send(xiiArrayPtr<const xiiUInt8> bytes) { return SendBuffers(xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>>(&bytes, 1), nullptr); }
xiiNetworkOperationResult xiiUdpSocket::ReceiveFrom(xiiArrayPtr<xiiUInt8> out_bytes, xiiNetworkEndpoint& out_sender) { return ReceiveFromScatter(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>>(&out_bytes, 1), out_sender); }
xiiNetworkOperationResult xiiUdpSocket::ReceiveFromScatter(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>> out_buffers, xiiNetworkEndpoint& out_sender) { return ReceiveBuffers(out_buffers, &out_sender); }

xiiNetworkError xiiUdpSocket::FinishConnect() { return FinishConnection(); }
