/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkSocket.h>

XII_FOUNDATION_INTERNAL_HEADER

/// Internal interoperability only. The returned value is borrowed, never duplicated or released.
/// Keep the owner immobile and alive until registration is removed AND all completions are drained.
/// A recycled native value is not an identity: an I/O context must maintain its own generation/token.
/// Native clients must not close the handle, alter blocking mode, or consume SO_ERROR independently.
/// Windows sockets are created overlapped-capable. External operations must retain their buffers and
/// serialize access to the owning object; cancellation does not itself establish completion/lifetime.
class xiiNetworkSocketAccess
{
public:
  static void SetState(xiiNetworkSocket& socket, xiiNetworkSocketState state, xiiNetworkError error = {})
  {
    socket.m_State        = state;
    socket.m_ConnectError = error;
  }
  static xiiUInt64 BorrowNativeHandle(const xiiNetworkSocket& socket) { return socket.m_uiNativeHandle; }
};
