/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Network/NetworkEndpoint.h>

class xiiStringBuilder;

enum class xiiNetworkTransport : xiiUInt8
{
  Tcp,
  Udp
};

/// Synchronous OS resolver; calls may block. No global initialization is required.
class XII_FOUNDATION_DLL xiiNetworkResolver
{
public:
  /// Resolves a hostname and numeric port or service name. Empty host resolves wildcard addresses.
  /// Returns unique IPv4/IPv6 endpoints in OS preference order, without AI_ADDRCONFIG filtering.
  /// The output is cleared on failure. No external connectivity or IPv4 preference is assumed.
  [[nodiscard]] static xiiNetworkError Resolve(xiiStringView sHost, xiiStringView sService,
                                               xiiDynamicArray<xiiNetworkEndpoint>& out_endpoints, xiiNetworkAddressFamily family = xiiNetworkAddressFamily::Unspecified,
                                               xiiNetworkTransport transport = xiiNetworkTransport::Tcp);
  /// Requires a reverse name (does not silently return a numeric address). Output unchanged on failure.
  [[nodiscard]] static xiiNetworkError ReverseLookup(const xiiNetworkEndpoint& endpoint, xiiStringBuilder& out_sHostname);
};
