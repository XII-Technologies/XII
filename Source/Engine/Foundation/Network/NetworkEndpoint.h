/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Network/NetworkAddress.h>

/// Address and host-order 16-bit port. Port zero requests an ephemeral port when binding.
class XII_FOUNDATION_DLL xiiNetworkEndpoint
{
public:
  xiiNetworkEndpoint() = default;
  xiiNetworkEndpoint(const xiiNetworkAddress& address, xiiUInt16 uiPort) : m_Address(address), m_uiPort(uiPort) {}
  /// Accepts a.b.c.d:port or [IPv6%scope]:port. Rejects unbracketed IPv6 and ports above 65535.
  [[nodiscard]] static xiiNetworkError   Parse(xiiStringView sText, xiiNetworkEndpoint& out_endpoint);
  void                                   ToString(xiiStringBuilder& out_sText) const;
  [[nodiscard]] const xiiNetworkAddress& GetAddress() const { return m_Address; }
  [[nodiscard]] xiiUInt16                GetPort() const { return m_uiPort; }
  [[nodiscard]] bool                     IsValid() const { return m_Address.IsValid(); }
  [[nodiscard]] bool                     operator==(const xiiNetworkEndpoint& other) const { return m_uiPort == other.m_uiPort && m_Address == other.m_Address; }
  [[nodiscard]] xiiUInt32                ComputeHash() const;

private:
  xiiNetworkAddress m_Address;
  xiiUInt16         m_uiPort = 0;
};

template <>
struct xiiHashHelper<xiiNetworkEndpoint>
{
  static xiiUInt32 Hash(const xiiNetworkEndpoint& endpoint) { return endpoint.ComputeHash(); }
  static bool      Equal(const xiiNetworkEndpoint& left, const xiiNetworkEndpoint& right) { return left == right; }
};
