/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>
#include <Foundation/Network/Implementation/NetworkIOContextAccess.h>
#include <Foundation/Threading/Lock.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

using namespace xiiNetworkHostInternal;

namespace
{
  std::atomic<xiiUInt64> s_uiNextHost{1};
  bool                   HostDuration(xiiTime value) { return std::isfinite(value.GetSeconds()) && value.GetSeconds() > 0; }
} // namespace

xiiNetworkHostRuntime::xiiNetworkHostRuntime() : m_uiHostId(s_uiNextHost.fetch_add(1)) {}
xiiNetworkHostRuntime::~xiiNetworkHostRuntime() { Shutdown(); }

xiiNetworkError xiiNetworkHostRuntime::Start(const xiiNetworkHostConfiguration& configuration)
{
  XII_LOCK(m_LifecycleMutex);
  if (m_bStarted)
  {
    return xiiNetworkError(xiiNetworkErrorCode::AlreadyOpen);
  }
  if (configuration.m_Transport == xiiNetworkTransportType::SecureDatagram)
  {
    return xiiNetworkError(xiiNetworkErrorCode::UnsupportedCapability);
  }
  const bool bSecure = configuration.m_Transport == xiiNetworkTransportType::TlsTcp || configuration.m_Transport == xiiNetworkTransportType::SecureDatagram;
  if (configuration.m_Transport > xiiNetworkTransportType::SecureDatagram || configuration.m_Authentication > xiiNetworkAuthenticationPolicy::AnonymousDevelopment ||
      (!bSecure && (!configuration.m_bAllowInsecureDevelopment || configuration.m_Authentication != xiiNetworkAuthenticationPolicy::AnonymousDevelopment)) ||
      (bSecure && (configuration.m_pCredentials == nullptr || !configuration.m_pCredentials->IsLoaded())) ||
      (configuration.m_Authentication == xiiNetworkAuthenticationPolicy::AnonymousDevelopment && !configuration.m_bAllowInsecureDevelopment) ||
      !configuration.m_Protocol.IsValid() || configuration.m_uiMinimumProtocolVersion == 0 || configuration.m_uiMaximumProtocolVersion < configuration.m_uiMinimumProtocolVersion ||
      (configuration.m_uiRequiredFeatures & ~configuration.m_uiSupportedFeatures) != 0 ||
      configuration.m_uiMaximumConnections == 0 || configuration.m_uiMaximumConnections > 65536 || configuration.m_uiMaximumListeners == 0 || configuration.m_uiMaximumListeners > 1024 ||
      configuration.m_uiMaximumChannelsPerConnection < 2 || configuration.m_uiMaximumChannelsPerConnection > 65534 ||
      configuration.m_uiMaximumMessageBytes == 0 || configuration.m_uiMaximumMessageBytes > 64 * 1024 * 1024 || configuration.m_uiMaximumQueuedMessages == 0 || configuration.m_uiMaximumQueuedMessages > 1024 * 1024 ||
      configuration.m_uiMaximumQueuedEvents < static_cast<xiiUInt64>(configuration.m_uiMaximumConnections) * 3 ||
      configuration.m_uiMaximumPeerSendBytes < configuration.m_uiMaximumMessageBytes || configuration.m_uiMaximumPeerReceiveBytes < configuration.m_uiMaximumMessageBytes ||
      configuration.m_uiMaximumReceiveBytes < configuration.m_uiMaximumPeerReceiveBytes ||
      configuration.m_uiSendBurstBytes < configuration.m_uiMaximumMessageBytes || configuration.m_uiPeerSendBurstBytes < configuration.m_uiMaximumMessageBytes ||
      !HostDuration(configuration.m_HandshakeTimeout) || !HostDuration(configuration.m_AuthenticationTimeout) || !HostDuration(configuration.m_IdleTimeout) ||
      !HostDuration(configuration.m_HeartbeatInterval) || !HostDuration(configuration.m_CloseTimeout) ||
      (configuration.m_bEnableSessionRecovery && (!bSecure || configuration.m_uiMaximumSessions < configuration.m_uiMaximumConnections || configuration.m_uiMaximumSessions > 1024 * 1024 || !HostDuration(configuration.m_SessionRecoveryWindow) || !HostDuration(configuration.m_MaximumSessionLifetime) || configuration.m_SessionRecoveryWindow > configuration.m_MaximumSessionLifetime || configuration.m_MaximumSessionLifetime > xiiTime::MakeFromHours(24))))
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  m_Configuration = configuration;
  m_pCredentials  = configuration.m_pCredentials;
  m_Configuration.m_pCredentials.Clear();
  m_Capabilities.m_bEncrypted                 = bSecure;
  m_Capabilities.m_bAuthenticated             = bSecure;
  m_Capabilities.m_bConnectionless            = configuration.m_Transport == xiiNetworkTransportType::NativeUdp;
  m_Capabilities.m_uiDeliveryModes            = m_Capabilities.m_bConnectionless ? xiiNetworkDeliveryMask(xiiNetworkDeliveryMode::UnreliableUnsequenced) : xiiNetworkDeliveryMask(xiiNetworkDeliveryMode::ReliableOrdered);
  m_Capabilities.m_uiMaximumUnreliablePayload = m_Capabilities.m_bConnectionless ? 65507U : 0U;
  m_Peers.SetCount(configuration.m_uiMaximumConnections);
  m_Listeners.SetCount(configuration.m_uiMaximumListeners);
  m_Sends.SetCount(configuration.m_uiMaximumQueuedMessages);
  if (configuration.m_bEnableSessionRecovery)
  {
    m_Sessions.SetCount(configuration.m_uiMaximumSessions);
  }
  auto error = xiiNetworkIOContextAccess::SetCompletionSignal(m_IO, m_WorkSignal);
  if (error.Failed())
  {
    return error;
  }
  error = m_IO.Start(configuration.m_IO);
  if (error.Failed())
  {
    return error;
  }
  m_bStarted = true;
  m_SendTokens.Reset(configuration.m_uiSendBurstBytes);
  m_bRunning.store(true);
  error = xiiNetworkIOInternal::StartThread(m_Thread, [](void* pData) { static_cast<xiiNetworkHostRuntime*>(pData)->Run(); }, this);
  if (error.Failed())
  {
    m_bRunning.store(false);
    m_IO.Shutdown();
    return error;
  }
  return {};
}

void xiiNetworkHostRuntime::Shutdown()
{
  XII_LOCK(m_LifecycleMutex);
  if (!m_bStarted || !m_bRunning.load())
  {
    return;
  }
  {
    XII_LOCK(m_AdmissionMutex);
    m_bStopping.store(true);
  }
  m_WorkSignal.RaiseSignal();
  xiiNetworkIOInternal::JoinThread(m_Thread);
  m_IO.Shutdown();
  xiiNetworkIOResult result;
  while (m_IO.TryGetCompletion(result))
  {
    CompleteIO(std::move(result));
  }
  m_bRunning.store(false);
  m_EventsMutex.SignalAll();
}

bool xiiNetworkHostRuntime::ReserveEvent()
{
  xiiUInt32 uiCount = m_uiEventReservations.load();
  while (uiCount < m_Configuration.m_uiMaximumQueuedEvents)
  {
    if (m_uiEventReservations.compare_exchange_weak(uiCount, uiCount + 1))
    {
      return true;
    }
  }
  return false;
}
void xiiNetworkHostRuntime::ReleaseEvents(xiiUInt32 uiCount)
{
  const auto uiOld = m_uiEventReservations.fetch_sub(uiCount);
  XII_ASSERT_DEV(uiOld >= uiCount, "Event credits cannot underflow.");
}
void xiiNetworkHostRuntime::Publish(xiiNetworkEvent&& event)
{
  XII_LOCK(m_EventsMutex);
  XII_ASSERT_DEV(m_Events.GetCount() < m_uiEventReservations.load(), "Every event reserves capacity before publication.");
  m_Events.PushBack(std::move(event));
  m_EventsMutex.SignalOne();
}

bool xiiNetworkHostRuntime::PeerEvent(Peer& peer, xiiNetworkEventType type, xiiNetworkError error)
{
  if (peer.m_uiReservedEvents > (type == xiiNetworkEventType::Closed ? 0U : 1U))
  {
    --peer.m_uiReservedEvents;
  }
  else if (!ReserveEvent())
  {
    return false;
  }
  xiiNetworkEvent event;
  event.m_Type           = type;
  event.m_Connection     = peer.m_Handle;
  event.m_Error          = error;
  event.m_pSessionTicket = peer.m_pTicket;
  event.m_Endpoint       = peer.m_Info.m_RemoteEndpoint;
  if (type == xiiNetworkEventType::ReconnectScheduled)
  {
    event.m_ReconnectDelay = xiiMath::Max(peer.m_ReconnectAt - xiiTime::Now(), xiiTime::MakeZero());
  }
  Publish(std::move(event));
  return true;
}
bool xiiNetworkHostRuntime::ChannelEvent(Peer& peer, Channel& channel, xiiNetworkEventType type, xiiNetworkError error)
{
  if (channel.m_uiReservedEvents != 0)
  {
    --channel.m_uiReservedEvents;
  }
  else if (!ReserveEvent())
  {
    return false;
  }
  xiiNetworkEvent event;
  event.m_Type                 = type;
  event.m_Connection           = peer.m_Handle;
  event.m_Channel              = {peer.m_Handle, channel.m_uiId, peer.m_uiAttempt};
  event.m_ChannelConfiguration = channel.m_Configuration;
  event.m_Error                = error;
  Publish(std::move(event));
  return true;
}

bool xiiNetworkHostRuntime::Poll(xiiNetworkEvent& out_event)
{
  xiiNetworkEvent event;
  {
    XII_LOCK(m_EventsMutex);
    if (m_Events.IsEmpty())
    {
      return false;
    }
    event = std::move(m_Events.PeekFront());
    m_Events.PopFront();
  }
  ReleaseEvents(1);
  if (event.m_Type == xiiNetworkEventType::Message)
  {
    const auto uiBytes = event.m_Payload.GetCount();
    m_Peers[event.m_Connection.m_uiSlot].m_uiReceiveBytes.fetch_sub(uiBytes);
    m_uiReceiveBytes.fetch_sub(uiBytes);
    m_WorkSignal.RaiseSignal();
  }
  if (event.m_Type == xiiNetworkEventType::Datagram)
  {
    m_uiReceiveBytes.fetch_sub(event.m_Payload.GetCount());
    m_WorkSignal.RaiseSignal();
  }
  if (event.m_Type == xiiNetworkEventType::ListenerClosed)
  {
    auto& listener = m_Listeners[event.m_Listener.m_uiSlot];
    XII_LOCK(listener.m_Mutex);
    listener.m_bTerminalConsumed = true;
    m_WorkSignal.RaiseSignal();
  }
  if (event.m_Type == xiiNetworkEventType::Closed)
  {
    auto& peer = m_Peers[event.m_Connection.m_uiSlot];
    XII_LOCK(peer.m_Mutex);
    if (peer.m_Handle == event.m_Connection)
    {
      peer.m_bTerminalConsumed = true;
    }
    m_WorkSignal.RaiseSignal();
  }
  if (event.m_Type == xiiNetworkEventType::MessageSent)
  {
    XII_LOCK(m_SendPoolMutex);
    auto& send = m_Sends[event.m_Message.m_uiSlot];
    XII_ASSERT_DEV(send.m_Handle == event.m_Message && send.m_bCompleted, "Send slots retire after their terminal event is consumed.");
    send.m_bInUse = false;
  }
  out_event = std::move(event);
  return true;
}

xiiNetworkError xiiNetworkHostRuntime::AllocatePeer(bool bIncoming, Peer*& out_pPeer)
{
  for (xiiUInt32 uiSlot = 0; uiSlot < m_Peers.GetCount(); ++uiSlot)
  {
    auto& peer = m_Peers[uiSlot];
    XII_LOCK(peer.m_Mutex);
    if (peer.m_bInUse && (!peer.m_bTerminalConsumed || peer.m_uiPendingIO != 0 || peer.m_uiReceiveBytes.load() != 0))
    {
      continue;
    }
    if (peer.m_Handle.m_uiGeneration == std::numeric_limits<xiiUInt64>::max())
    {
      continue;
    }
    xiiUInt32 uiReserved = 0;
    while (uiReserved < 3 && ReserveEvent())
    {
      ++uiReserved;
    }
    if (uiReserved != 3)
    {
      ReleaseEvents(uiReserved);
      return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
    }
    peer.m_Handle                       = {m_uiHostId, peer.m_Handle.m_uiGeneration + 1, uiSlot};
    peer.m_Info                         = {};
    peer.m_Info.m_bIncoming             = bIncoming;
    peer.m_Info.m_Capabilities          = m_Capabilities;
    peer.m_Info.m_uiMaximumMessageBytes = m_Configuration.m_uiMaximumMessageBytes;
    peer.m_Info.m_State                 = xiiNetworkConnectionState::Connecting;
    peer.m_Options                      = {};
    peer.m_pTicket.Clear();
    peer.m_Certificate   = {};
    peer.m_Authorization = {};
    peer.m_sHost.Clear();
    peer.m_sService.Clear();
    peer.m_Endpoints.Clear();
    peer.m_uiEndpointIndex   = 0;
    peer.m_Registration      = {};
    peer.m_ReceiveOperation  = {};
    peer.m_uiAttempt         = 0;
    peer.m_uiPendingIO       = 0;
    peer.m_uiReservedEvents  = 3;
    peer.m_uiSessionSlot     = InvalidSlot;
    peer.m_uiMaximumChannels = m_Configuration.m_uiMaximumChannelsPerConnection;
    peer.m_uiDeliveryModes   = m_Capabilities.m_uiDeliveryModes;
    peer.m_uiScheduleChannel = 1;
    peer.m_uiScheduleWeight  = 0;
    peer.m_uiCipherProduced = peer.m_uiCipherCompleted = 0;
    peer.m_uiDecoderReservedBytes                      = 0;
    peer.m_uiPingSequence                              = 0;
    peer.m_PingSent                                    = xiiTime::MakeZero();
    peer.m_LastActivity                                = xiiTime::Now();
    peer.m_Deadline                                    = peer.m_LastActivity + m_Configuration.m_HandshakeTimeout;
    peer.m_ReconnectAt                                 = xiiTime::MakeZero();
    peer.m_SendTokens.Reset(m_Configuration.m_uiPeerSendBurstBytes);
    peer.m_pTls.Clear();
    peer.m_Decoder.Reset();
    peer.m_Decoder.SetMaximumPayloadBytes(8192).IgnoreResult();
    peer.m_CipherInput.Clear();
    peer.m_uiCipherOffset = 0;
    peer.m_Channels.Clear();
    peer.m_Channels.SetCount(peer.m_uiMaximumChannels + 1);
    peer.m_ControlQueue.Clear();
    peer.m_uiActiveSend       = InvalidSlot;
    peer.m_bActiveControl     = false;
    peer.m_bInUse             = false;
    peer.m_bStarted           = false;
    peer.m_bIncoming          = bIncoming;
    peer.m_bCipherSendPending = false;
    peer.m_bClosing           = false;
    peer.m_bCloseRequested    = false;
    peer.m_bGraceful          = true;
    peer.m_bDeregistered = peer.m_bPeerEof = peer.m_bTlsClosed = peer.m_bTransportLost = false;
    peer.m_RequestedSession                                                            = {};
    peer.m_bAllowFreshSession                                                          = false;
    peer.m_bDisconnectSent                                                             = false;
    peer.m_bShutdownSent                                                               = false;
    peer.m_bTerminalPublished = peer.m_bTerminalConsumed = false;
    peer.m_bAuthorizationReady = peer.m_bAcceptAuthorization = false;
    peer.m_bHelloReceived = peer.m_bWelcomeReceived = peer.m_bReadyReceived = peer.m_bActivated = false;
    peer.m_bDecoderEventReserved                                                                = false;
    peer.m_CloseError                                                                           = {};
    {
      XII_LOCK(m_CredentialsMutex);
      peer.m_pCredentials = m_pCredentials;
    }
    out_pPeer = &peer;
    return {};
  }
  return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
}

xiiNetworkError xiiNetworkHostRuntime::AllocateSend(xiiNetworkChannel channel, xiiNetworkIOBuffer&& payload, xiiUInt64 uiUserData, xiiNetworkMessage& out_message)
{
  XII_LOCK(m_SendPoolMutex);
  for (xiiUInt32 uiSlot = 0; uiSlot < m_Sends.GetCount(); ++uiSlot)
  {
    auto& send = m_Sends[uiSlot];
    if (send.m_bInUse || send.m_Handle.m_uiGeneration == std::numeric_limits<xiiUInt64>::max())
    {
      continue;
    }
    if (!ReserveEvent())
    {
      return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
    }
    send.m_Handle                             = {m_uiHostId, send.m_Handle.m_uiGeneration + 1, uiSlot};
    send.m_Channel                            = channel;
    send.m_Listener                           = {};
    send.m_Destination                        = {};
    send.m_Outbound                           = {};
    send.m_Outbound.m_Header.m_uiChannel      = channel.m_uiId;
    send.m_Outbound.m_Header.m_uiPayloadBytes = payload.GetCount();
    send.m_Outbound.m_Payload                 = std::move(payload);
    send.m_Outbound.m_uiMessageSlot           = uiSlot;
    send.m_uiUserData                         = uiUserData;
    send.m_uiPayloadBytes                     = send.m_Outbound.m_Header.m_uiPayloadBytes;
    send.m_bInUse                             = true;
    send.m_bCompleted                         = false;
    out_message                               = send.m_Handle;
    return {};
  }
  return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
}

void TokenBucket::Reset(xiiUInt64 uiBurst)
{
  m_fTokens = static_cast<double>(uiBurst);
  m_Updated = xiiTime::Now();
}
bool TokenBucket::Take(xiiUInt32 uiBytes, xiiUInt64 uiRate, xiiUInt64 uiBurst, xiiTime now, xiiTime& ref_nextWake)
{
  if (uiRate == 0)
  {
    return true;
  }
  m_fTokens = xiiMath::Min(static_cast<double>(uiBurst), m_fTokens + xiiMath::Max(0.0, (now - m_Updated).GetSeconds()) * static_cast<double>(uiRate));
  m_Updated = now;
  if (m_fTokens >= uiBytes)
  {
    m_fTokens -= uiBytes;
    return true;
  }
  ref_nextWake = xiiMath::Min(ref_nextWake, now + xiiTime::MakeFromSeconds((uiBytes - m_fTokens) / static_cast<double>(uiRate)));
  return false;
}
