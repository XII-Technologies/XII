/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/TcpSocket.h>

xiiNetworkError           xiiTcpSocket::Open(xiiNetworkAddressFamily family, bool bNonBlocking) { return OpenSocket(family, false, bNonBlocking); }
xiiNetworkError           xiiTcpSocket::Bind(const xiiNetworkEndpoint& endpoint) { return BindSocket(endpoint); }
xiiNetworkOperationResult xiiTcpSocket::Send(xiiArrayPtr<const xiiUInt8> bytes) { return SendGather(xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>>(&bytes, 1)); }
xiiNetworkOperationResult xiiTcpSocket::Receive(xiiArrayPtr<xiiUInt8> bytes) { return ReceiveScatter(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>>(&bytes, 1)); }
xiiNetworkOperationResult xiiTcpSocket::SendGather(xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> buffers) { return SendBuffers(buffers, nullptr); }
xiiNetworkOperationResult xiiTcpSocket::ReceiveScatter(xiiArrayPtr<const xiiArrayPtr<xiiUInt8>> buffers) { return ReceiveBuffers(buffers, nullptr); }

xiiNetworkError xiiTcpSocket::FinishConnect() { return FinishConnection(); }
