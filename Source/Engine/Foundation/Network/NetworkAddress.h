/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Network/NetworkError.h>
#include <Foundation/Strings/StringView.h>
#include <Foundation/Types/ArrayPtr.h>

class xiiStringBuilder;

enum class xiiNetworkAddressFamily : xiiUInt8
{
  Unspecified,
  Ipv4,
  Ipv6
};

/// Binary address value. Unspecified is invalid; IPv4/IPv6 wildcards are valid and distinct.
/// IPv4-mapped IPv6 addresses remain IPv6. Scope identifiers participate in equality and hashing.
class XII_FOUNDATION_DLL xiiNetworkAddress
{
public:
  xiiNetworkAddress() = default;
  [[nodiscard]] static xiiNetworkAddress MakeWildcard(xiiNetworkAddressFamily family);
  [[nodiscard]] static xiiNetworkAddress MakeLoopback(xiiNetworkAddressFamily family);
  /// Accepts exactly 4 or 16 network-order bytes. Nonzero scopes are valid only for IPv6.
  [[nodiscard]] static xiiNetworkError FromBytes(xiiArrayPtr<const xiiUInt8> bytes, xiiUInt32 uiScopeId, xiiNetworkAddress& out_address);
  /// Numeric literals only, with optional IPv6 %numeric-index or %interface-name. Output unchanged on failure.
  [[nodiscard]] static xiiNetworkError Parse(xiiStringView sText, xiiNetworkAddress& out_address);
  /// Canonical numeric representation; scopes are formatted as numeric indices.
  void ToString(xiiStringBuilder& out_sText) const;

  [[nodiscard]] bool                        IsValid() const { return m_Family != xiiNetworkAddressFamily::Unspecified; }
  [[nodiscard]] bool                        IsWildcard() const;
  [[nodiscard]] bool                        IsLoopback() const;
  [[nodiscard]] bool                        IsMulticast() const;
  [[nodiscard]] xiiNetworkAddressFamily     GetFamily() const { return m_Family; }
  [[nodiscard]] xiiUInt32                   GetScopeId() const { return m_uiScopeId; }
  [[nodiscard]] xiiArrayPtr<const xiiUInt8> GetBytes() const;
  [[nodiscard]] xiiUInt32                   ComputeHash() const;
  [[nodiscard]] bool                        operator==(const xiiNetworkAddress& other) const;

private:
  xiiUInt8                m_uiBytes[16] = {};
  xiiUInt32               m_uiScopeId   = 0;
  xiiNetworkAddressFamily m_Family      = xiiNetworkAddressFamily::Unspecified;
};

template <>
struct xiiHashHelper<xiiNetworkAddress>
{
  static xiiUInt32 Hash(const xiiNetworkAddress& address) { return address.ComputeHash(); }
  static bool      Equal(const xiiNetworkAddress& left, const xiiNetworkAddress& right) { return left == right; }
};
