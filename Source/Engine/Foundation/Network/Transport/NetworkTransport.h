/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

/// Selection is per host. NativeUdp is connectionless and does not establish application sessions.
enum class xiiNetworkTransportType : xiiUInt8
{
  Tcp,
  TlsTcp,
  NativeUdp,
  SecureDatagram,
  Local
};

enum class xiiNetworkDeliveryMode : xiiUInt8
{
  ReliableOrdered,
  ReliableUnordered,
  UnreliableSequenced,
  UnreliableUnsequenced
};

[[nodiscard]] constexpr xiiUInt32 xiiNetworkDeliveryMask(xiiNetworkDeliveryMode mode)
{
  return mode <= xiiNetworkDeliveryMode::UnreliableUnsequenced ? (1U << static_cast<xiiUInt32>(mode)) : 0U;
}

struct XII_FOUNDATION_DLL xiiNetworkTransportCapabilities
{
  xiiUInt32          m_uiDeliveryModes             = 0;
  xiiUInt32          m_uiMaximumUnreliablePayload  = 0;
  bool               m_bEncrypted                  = false;
  bool               m_bAuthenticated              = false;
  bool               m_bConnectionless             = false;
  bool               m_bIndependentChannelProgress = false;
  bool               m_bNativeCongestionStatistics = false;
  [[nodiscard]] bool Supports(xiiNetworkDeliveryMode mode) const { return (m_uiDeliveryModes & xiiNetworkDeliveryMask(mode)) != 0; }
};
