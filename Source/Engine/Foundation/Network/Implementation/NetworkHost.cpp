/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>
#include <Foundation/Network/Implementation/NetworkIOContextAccess.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Threading/Lock.h>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

using namespace xiiNetworkHostInternal;

namespace
{
  xiiNetworkError HostError(xiiNetworkErrorCode code) { return xiiNetworkError(code); }
  bool            ValidDuration(xiiTime value) { return std::isfinite(value.GetSeconds()) && value.GetSeconds() > 0; }
  bool            ValidChannel(const xiiNetworkChannelConfiguration& configuration)
  {
    return configuration.m_uiWeight > 0 && configuration.m_uiWeight <= 16 && configuration.m_uiMaximumQueuedMessages > 0 && configuration.m_uiMaximumQueuedBytes > 0;
  }
} // namespace

xiiNetworkHost::xiiNetworkHost() : m_pRuntime(XII_DEFAULT_NEW(xiiNetworkHostRuntime)) {}
xiiNetworkHost::~xiiNetworkHost() { XII_DEFAULT_DELETE(m_pRuntime); }
xiiNetworkError                 xiiNetworkHost::Start(const xiiNetworkHostConfiguration& configuration) { return m_pRuntime->Start(configuration); }
void                            xiiNetworkHost::Shutdown() { m_pRuntime->Shutdown(); }
bool                            xiiNetworkHost::IsRunning() const { return m_pRuntime->m_bRunning.load() && !m_pRuntime->m_bStopping.load(); }
xiiNetworkTransportCapabilities xiiNetworkHost::GetCapabilities() const { return m_pRuntime->m_Capabilities; }
xiiNetworkError                 xiiNetworkHost::AcquireBuffer(xiiUInt32 uiBytes, xiiNetworkIOBuffer& out_buffer)
{
  return IsRunning() ? m_pRuntime->m_IO.AcquireBuffer(uiBytes, out_buffer) : HostError(xiiNetworkErrorCode::NotRunning);
}

xiiNetworkHostRuntime::Peer* xiiNetworkHostRuntime::Find(xiiNetworkConnection handle)
{
  if (handle.m_uiHost != m_uiHostId || handle.m_uiSlot >= m_Peers.GetCount())
  {
    return nullptr;
  }
  return &m_Peers[handle.m_uiSlot];
}
xiiNetworkHostRuntime::Listener* xiiNetworkHostRuntime::Find(xiiNetworkListener handle)
{
  if (handle.m_uiHost != m_uiHostId || handle.m_uiSlot >= m_Listeners.GetCount())
  {
    return nullptr;
  }
  return &m_Listeners[handle.m_uiSlot];
}
xiiNetworkHostRuntime::Channel* xiiNetworkHostRuntime::Find(Peer& peer, xiiNetworkChannel handle)
{
  const xiiUInt32 uiSlot = handle.m_uiId & 0xFFFFU;
  if (peer.m_Handle != handle.m_Connection || peer.m_uiAttempt != handle.m_uiSessionGeneration || !peer.m_bInUse || uiSlot == 0 || uiSlot >= peer.m_Channels.GetCount())
  {
    return nullptr;
  }
  auto& channel = peer.m_Channels[uiSlot];
  return channel.m_uiId == handle.m_uiId && channel.m_State != ChannelState::Free ? &channel : nullptr;
}
bool xiiNetworkHostRuntime::OwnsBuffer(const xiiNetworkIOBuffer& buffer) const { return xiiNetworkIOContextAccess::OwnsBuffer(m_IO, buffer); }

xiiNetworkError xiiNetworkHost::Listen(const xiiNetworkEndpoint& endpoint, xiiNetworkListener& out_listener, xiiInt32 iBacklog, bool bIpv6Only)
{
  auto& runtime = *m_pRuntime;
  XII_LOCK(runtime.m_AdmissionMutex);
  if (!IsRunning())
  {
    return HostError(xiiNetworkErrorCode::NotRunning);
  }
  if (runtime.m_Capabilities.m_bEncrypted)
  {
    XII_LOCK(runtime.m_CredentialsMutex);
    if (runtime.m_pCredentials == nullptr || !runtime.m_pCredentials->HasIdentity() ||
        (runtime.m_Configuration.m_Authentication == xiiNetworkAuthenticationPolicy::Certificate && !runtime.m_pCredentials->GetConfiguration().m_bRequireClientCertificate))
    {
      return HostError(xiiNetworkErrorCode::InvalidArgument);
    }
  }
  for (xiiUInt32 uiSlot = 0; uiSlot < runtime.m_Listeners.GetCount(); ++uiSlot)
  {
    auto& entry = runtime.m_Listeners[uiSlot];
    XII_LOCK(entry.m_Mutex);
    if (entry.m_bInUse || entry.m_Handle.m_uiGeneration == std::numeric_limits<xiiUInt64>::max())
    {
      continue;
    }
    xiiNetworkError error;
    if (runtime.m_Configuration.m_Transport == xiiNetworkTransportType::NativeUdp)
    {
      xiiUdpSocket socket;
      error = socket.Open(endpoint.GetAddress().GetFamily());
      if (error.Succeeded() && endpoint.GetAddress().GetFamily() == xiiNetworkAddressFamily::Ipv6)
      {
        error = socket.SetIpv6Only(bIpv6Only);
      }
      if (error.Succeeded())
      {
        error = socket.Bind(endpoint);
      }
      if (error.Succeeded())
      {
        error = runtime.m_IO.Register(std::move(socket), entry.m_Registration);
      }
    }
    else
    {
      xiiTcpListener socket;
      error = socket.Open(endpoint.GetAddress().GetFamily());
      if (error.Succeeded() && endpoint.GetAddress().GetFamily() == xiiNetworkAddressFamily::Ipv6)
      {
        error = socket.SetIpv6Only(bIpv6Only);
      }
      if (error.Succeeded())
      {
        error = socket.Bind(endpoint);
      }
      if (error.Succeeded())
      {
        error = socket.Listen(iBacklog);
      }
      if (error.Succeeded())
      {
        error = runtime.m_IO.Register(std::move(socket), entry.m_Registration);
      }
    }
    if (error.Failed())
    {
      return error;
    }
    error = runtime.m_IO.GetLocalEndpoint(entry.m_Registration, entry.m_Endpoint);
    if (error.Failed())
    {
      runtime.m_IO.Deregister(entry.m_Registration).IgnoreResult();
      return error;
    }
    if (!runtime.ReserveEvent())
    {
      runtime.m_IO.Deregister(entry.m_Registration).IgnoreResult();
      return HostError(xiiNetworkErrorCode::Backpressure);
    }
    entry.m_bDeregistered = entry.m_bTerminalPublished = entry.m_bTerminalConsumed = false;
    entry.m_Error                                                                  = {};
    entry.m_uiQueuedBytes                                                          = 0;
    entry.m_Handle                                                                 = {runtime.m_uiHostId, entry.m_Handle.m_uiGeneration + 1, uiSlot};
    entry.m_bInUse                                                                 = true;
    entry.m_bClosing                                                               = false;
    entry.m_uiPendingIO                                                            = 0;
    entry.m_bReceivePending                                                        = false;
    out_listener                                                                   = entry.m_Handle;
    runtime.m_WorkSignal.RaiseSignal();
    return {};
  }
  return HostError(xiiNetworkErrorCode::Backpressure);
}

xiiNetworkError xiiNetworkHost::CloseListener(xiiNetworkListener handle)
{
  auto* pListener = m_pRuntime->Find(handle);
  if (pListener == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pListener->m_Mutex);
  if (!pListener->m_bInUse || pListener->m_Handle != handle)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  pListener->m_bClosing = true;
  m_pRuntime->m_WorkSignal.RaiseSignal();
  return {};
}

xiiNetworkError xiiNetworkHost::GetLocalEndpoint(xiiNetworkListener handle, xiiNetworkEndpoint& out_endpoint) const
{
  auto* pListener = m_pRuntime->Find(handle);
  if (pListener == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pListener->m_Mutex);
  if (!pListener->m_bInUse || pListener->m_Handle != handle)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  out_endpoint = pListener->m_Endpoint;
  return {};
}

namespace
{
  xiiNetworkError AdmitConnection(xiiNetworkHostRuntime& runtime, const xiiNetworkEndpoint* pEndpoint, xiiStringView sHost, xiiStringView sService,
                                  const xiiNetworkConnectOptions& options, xiiNetworkConnection& out_connection)
  {
    XII_LOCK(runtime.m_AdmissionMutex);
    if (!runtime.m_bRunning.load() || runtime.m_bStopping.load())
    {
      return HostError(xiiNetworkErrorCode::NotRunning);
    }
    if (runtime.m_Capabilities.m_bConnectionless)
    {
      return HostError(xiiNetworkErrorCode::UnsupportedOperation);
    }
    const auto& retry = options.m_Reconnect;
    if ((runtime.m_Capabilities.m_bEncrypted && options.m_sServerName.IsEmpty()) ||
        retry.m_Mode > xiiNetworkReconnectMode::Exponential || !ValidDuration(retry.m_InitialDelay) || !ValidDuration(retry.m_MaximumDelay) ||
        retry.m_MaximumDelay < retry.m_InitialDelay || !std::isfinite(retry.m_fJitterFraction) || retry.m_fJitterFraction < 0 || retry.m_fJitterFraction > 1 ||
        (retry.m_Mode != xiiNetworkReconnectMode::Disabled && retry.m_uiMaximumAttempts == 0 && !retry.m_bAllowUnboundedRetries) ||
        (pEndpoint != nullptr && !pEndpoint->GetAddress().IsValid()) || (pEndpoint == nullptr && (sHost.IsEmpty() || sService.IsEmpty())))
    {
      return HostError(xiiNetworkErrorCode::InvalidArgument);
    }
    xiiNetworkHostRuntime::Peer* pPeer = nullptr;
    auto                         error = runtime.AllocatePeer(false, pPeer);
    if (error.Failed())
    {
      return error;
    }
    XII_LOCK(pPeer->m_Mutex);
    pPeer->m_Options  = options;
    pPeer->m_sHost    = sHost;
    pPeer->m_sService = sService;
    if (pEndpoint != nullptr)
    {
      pPeer->m_Endpoints.PushBack(*pEndpoint);
    }
    pPeer->m_bInUse = true;
    out_connection  = pPeer->m_Handle;
    runtime.m_WorkSignal.RaiseSignal();
    return {};
  }
} // namespace
xiiNetworkError xiiNetworkHost::Connect(const xiiNetworkEndpoint& endpoint, const xiiNetworkConnectOptions& options, xiiNetworkConnection& out_connection)
{
  return AdmitConnection(*m_pRuntime, &endpoint, {}, {}, options, out_connection);
}
xiiNetworkError xiiNetworkHost::Connect(xiiStringView sHost, xiiStringView sService, const xiiNetworkConnectOptions& options, xiiNetworkConnection& out_connection)
{
  return AdmitConnection(*m_pRuntime, nullptr, sHost, sService, options, out_connection);
}

xiiNetworkError xiiNetworkHost::GetConnectionInfo(xiiNetworkConnection handle, xiiNetworkConnectionInfo& out_info) const
{
  auto* pPeer = m_pRuntime->Find(handle);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  if (!pPeer->m_bInUse || pPeer->m_Handle != handle)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  out_info = pPeer->m_Info;
  return {};
}
xiiNetworkError xiiNetworkHost::GetDefaultChannel(xiiNetworkConnection handle, xiiNetworkChannel& out_channel) const
{
  auto* pPeer = m_pRuntime->Find(handle);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  if (!pPeer->m_bInUse || pPeer->m_Handle != handle)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (pPeer->m_Info.m_State != xiiNetworkConnectionState::Connected || pPeer->m_Channels[1].m_State != ChannelState::Open)
  {
    return HostError(xiiNetworkErrorCode::NotConnected);
  }
  out_channel = {handle, DefaultChannelId, pPeer->m_uiAttempt};
  return {};
}

xiiNetworkError xiiNetworkHost::OpenChannel(xiiNetworkConnection handle, const xiiNetworkChannelConfiguration& configuration, xiiNetworkChannel& out_channel)
{
  auto& runtime = *m_pRuntime;
  auto* pPeer   = runtime.Find(handle);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  if (!pPeer->m_bInUse || pPeer->m_Handle != handle)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (pPeer->m_Info.m_State != xiiNetworkConnectionState::Connected || pPeer->m_bCloseRequested || !IsRunning())
  {
    return HostError(xiiNetworkErrorCode::NotConnected);
  }
  if (!pPeer->m_bIncoming && !pPeer->m_Authorization.m_bMayOpenChannels)
  {
    return HostError(xiiNetworkErrorCode::PermissionDenied);
  }
  if (!ValidChannel(configuration))
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  if ((pPeer->m_uiDeliveryModes & xiiNetworkDeliveryMask(configuration.m_Delivery)) == 0)
  {
    return HostError(xiiNetworkErrorCode::UnsupportedCapability);
  }
  for (xiiUInt32 uiSlot = pPeer->m_bIncoming ? 2U : 3U; uiSlot <= pPeer->m_uiMaximumChannels; uiSlot += 2)
  {
    auto& channel = pPeer->m_Channels[uiSlot];
    if ((channel.m_State != ChannelState::Free && channel.m_State != ChannelState::Closed) || channel.m_uiGeneration == 0xFFFF || channel.m_uiReservedEvents != 0 || !channel.m_SendQueue.IsEmpty())
    {
      continue;
    }
    if (!runtime.ReserveEvent())
    {
      return HostError(xiiNetworkErrorCode::Backpressure);
    }
    if (!runtime.ReserveEvent())
    {
      runtime.ReleaseEvents(1);
      return HostError(xiiNetworkErrorCode::Backpressure);
    }
    channel.m_uiReservedEvents      = 2;
    channel.m_uiId                  = (++channel.m_uiGeneration << 16) | uiSlot;
    channel.m_Configuration         = configuration;
    channel.m_Statistics            = {};
    channel.m_uiNextSendSequence    = 1;
    channel.m_uiLastReceiveSequence = 0;
    channel.m_State                 = ChannelState::Opening;
    channel.m_bLocal                = true;
    channel.m_bCloseReceived = channel.m_bCloseSent = channel.m_bCloseAcknowledged = false;
    channel.m_bNeedsControl                                                        = true;
    out_channel                                                                    = {handle, channel.m_uiId, pPeer->m_uiAttempt};
    runtime.m_WorkSignal.RaiseSignal();
    return {};
  }
  return HostError(xiiNetworkErrorCode::Backpressure);
}

xiiNetworkError xiiNetworkHost::AuthorizeChannel(xiiNetworkChannel handle, bool bAccept)
{
  auto& runtime = *m_pRuntime;
  auto* pPeer   = runtime.Find(handle.m_Connection);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  auto* pChannel = runtime.Find(*pPeer, handle);
  if (pChannel == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (pChannel->m_State != ChannelState::Authorizing)
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  pChannel->m_bAccept       = bAccept;
  pChannel->m_bNeedsControl = true;
  runtime.m_WorkSignal.RaiseSignal();
  return {};
}
xiiNetworkError xiiNetworkHost::CloseChannel(xiiNetworkChannel handle)
{
  auto& runtime = *m_pRuntime;
  auto* pPeer   = runtime.Find(handle.m_Connection);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  auto* pChannel = runtime.Find(*pPeer, handle);
  if (pChannel == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (pChannel->m_State == ChannelState::Closed || pChannel->m_State == ChannelState::Closing)
  {
    return HostError(xiiNetworkErrorCode::ChannelClosed);
  }
  if (pChannel->m_uiReservedEvents == 0)
  {
    if (!runtime.ReserveEvent())
    {
      return HostError(xiiNetworkErrorCode::Backpressure);
    }
    ++pChannel->m_uiReservedEvents;
  }
  pChannel->m_State         = ChannelState::Closing;
  pChannel->m_bNeedsControl = true;
  runtime.m_WorkSignal.RaiseSignal();
  return {};
}
xiiNetworkError xiiNetworkHost::GetChannelStatistics(xiiNetworkChannel handle, xiiNetworkChannelStatistics& out_statistics) const
{
  auto& runtime = *m_pRuntime;
  auto* pPeer   = runtime.Find(handle.m_Connection);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  auto* pChannel = runtime.Find(*pPeer, handle);
  if (pChannel == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  out_statistics = pChannel->m_Statistics;
  return {};
}

xiiNetworkError xiiNetworkHost::Authorize(xiiNetworkConnection handle, const xiiNetworkAuthorization& authorization, bool bAccept)
{
  auto* pPeer = m_pRuntime->Find(handle);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  if (!pPeer->m_bInUse || pPeer->m_Handle != handle)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (pPeer->m_Info.m_State != xiiNetworkConnectionState::Authenticating || !pPeer->m_bIncoming || pPeer->m_bAuthorizationReady)
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (bAccept && (authorization.m_sIdentity.IsEmpty() || authorization.m_sIdentity.GetElementCount() > 256))
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  pPeer->m_Authorization        = authorization;
  pPeer->m_bAcceptAuthorization = bAccept;
  pPeer->m_bAuthorizationReady  = true;
  m_pRuntime->m_WorkSignal.RaiseSignal();
  return {};
}
xiiNetworkError xiiNetworkHost::Disconnect(xiiNetworkConnection handle, bool bGraceful)
{
  auto* pPeer = m_pRuntime->Find(handle);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  if (!pPeer->m_bInUse || pPeer->m_Handle != handle || pPeer->m_bTerminalPublished)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  pPeer->m_bCloseRequested = true;
  pPeer->m_bGraceful       = bGraceful;
  m_pRuntime->m_WorkSignal.RaiseSignal();
  return {};
}
xiiNetworkError xiiNetworkHost::CancelReconnect(xiiNetworkConnection handle) { return Disconnect(handle, false); }

xiiNetworkError xiiNetworkHost::ReplaceCredentials(xiiSharedPtr<xiiNetworkCredentials> pCredentials)
{
  if (pCredentials == nullptr || !pCredentials->IsLoaded())
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  XII_LOCK(m_pRuntime->m_CredentialsMutex);
  m_pRuntime->m_pCredentials = pCredentials;
  return {};
}

xiiNetworkError xiiNetworkHost::Send(xiiNetworkChannel channel, xiiNetworkIOBuffer&& payload, xiiNetworkMessage& out_message, xiiUInt64 uiUserData)
{
  auto& runtime = *m_pRuntime;
  auto* pPeer   = runtime.Find(channel.m_Connection);
  if (pPeer == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pPeer->m_Mutex);
  auto* pChannel = runtime.Find(*pPeer, channel);
  if (pChannel == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (!IsRunning() || pPeer->m_Info.m_State != xiiNetworkConnectionState::Connected || pPeer->m_bCloseRequested)
  {
    return HostError(xiiNetworkErrorCode::NotConnected);
  }
  if (pChannel->m_State != ChannelState::Open)
  {
    return HostError(xiiNetworkErrorCode::ChannelClosed);
  }
  if (!pPeer->m_bIncoming && !pPeer->m_Authorization.m_bMaySend)
  {
    return HostError(xiiNetworkErrorCode::PermissionDenied);
  }
  const xiiUInt32 uiBytes = payload.GetCount();
  if (!runtime.OwnsBuffer(payload))
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (uiBytes > pPeer->m_Info.m_uiMaximumMessageBytes)
  {
    return HostError(xiiNetworkErrorCode::MessageTooLarge);
  }
  if (pChannel->m_Statistics.m_uiQueuedMessages >= pChannel->m_Configuration.m_uiMaximumQueuedMessages ||
      uiBytes > pChannel->m_Configuration.m_uiMaximumQueuedBytes - pChannel->m_Statistics.m_uiQueuedBytes ||
      uiBytes > runtime.m_Configuration.m_uiMaximumPeerSendBytes - pPeer->m_Info.m_Statistics.m_uiQueuedSendBytes)
  {
    ++pPeer->m_Info.m_Statistics.m_uiBackpressureEvents;
    return HostError(xiiNetworkErrorCode::Backpressure);
  }
  const auto error = runtime.AllocateSend(channel, std::move(payload), uiUserData, out_message);
  if (error.Succeeded())
  {
    auto& send                            = runtime.m_Sends[out_message.m_uiSlot];
    send.m_Outbound.m_Header.m_uiSequence = pChannel->m_uiNextSendSequence++;
    pChannel->m_SendQueue.PushBack(out_message.m_uiSlot);
    pChannel->m_Statistics.m_uiQueuedBytes += uiBytes;
    ++pChannel->m_Statistics.m_uiQueuedMessages;
    pPeer->m_Info.m_Statistics.m_uiQueuedSendBytes += uiBytes;
    runtime.m_WorkSignal.RaiseSignal();
  }
  return error;
}
xiiNetworkError xiiNetworkHost::Send(xiiNetworkChannel channel, xiiArrayPtr<const xiiUInt8> payload, xiiNetworkMessage& out_message, xiiUInt64 uiUserData)
{
  if (!payload.IsEmpty() && payload.GetPtr() == nullptr)
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  xiiNetworkIOBuffer buffer;
  auto               error = AcquireBuffer(payload.GetCount(), buffer);
  if (error.Failed())
  {
    return error;
  }
  if (!payload.IsEmpty())
  {
    std::memcpy(buffer.GetBytes().GetPtr(), payload.GetPtr(), payload.GetCount());
  }
  return Send(channel, std::move(buffer), out_message, uiUserData);
}

bool            xiiNetworkHost::TryGetEvent(xiiNetworkEvent& out_event) { return m_pRuntime->Poll(out_event); }
xiiNetworkError xiiNetworkHost::WaitForEvent(xiiNetworkEvent& out_event, xiiTime timeout)
{
  if (!std::isfinite(timeout.GetSeconds()) || timeout.IsNegative())
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  const xiiTime deadline = xiiTime::Now() + timeout;
  if (!std::isfinite(deadline.GetSeconds()))
  {
    return HostError(xiiNetworkErrorCode::InvalidArgument);
  }
  while (true)
  {
    if (TryGetEvent(out_event))
    {
      return {};
    }
    XII_LOCK(m_pRuntime->m_EventsMutex);
    if (!m_pRuntime->m_Events.IsEmpty())
    {
      continue;
    }
    if (!m_pRuntime->m_bRunning.load())
    {
      return HostError(xiiNetworkErrorCode::NotRunning);
    }
    const auto remaining = deadline - xiiTime::Now();
    if (remaining <= xiiTime::MakeZero())
    {
      return HostError(xiiNetworkErrorCode::TimedOut);
    }
    m_pRuntime->m_EventsMutex.UnlockWaitForSignalAndLock(remaining);
  }
}
