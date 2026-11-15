/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/NetworkChannel.h>
#include <Foundation/Network/NetworkIOConfiguration.h>
#include <Foundation/Network/NetworkIOStatistics.h>
#include <Foundation/Network/Security/NetworkSecurityConfig.h>
#include <Foundation/Network/Session/NetworkSession.h>

struct XII_FOUNDATION_DLL xiiNetworkTcpOptions
{
  bool m_bNoDelay              = true;
  bool m_bKeepAlive            = false;
  bool m_bReuseListenerAddress = false;
  /// Zero leaves native defaults. Options are applied before native registration.
  xiiUInt32 m_uiSendBufferBytes    = 0;
  xiiUInt32 m_uiReceiveBufferBytes = 0;
};

/// Observation of host queues and lifetime totals. Individual counters may advance during sampling.
struct XII_FOUNDATION_DLL xiiNetworkHostStatistics
{
  xiiUInt32              m_uiActiveConnections      = 0;
  xiiUInt32              m_uiActiveSessions         = 0;
  xiiUInt32              m_uiActiveChannels         = 0;
  xiiUInt32              m_uiActiveListeners        = 0;
  xiiUInt32              m_uiQueuedEvents           = 0;
  xiiUInt32              m_uiReservedEventSlots     = 0;
  xiiUInt64              m_uiQueuedMessages         = 0;
  xiiUInt64              m_uiQueuedSendBytes        = 0;
  xiiUInt64              m_uiReservedReceiveBytes   = 0;
  xiiUInt64              m_uiConnectionsEstablished = 0;
  xiiUInt64              m_uiAuthenticationFailures = 0;
  xiiUInt64              m_uiProtocolFailures       = 0;
  xiiUInt64              m_uiTimeouts               = 0;
  xiiUInt64              m_uiBackpressureEvents     = 0;
  xiiUInt64              m_uiMessagesSent           = 0;
  xiiUInt64              m_uiMessagesReceived       = 0;
  xiiUInt64              m_uiPayloadBytesSent       = 0;
  xiiUInt64              m_uiPayloadBytesReceived   = 0;
  xiiUInt64              m_uiDroppedDatagrams       = 0;
  xiiNetworkIOStatistics m_IO;
};

struct XII_FOUNDATION_DLL xiiNetworkHostConfiguration
{
  xiiNetworkTransportType             m_Transport = xiiNetworkTransportType::TlsTcp;
  xiiSharedPtr<xiiNetworkCredentials> m_pCredentials;
  xiiNetworkAuthenticationPolicy      m_Authentication                 = xiiNetworkAuthenticationPolicy::Certificate;
  bool                                m_bAllowInsecureDevelopment      = false;
  bool                                m_bRequireChannelAuthorization   = false;
  xiiUuid                             m_Protocol                       = xiiUuid(0x5849494E45545731ULL, 0x50524F544F434F4CULL);
  xiiUInt32                           m_uiMinimumProtocolVersion       = 1;
  xiiUInt32                           m_uiMaximumProtocolVersion       = 1;
  xiiUInt64                           m_uiSupportedFeatures            = 0;
  xiiUInt64                           m_uiRequiredFeatures             = 0;
  xiiUInt32                           m_uiMaximumConnections           = 128;
  xiiUInt32                           m_uiMaximumListeners             = 4;
  xiiUInt32                           m_uiMaximumChannelsPerConnection = 64;
  xiiUInt32                           m_uiMaximumMessageBytes          = 1024 * 1024;
  xiiUInt32                           m_uiMaximumQueuedMessages        = 2048;
  xiiUInt32                           m_uiMaximumQueuedEvents          = 8192;
  xiiUInt64                           m_uiMaximumPeerSendBytes         = 8 * 1024 * 1024;
  xiiUInt64                           m_uiMaximumPeerReceiveBytes      = 8 * 1024 * 1024;
  xiiUInt64                           m_uiMaximumReceiveBytes          = 32 * 1024 * 1024;
  /// Zero disables rate limiting. Burst must cover a complete maximum-size message.
  xiiUInt64                 m_uiSendBytesPerSecond     = 0;
  xiiUInt64                 m_uiSendBurstBytes         = 2 * 1024 * 1024;
  xiiUInt64                 m_uiPeerSendBytesPerSecond = 0;
  xiiUInt64                 m_uiPeerSendBurstBytes     = 2 * 1024 * 1024;
  xiiTime                   m_HandshakeTimeout         = xiiTime::MakeFromSeconds(10);
  xiiTime                   m_AuthenticationTimeout    = xiiTime::MakeFromSeconds(10);
  xiiTime                   m_IdleTimeout              = xiiTime::MakeFromSeconds(30);
  xiiTime                   m_HeartbeatInterval        = xiiTime::MakeFromSeconds(5);
  xiiTime                   m_CloseTimeout             = xiiTime::MakeFromSeconds(2);
  bool                      m_bEnableSessionRecovery   = false;
  xiiUInt32                 m_uiMaximumSessions        = 256;
  xiiTime                   m_SessionRecoveryWindow    = xiiTime::MakeFromSeconds(30);
  xiiTime                   m_MaximumSessionLifetime   = xiiTime::MakeFromSeconds(3600);
  xiiNetworkTcpOptions      m_Tcp;
  xiiNetworkIOConfiguration m_IO;
};

enum class xiiNetworkEventType : xiiUInt8
{
  Connected,
  ListenerClosed,
  Closed,
  ReconnectScheduled,
  AuthenticationRequired,
  ChannelRequested,
  ChannelOpened,
  ChannelClosed,
  Message,
  MessageSent,
  Datagram
};

/// Caller-owned, move-only event. No application callback executes on native I/O/provider workers.
/// MessageSent means the transport released its payload; it never promises application delivery.
/// Message/Datagram payload leases remain valid beyond host shutdown. Events retain FIFO publication
/// order; no received application message is published after the connection's Closed event.
struct XII_FOUNDATION_DLL xiiNetworkEvent
{
  xiiNetworkEvent()                                                            = default;
  xiiNetworkEvent(xiiNetworkEvent&&) noexcept                                  = default;
  xiiNetworkEvent& operator=(xiiNetworkEvent&&) noexcept                       = default;
  xiiNetworkEvent(const xiiNetworkEvent&)                                      = delete;
  xiiNetworkEvent&                           operator=(const xiiNetworkEvent&) = delete;
  xiiNetworkEventType                        m_Type                            = xiiNetworkEventType::Closed;
  xiiNetworkConnection                       m_Connection;
  xiiNetworkListener                         m_Listener;
  xiiNetworkChannel                          m_Channel;
  xiiNetworkMessage                          m_Message;
  xiiNetworkError                            m_Error;
  xiiNetworkEndpoint                         m_Endpoint;
  xiiNetworkIOBuffer                         m_Payload;
  xiiSharedPtr<xiiNetworkAuthenticationData> m_pAuthentication;
  xiiNetworkChannelConfiguration             m_ChannelConfiguration;
  xiiSharedPtr<xiiNetworkSessionTicket>      m_pSessionTicket;
  xiiUInt64                                  m_uiUserData = 0;
  xiiTime                                    m_ReconnectDelay;
};

/// Independent admission result for one broadcast recipient. Successful admission produces the
/// ordinary MessageSent event; failed admission leaves m_Message invalid and produces no event.
struct XII_FOUNDATION_DLL xiiNetworkBroadcastResult
{
  xiiNetworkMessage m_Message;
  xiiNetworkError   m_Error;
};

/// Independently owned host with one protocol driver and a native M2 I/O context. Start/Shutdown are
/// mutually serialized; submissions, queries and event polling may otherwise be concurrent. Destroy
/// only after callers stop accessing the facade. Shutdown rejects admission, quiesces native users,
/// and retains terminal events for dequeue. There are no per-connection threads or application callbacks.
class XII_FOUNDATION_DLL xiiNetworkHost final
{
public:
  xiiNetworkHost();
  ~xiiNetworkHost();
  xiiNetworkHost(const xiiNetworkHost&)                                     = delete;
  xiiNetworkHost& operator=(const xiiNetworkHost&)                          = delete;
  xiiNetworkHost(xiiNetworkHost&&)                                          = delete;
  xiiNetworkHost&                               operator=(xiiNetworkHost&&) = delete;
  [[nodiscard]] xiiNetworkError                 Start(const xiiNetworkHostConfiguration& configuration);
  void                                          Shutdown();
  [[nodiscard]] bool                            IsRunning() const;
  [[nodiscard]] xiiNetworkTransportCapabilities GetCapabilities() const;
  [[nodiscard]] xiiNetworkHostStatistics        GetStatistics() const;
  [[nodiscard]] xiiNetworkError                 GetPeerCertificate(xiiNetworkConnection connection, xiiNetworkCertificate& out_certificate) const;
  [[nodiscard]] xiiNetworkError                 Listen(const xiiNetworkEndpoint& endpoint, xiiNetworkListener& out_listener, xiiInt32 iBacklog = 128, bool bIpv6Only = true);
  [[nodiscard]] xiiNetworkError                 CloseListener(xiiNetworkListener listener);
  [[nodiscard]] xiiNetworkError                 GetLocalEndpoint(xiiNetworkListener listener, xiiNetworkEndpoint& out_endpoint) const;
  [[nodiscard]] xiiNetworkError                 Connect(const xiiNetworkEndpoint& endpoint, const xiiNetworkConnectOptions& options, xiiNetworkConnection& out_connection);
  [[nodiscard]] xiiNetworkError                 Connect(xiiStringView sHost, xiiStringView sService, const xiiNetworkConnectOptions& options, xiiNetworkConnection& out_connection);
  [[nodiscard]] xiiNetworkError                 GetConnectionInfo(xiiNetworkConnection connection, xiiNetworkConnectionInfo& out_info) const;
  [[nodiscard]] xiiNetworkError                 GetDefaultChannel(xiiNetworkConnection connection, xiiNetworkChannel& out_channel) const;
  [[nodiscard]] xiiNetworkError                 OpenChannel(xiiNetworkConnection connection, const xiiNetworkChannelConfiguration& configuration, xiiNetworkChannel& out_channel);
  [[nodiscard]] xiiNetworkError                 AuthorizeChannel(xiiNetworkChannel channel, bool bAccept);
  [[nodiscard]] xiiNetworkError                 CloseChannel(xiiNetworkChannel channel);
  [[nodiscard]] xiiNetworkError                 GetChannelStatistics(xiiNetworkChannel channel, xiiNetworkChannelStatistics& out_statistics) const;
  [[nodiscard]] xiiNetworkError                 Authorize(xiiNetworkConnection connection, const xiiNetworkAuthorization& authorization, bool bAccept);
  [[nodiscard]] xiiNetworkError                 Disconnect(xiiNetworkConnection connection, bool bGraceful = true);
  [[nodiscard]] xiiNetworkError                 CancelReconnect(xiiNetworkConnection connection);
  [[nodiscard]] xiiNetworkError                 RevokeSession(xiiUuid session);
  [[nodiscard]] xiiNetworkError                 ReplaceCredentials(xiiSharedPtr<xiiNetworkCredentials> pCredentials);
  [[nodiscard]] xiiNetworkError                 AcquireBuffer(xiiUInt32 uiBytes, xiiNetworkIOBuffer& out_buffer);
  /// Owned payload must originate from this host. Rejection preserves ownership; no implicit replay.
  [[nodiscard]] xiiNetworkError Send(xiiNetworkChannel channel, xiiNetworkIOBuffer&& payload, xiiNetworkMessage& out_message, xiiUInt64 uiUserData = 0);
  /// Explicit copy convenience; use AcquireBuffer/owning overload for retained zero-copy admission.
  [[nodiscard]] xiiNetworkError Send(xiiNetworkChannel channel, xiiArrayPtr<const xiiUInt8> payload, xiiNetworkMessage& out_message, xiiUInt64 uiUserData = 0);
  /// Copies payload into an independent lease for each channel, in input order. Admission is not
  /// atomic: inspect every result. Equal channel/result counts are required before any submission.
  /// Empty recipient lists succeed without work. Duplicate channels receive duplicate messages.
  [[nodiscard]] xiiNetworkError Broadcast(xiiArrayPtr<const xiiNetworkChannel> channels, xiiArrayPtr<const xiiUInt8> payload, xiiArrayPtr<xiiNetworkBroadcastResult> out_results, xiiUInt64 uiUserData = 0);
  /// NativeUdp only: raw datagrams have no sessions, authentication or logical channels.
  [[nodiscard]] xiiNetworkError SendDatagram(xiiNetworkListener socket, const xiiNetworkEndpoint& destination, xiiNetworkIOBuffer&& payload, xiiNetworkMessage& out_message, xiiUInt64 uiUserData = 0);
  [[nodiscard]] bool            TryGetEvent(xiiNetworkEvent& out_event);
  [[nodiscard]] xiiNetworkError WaitForEvent(xiiNetworkEvent& out_event, xiiTime timeout);

private:
  xiiNetworkHostRuntime* m_pRuntime = nullptr;
};
