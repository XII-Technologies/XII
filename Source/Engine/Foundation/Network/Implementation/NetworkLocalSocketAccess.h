/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/NetworkLocalSocket.h>
#include <Foundation/Strings/String.h>

XII_FOUNDATION_INTERNAL_HEADER

namespace xiiNetworkIOInternal
{
  struct LocalSocketAccess
  {
    static xiiNetworkError Open(xiiNetworkLocalSocket& ref_socket);
    static xiiNetworkError Listen(xiiNetworkLocalSocket& ref_socket, const xiiNetworkLocalEndpoint& endpoint, xiiInt32 iBacklog);
    static xiiNetworkError PeerIdentity(const xiiNetworkLocalSocket& socket, xiiString& out_sIdentity);
    static void            Close(xiiNetworkLocalSocket& ref_socket);

    static xiiUInt64& Native(xiiNetworkLocalSocket& ref_socket) { return ref_socket.m_uiNativeHandle; }
    static xiiUInt64  Native(const xiiNetworkLocalSocket& socket) { return socket.m_uiNativeHandle; }
    static void       SetEndpoint(xiiNetworkLocalSocket& ref_socket, const xiiNetworkLocalEndpoint& endpoint) { ref_socket.m_Endpoint = endpoint; }
    static void       SetConnected(xiiNetworkLocalSocket& ref_socket) { ref_socket.m_bOpen = ref_socket.m_bConnected = true; }
    static bool       IsConnected(const xiiNetworkLocalSocket& socket) { return socket.m_bConnected; }
    static bool       IsListener(const xiiNetworkLocalSocket& socket) { return socket.m_bListener; }
    static xiiString& Identity(xiiNetworkLocalSocket& ref_socket) { return ref_socket.m_sPeerIdentity; }
    static xiiUInt64  GetContext(const xiiNetworkLocalSocket& socket) { return socket.m_uiContextId; }
    static xiiUInt32  GetWorker(const xiiNetworkLocalSocket& socket) { return socket.m_uiWorker; }
    static void       SetAffinity(xiiNetworkLocalSocket& ref_socket, xiiUInt64 uiContext, xiiUInt32 uiWorker)
    {
      ref_socket.m_uiContextId = uiContext;
      ref_socket.m_uiWorker    = uiWorker;
    }
  };
} // namespace xiiNetworkIOInternal
