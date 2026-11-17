/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkEndpoint.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <Foundation/Network/Transport/NetworkTransport.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>

/// Borrowed logical identity. Host and generation checks prevent cross-host and recycled-slot use.
struct XII_FOUNDATION_DLL xiiNetworkConnection
{
  xiiUInt64          m_uiHost       = 0;
  xiiUInt64          m_uiGeneration = 0;
  xiiUInt32          m_uiSlot       = 0;
  [[nodiscard]] bool IsValid() const { return m_uiHost != 0 && m_uiGeneration != 0; }
  bool               operator==(const xiiNetworkConnection&) const = default;
};

struct XII_FOUNDATION_DLL xiiNetworkListener
{
  xiiUInt64          m_uiHost       = 0;
  xiiUInt64          m_uiGeneration = 0;
  xiiUInt32          m_uiSlot       = 0;
  [[nodiscard]] bool IsValid() const { return m_uiHost != 0 && m_uiGeneration != 0; }
  bool               operator==(const xiiNetworkListener&) const = default;
};

struct XII_FOUNDATION_DLL xiiNetworkMessage
{
  xiiUInt64          m_uiHost       = 0;
  xiiUInt64          m_uiGeneration = 0;
  xiiUInt32          m_uiSlot       = 0;
  [[nodiscard]] bool IsValid() const { return m_uiHost != 0 && m_uiGeneration != 0; }
  bool               operator==(const xiiNetworkMessage&) const = default;
};

enum class xiiNetworkConnectionState : xiiUInt8
{
  Resolving,
  Connecting,
  Securing,
  Negotiating,
  Authenticating,
  Connected,
  Draining,
  Reconnecting,
  Closed
};

struct XII_FOUNDATION_DLL xiiNetworkConnectionStatistics
{
  xiiUInt64 m_uiMessagesSent              = 0;
  xiiUInt64 m_uiMessagesReceived          = 0;
  xiiUInt64 m_uiPayloadBytesSent          = 0;
  xiiUInt64 m_uiPayloadBytesReceived      = 0;
  xiiUInt64 m_uiWireBytesSent             = 0;
  xiiUInt64 m_uiWireBytesReceived         = 0;
  xiiUInt64 m_uiQueuedSendBytes           = 0;
  xiiUInt64 m_uiLocallyDroppedDatagrams   = 0;
  xiiUInt64 m_uiProviderReceiveQueueDrops = 0;
  xiiUInt64 m_uiStaleDatagrams            = 0;
  xiiUInt64 m_uiBackpressureEvents        = 0;
  /// Measured application ping round trip, distinct from provider congestion estimates.
  xiiTime   m_HeartbeatRoundTrip;
  bool      m_bHasNativeCongestionStatistics = false;
  xiiTime   m_NativeSmoothedRoundTrip;
  xiiUInt64 m_uiNativeLostPackets           = 0;
  xiiUInt32 m_uiNativeCongestionWindowBytes = 0;
  xiiUInt64 m_uiNativeReorderedPackets      = 0;
};

struct XII_FOUNDATION_DLL xiiNetworkConnectionInfo
{
  xiiTime         m_EstablishmentTime;
  xiiNetworkError m_TerminalError;

  xiiNetworkConnectionState       m_State = xiiNetworkConnectionState::Closed;
  xiiNetworkEndpoint              m_LocalEndpoint;
  xiiNetworkEndpoint              m_RemoteEndpoint;
  /// On Local connections this names the listener service on both peers.
  xiiNetworkLocalEndpoint         m_LocalServiceEndpoint;
  xiiNetworkTransportCapabilities m_Capabilities;
  xiiNetworkConnectionStatistics  m_Statistics;
  xiiUuid                         m_Session;
  xiiString                       m_sAuthenticatedIdentity;
  xiiUInt64                       m_uiNegotiatedFeatures  = 0;
  xiiUInt32                       m_uiProtocolVersion     = 0;
  xiiUInt32                       m_uiMaximumMessageBytes = 0;
  xiiUInt32                       m_uiReconnectAttempts   = 0;
  bool                            m_bIncoming             = false;
  bool                            m_bSessionResumed       = false;
};
