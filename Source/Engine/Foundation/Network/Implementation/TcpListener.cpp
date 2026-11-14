/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/TcpListener.h>

xiiNetworkError xiiTcpListener::Open(xiiNetworkAddressFamily family, bool bNonBlocking) { return OpenSocket(family, false, bNonBlocking); }
xiiNetworkError xiiTcpListener::Bind(const xiiNetworkEndpoint& endpoint) { return BindSocket(endpoint); }
