/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkConnection.h>

/// Wire identifier contains a channel slot and generation. Connection identity scopes every lookup.
struct XII_FOUNDATION_DLL xiiNetworkChannel
{
  xiiNetworkConnection m_Connection;
  xiiUInt32            m_uiId                = 0;
  xiiUInt64            m_uiSessionGeneration = 0;
  [[nodiscard]] bool   IsValid() const { return m_Connection.IsValid() && m_uiId != 0 && m_uiSessionGeneration != 0; }
  bool                 operator==(const xiiNetworkChannel&) const = default;
};

struct XII_FOUNDATION_DLL xiiNetworkChannelConfiguration
{
  xiiNetworkDeliveryMode m_Delivery = xiiNetworkDeliveryMode::ReliableOrdered;
  /// Weighted fair scheduling uses weights 1..16. Underlying TCP channels still share stream HOL.
  xiiUInt8  m_uiWeight                = 1;
  xiiUInt32 m_uiMaximumQueuedMessages = 128;
  xiiUInt64 m_uiMaximumQueuedBytes    = 2 * 1024 * 1024;
  /// Application-defined service/category identifier for channel authorization.
  xiiUInt32 m_uiService = 0;
};

struct XII_FOUNDATION_DLL xiiNetworkChannelStatistics
{
  xiiUInt64 m_uiMessagesSent     = 0;
  xiiUInt64 m_uiMessagesReceived = 0;
  xiiUInt64 m_uiQueuedBytes      = 0;
  xiiUInt32 m_uiQueuedMessages   = 0;
  xiiUInt64 m_uiStaleMessages    = 0;
};
