/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/Lock.h>
#include <cmath>
#include <cstring>
#include <utility>

using namespace xiiNetworkHostInternal;

namespace
{
  bool RecoverableHostError(xiiNetworkErrorCode code)
  {
    return code == xiiNetworkErrorCode::ConnectionReset || code == xiiNetworkErrorCode::ConnectionAborted || code == xiiNetworkErrorCode::ConnectionRefused ||
      code == xiiNetworkErrorCode::NetworkUnreachable || code == xiiNetworkErrorCode::HostUnreachable || code == xiiNetworkErrorCode::TimedOut || code == xiiNetworkErrorCode::ConnectionClosed;
  }
} // namespace

void xiiNetworkHostRuntime::Run()
{
  if (m_Configuration.m_IO.m_bEnableProfiling)
  {
    xiiProfilingSystem::SetThreadName("Network Host");
  }
  xiiUInt32 uiPeerStart     = 0;
  xiiUInt32 uiListenerStart = 0;
  while (true)
  {
    if (m_pQuic != nullptr)
    {
      ProcessQuic();
    }
    xiiNetworkIOResult result;
    xiiUInt32          uiCompletions = 0;
    while (uiCompletions < 256 && m_IO.TryGetCompletion(result))
    {
      CompleteIO(std::move(result));
      ++uiCompletions;
    }
    if (uiCompletions == 256)
    {
      m_WorkSignal.RaiseSignal();
    }
    const auto now      = xiiTime::Now();
    auto       nextWake = now + xiiTime::MakeFromHours(24);
    bool       bActive  = false;
    for (xiiUInt32 uiIndex = 0; uiIndex < m_Listeners.GetCount(); ++uiIndex)
    {
      auto& listener = m_Listeners[(uiListenerStart + uiIndex) % m_Listeners.GetCount()];
      XII_LOCK(listener.m_Mutex);
      if (!listener.m_bInUse)
      {
        continue;
      }
      if (m_bStopping.load())
      {
        listener.m_bClosing = true;
      }
      DriveListener(listener, now, nextWake);
      bActive |= listener.m_bInUse && !listener.m_bTerminalPublished;
    }
    uiListenerStart = (uiListenerStart + 1) % m_Listeners.GetCount();
    for (xiiUInt32 uiIndex = 0; uiIndex < m_Peers.GetCount(); ++uiIndex)
    {
      auto& peer = m_Peers[(uiPeerStart + uiIndex) % m_Peers.GetCount()];
      XII_LOCK(peer.m_Mutex);
      if (!peer.m_bInUse || peer.m_bTerminalPublished)
      {
        continue;
      }
      if (m_bStopping.load() && !peer.m_bCloseRequested)
      {
        peer.m_bCloseRequested = true;
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ShuttingDown), peer.m_bActivated);
      }
      if (m_Configuration.m_IO.m_bEnableProfiling)
      {
        XII_PROFILE_SCOPE("NetworkHost.Peer");
        DrivePeer(peer, now, nextWake);
      }
      else
      {
        DrivePeer(peer, now, nextWake);
      }
      bActive |= !peer.m_bTerminalPublished;
    }
    uiPeerStart = (uiPeerStart + 1) % m_Peers.GetCount();
    if (m_pQuic != nullptr)
    {
      for (auto& peer : m_Peers)
      {
        CloseQuicPeer(peer);
      }
    }
    if (m_bStopping.load() && !bActive)
    {
      if (m_pQuic != nullptr && ProcessQuic())
      {
        continue;
      }
      break;
    }
    const auto remaining = nextWake - xiiTime::Now();
    if (remaining > xiiTime::MakeZero())
    {
      m_WorkSignal.WaitForSignal(remaining);
    }
  }
  if (m_Configuration.m_IO.m_bEnableProfiling)
  {
    xiiProfilingSystem::RemoveThread();
  }
}

void xiiNetworkHostRuntime::StartAttempt(Peer& peer)
{
  peer.m_bStarted     = true;
  peer.m_uiAttempt    = m_uiNextAttempt.fetch_add(1);
  peer.m_LastActivity = xiiTime::Now();
  if (peer.m_uiEndpointIndex == 0 && peer.m_Info.m_State != xiiNetworkConnectionState::Resolving)
  {
    peer.m_AttemptStartedAt = peer.m_LastActivity;
    peer.m_Deadline         = peer.m_LastActivity + m_Configuration.m_HandshakeTimeout;
  }
  if (peer.m_bIncoming)
  {
    StartTransport(peer);
    return;
  }
  if (peer.m_Endpoints.IsEmpty())
  {
    peer.m_Info.m_State = xiiNetworkConnectionState::Resolving;
    xiiNetworkIOOperation operation;
    xiiNetworkIOOptions   options;
    options.m_uiUserData = peer.m_uiAttempt;
    options.m_Timeout    = m_Configuration.m_HandshakeTimeout;
    const auto error     = m_IO.Resolve(peer.m_sHost, peer.m_sService, operation, xiiNetworkAddressFamily::Unspecified, m_pQuic != nullptr ? xiiNetworkTransport::Udp : xiiNetworkTransport::Tcp, options);
    if (error.Failed())
    {
      Close(peer, error);
    }
    else
    {
      ++peer.m_uiPendingIO;
    }
    return;
  }
  peer.m_Info.m_State = xiiNetworkConnectionState::Connecting;
  const auto endpoint = peer.m_Endpoints[peer.m_uiEndpointIndex];
  if (m_pQuic != nullptr)
  {
    const auto remaining         = xiiMath::Max(peer.m_Deadline - xiiTime::Now(), xiiTime::MakeZero());
    const auto uiCandidates      = peer.m_Endpoints.GetCount() - peer.m_uiEndpointIndex;
    peer.m_QuicCandidateDeadline = xiiTime::Now() + remaining / static_cast<double>(uiCandidates);
    const auto error             = m_pQuic->Connect(endpoint, peer.m_Options.m_sServerName, peer.m_pCredentials, peer.m_Handle, peer.m_pQuic);
    if (error.Failed() && !TryNextQuicEndpoint(peer, error))
    {
      Close(peer, error);
    }
    return;
  }
  xiiTcpSocket socket;
  auto         error = socket.Open(endpoint.GetAddress().GetFamily());
  if (error.Succeeded())
  {
    error = ConfigureTcp(socket);
  }
  if (error.Succeeded())
  {
    error = m_IO.Register(std::move(socket), peer.m_Registration);
  }
  if (error.Succeeded())
  {
    xiiNetworkIOOperation operation;
    xiiNetworkIOOptions   options;
    options.m_uiUserData = peer.m_uiAttempt;
    options.m_Timeout    = xiiMath::Max(peer.m_Deadline - xiiTime::Now(), xiiTime::MakeZero());
    error                = m_IO.Connect(peer.m_Registration, endpoint, operation, options);
  }
  if (error.Failed())
  {
    Close(peer, error);
  }
  else
  {
    ++peer.m_uiPendingIO;
  }
}

void xiiNetworkHostRuntime::StartTransport(Peer& peer)
{
  auto error = m_IO.GetLocalEndpoint(peer.m_Registration, peer.m_Info.m_LocalEndpoint);
  if (error.Succeeded())
  {
    error = m_IO.GetPeerEndpoint(peer.m_Registration, peer.m_Info.m_RemoteEndpoint);
  }
  if (error.Failed())
  {
    Close(peer, error);
    return;
  }
  if (m_Configuration.m_Transport == xiiNetworkTransportType::TlsTcp)
  {
    peer.m_Info.m_State = xiiNetworkConnectionState::Securing;
    peer.m_pTls         = XII_DEFAULT_NEW(xiiNetworkTlsStream);
    error               = peer.m_pTls->Initialize(peer.m_pCredentials, peer.m_bIncoming, peer.m_Options.m_sServerName, peer.m_Info.m_RemoteEndpoint.GetPort());
    if (error.Failed())
    {
      Close(peer, error);
      return;
    }
  }
  else
  {
    BeginProtocol(peer);
  }
  Receive(peer);
}

void xiiNetworkHostRuntime::BeginProtocol(Peer& peer)
{
  peer.m_Info.m_State = xiiNetworkConnectionState::Negotiating;
  if (!peer.m_bIncoming)
  {
    SendHello(peer);
  }
}

void xiiNetworkHostRuntime::Receive(Peer& peer)
{
  if (m_pQuic != nullptr)
  {
    return;
  }
  if (peer.m_Info.m_State == xiiNetworkConnectionState::Connecting || peer.m_Info.m_State == xiiNetworkConnectionState::Resolving || peer.m_Info.m_State == xiiNetworkConnectionState::Reconnecting)
  {
    return;
  }
  if (peer.m_bClosing || peer.m_bPeerEof || peer.m_bDeregistered || peer.m_ReceiveOperation.IsValid() || peer.m_CipherInput.IsValid() || !peer.m_Registration.IsValid())
  {
    return;
  }
  xiiNetworkIOBuffer buffer;
  auto               error = m_IO.AcquireBuffer(16384, buffer);
  if (error.Failed())
  {
    Close(peer, error);
    return;
  }
  xiiNetworkIOBuffers buffers;
  buffers.PushBack(std::move(buffer));
  xiiNetworkIOOptions options;
  options.m_uiUserData   = peer.m_uiAttempt;
  options.m_TransferMode = xiiNetworkIOTransferMode::Some;
  error                  = m_IO.Receive(peer.m_Registration, std::move(buffers), peer.m_ReceiveOperation, options);
  if (error.Failed())
  {
    Close(peer, error);
  }
  else
  {
    ++peer.m_uiPendingIO;
  }
}

void xiiNetworkHostRuntime::DriveInput(Peer& peer)
{
  if (m_pQuic != nullptr)
  {
    return;
  }
  if (peer.m_bClosing)
  {
    return;
  }
  if (peer.m_pTls == nullptr)
  {
    if (peer.m_CipherInput.IsValid())
    {
      Parse(peer, peer.m_CipherInput.GetBytes());
      peer.m_CipherInput.Clear();
    }
    if (peer.m_bPeerEof)
    {
      if (!peer.m_Receive.m_Decoder.IsAtFrameBoundary())
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
      }
      else if (peer.m_Info.m_State == xiiNetworkConnectionState::Draining)
      {
        Close(peer, peer.m_CloseError);
      }
      else
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed));
      }
    }
    return;
  }
  auto& tls = *peer.m_pTls;
  for (xiiUInt32 uiIteration = 0; uiIteration < 32 && !peer.m_bClosing; ++uiIteration)
  {
    bool bProgress = false;
    if (peer.m_CipherInput.IsValid())
    {
      const auto input = peer.m_CipherInput.GetBytes().GetSubArray(peer.m_uiCipherOffset);
      const auto feed  = tls.FeedCiphertext(input);
      if (feed.Failed() && feed.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock)
      {
        Close(peer, feed.m_Error);
        return;
      }
      peer.m_uiCipherOffset += feed.m_uiBytesTransferred;
      bProgress |= feed.m_uiBytesTransferred != 0;
      if (peer.m_uiCipherOffset == peer.m_CipherInput.GetCount())
      {
        peer.m_CipherInput.Clear();
        peer.m_uiCipherOffset = 0;
      }
    }
    if (peer.m_bPeerEof && !peer.m_CipherInput.IsValid())
    {
      tls.NotifyEndOfInput();
    }
    if (!tls.IsEstablished())
    {
      const auto error = tls.Handshake();
      if (error.Succeeded())
      {
        const auto certificate = tls.GetPeerCertificate(peer.m_Certificate);
        if (certificate.Failed() && (!peer.m_bIncoming || peer.m_pCredentials->GetConfiguration().m_bRequireClientCertificate))
        {
          Close(peer, certificate);
          return;
        }
        BeginProtocol(peer);
        bProgress = true;
      }
      else if (error.m_Code != xiiNetworkErrorCode::WouldBlock)
      {
        Close(peer, error);
        return;
      }
    }
    else
    {
      xiiUInt8   uiPlaintext[16384] = {};
      const auto read               = tls.Read(uiPlaintext);
      if (read.m_uiBytesTransferred != 0)
      {
        Parse(peer, {uiPlaintext, read.m_uiBytesTransferred});
        xiiNetworkSecurityInternal::ClearSecret(uiPlaintext);
        bProgress = true;
      }
      if (read.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed)
      {
        peer.m_bTlsClosed = true;
        if (!peer.m_Receive.m_Decoder.IsAtFrameBoundary())
        {
          Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
        }
        else
        {
          Close(peer, xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed), true);
        }
        return;
      }
      if (read.Failed() && read.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock)
      {
        Close(peer, read.m_Error);
        return;
      }
    }
    if (!bProgress)
    {
      return;
    }
    if (uiIteration == 31)
    {
      m_WorkSignal.RaiseSignal();
    }
  }
}

void xiiNetworkHostRuntime::Parse(Peer& peer, xiiArrayPtr<const xiiUInt8> bytes)
{
  Parse(peer, peer.m_Receive, bytes);
}

xiiUInt32 xiiNetworkHostRuntime::Parse(Peer& peer, xiiNetworkReceiveState& receive, xiiArrayPtr<const xiiUInt8> bytes)
{
  xiiUInt32 uiOffset         = 0;
  receive.m_bAwaitingChannel = false;
  while (!peer.m_bClosing && (uiOffset < bytes.GetCount() || receive.m_Decoder.GetState() == xiiNetworkFrameDecodeState::NeedBuffer || receive.m_Decoder.GetState() == xiiNetworkFrameDecodeState::Ready))
  {
    xiiUInt32  uiConsumed = 0;
    const auto error      = receive.m_Decoder.Consume(bytes.GetSubArray(uiOffset), uiConsumed);
    uiOffset += uiConsumed;
    if (error.Failed())
    {
      Close(peer, error);
      return uiOffset;
    }
    const auto state = receive.m_Decoder.GetState();
    if (!receive.m_bHeaderValidated && (state == xiiNetworkFrameDecodeState::NeedBuffer || state == xiiNetworkFrameDecodeState::Ready))
    {
      const auto& header = receive.m_Decoder.GetHeader();
      if ((receive.m_bRequireControlFrames && header.m_Type == xiiNetworkFrameType::Data) ||
          (receive.m_bRequireDataFrames && header.m_Type != xiiNetworkFrameType::Data))
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
        return uiOffset;
      }
      if (receive.m_bRequireDataFrames)
      {
        auto* pChannel = Find(peer, {peer.m_Handle, header.m_uiChannel, peer.m_uiAttempt});
        if (pChannel != nullptr && pChannel->m_State == ChannelState::Opening)
        {
          receive.m_bAwaitingChannel = true;
          return uiOffset;
        }
        if (pChannel == nullptr || (pChannel->m_State != ChannelState::Open && pChannel->m_State != ChannelState::Closing) ||
            (pChannel->m_Configuration.m_Delivery != xiiNetworkDeliveryMode::ReliableOrdered && pChannel->m_Configuration.m_Delivery != xiiNetworkDeliveryMode::ReliableUnordered) ||
            (receive.m_uiBoundChannel != 0 && receive.m_uiBoundChannel != header.m_uiChannel) ||
            (pChannel->m_Configuration.m_Delivery == xiiNetworkDeliveryMode::ReliableUnordered && receive.m_uiFrames != 0))
        {
          Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
          return uiOffset;
        }
        if (pChannel->m_Configuration.m_Delivery == xiiNetworkDeliveryMode::ReliableOrdered)
        {
          if (pChannel->m_bOrderedReceiveFinished || (pChannel->m_pOrderedReceive != nullptr && pChannel->m_pOrderedReceive != &receive))
          {
            Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
            return uiOffset;
          }
          pChannel->m_pOrderedReceive = &receive;
        }
        receive.m_uiBoundChannel = header.m_uiChannel;
        receive.m_bSingleFrame   = pChannel->m_Configuration.m_Delivery == xiiNetworkDeliveryMode::ReliableUnordered;
      }
      receive.m_bHeaderValidated = true;
    }
    if (receive.m_Decoder.GetState() == xiiNetworkFrameDecodeState::NeedBuffer)
    {
      const auto& header  = receive.m_Decoder.GetHeader();
      const auto  uiBytes = header.m_uiPayloadBytes;
      if (uiBytes > m_Configuration.m_uiMaximumPeerReceiveBytes - peer.m_uiReceiveBytes.load() || uiBytes > m_Configuration.m_uiMaximumReceiveBytes - m_uiReceiveBytes.load())
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
        return uiOffset;
      }
      if (header.m_Type == xiiNetworkFrameType::Data)
      {
        if (!ReserveEvent())
        {
          Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
          return uiOffset;
        }
        receive.m_bEventReserved = true;
      }
      peer.m_uiReceiveBytes.fetch_add(uiBytes);
      m_uiReceiveBytes.fetch_add(uiBytes);
      receive.m_uiReservedBytes = uiBytes;
      xiiNetworkIOBuffer buffer;
      auto               allocation = m_IO.AcquireBuffer(uiBytes, buffer);
      if (allocation.Succeeded())
      {
        allocation = receive.m_Decoder.SupplyBuffer(std::move(buffer));
      }
      if (allocation.Failed())
      {
        Close(peer, allocation);
        return uiOffset;
      }
    }
    if (receive.m_Decoder.GetState() == xiiNetworkFrameDecodeState::Ready)
    {
      xiiNetworkFrame frame;
      const auto      taken = receive.m_Decoder.TakeFrame(frame);
      if (taken.Failed())
      {
        Close(peer, taken);
        return uiOffset;
      }
      Frame(peer, std::move(frame), receive);
      receive.m_bHeaderValidated = false;
      ++receive.m_uiFrames;
      if (receive.m_uiReservedBytes != 0)
      {
        peer.m_uiReceiveBytes.fetch_sub(receive.m_uiReservedBytes);
        m_uiReceiveBytes.fetch_sub(receive.m_uiReservedBytes);
        receive.m_uiReservedBytes = 0;
      }
      if (receive.m_bEventReserved)
      {
        ReleaseEvents(1);
        receive.m_bEventReserved = false;
      }
    }
    else if (uiConsumed == 0 && receive.m_Decoder.GetState() != xiiNetworkFrameDecodeState::Payload)
    {
      return uiOffset;
    }
  }
  return uiOffset;
}

void xiiNetworkHostRuntime::DrivePeer(Peer& peer, xiiTime now, xiiTime& ref_nextWake)
{
  if (!peer.m_bStarted && !peer.m_bClosing)
  {
    StartAttempt(peer);
  }
  if (peer.m_bCloseRequested && peer.m_Info.m_State != xiiNetworkConnectionState::Draining && !peer.m_bClosing)
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::Cancelled), peer.m_bGraceful && peer.m_bActivated);
  }
  if (peer.m_bClosing)
  {
    if (!peer.m_bDeregistered && peer.m_Registration.IsValid())
    {
      m_IO.Deregister(peer.m_Registration).IgnoreResult();
      peer.m_bDeregistered = true;
    }
    if (peer.m_uiPendingIO == 0 && peer.m_pQuic == nullptr)
    {
      FinishClose(peer);
    }
    return;
  }
  if (peer.m_Info.m_State == xiiNetworkConnectionState::Reconnecting)
  {
    if (now < peer.m_ReconnectAt)
    {
      ref_nextWake = xiiMath::Min(ref_nextWake, peer.m_ReconnectAt);
      return;
    }
    peer.m_bStarted     = false;
    peer.m_Info.m_State = xiiNetworkConnectionState::Connecting;
    StartAttempt(peer);
    return;
  }
  if (peer.m_Info.m_State != xiiNetworkConnectionState::Connected && now >= peer.m_Deadline)
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::TimedOut));
    m_WorkSignal.RaiseSignal();
    return;
  }
  if (peer.m_Info.m_State == xiiNetworkConnectionState::Authenticating && peer.m_bAuthorizationReady)
  {
    if (!peer.m_bAcceptAuthorization)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed));
      return;
    }
    SendWelcome(peer);
  }
  if (peer.m_uiSessionSlot != InvalidSlot && peer.m_bActivated)
  {
    XII_LOCK(m_SessionMutex);
    const auto& session = m_Sessions[peer.m_uiSessionSlot];
    if (!session.m_bInUse || now >= session.m_Created + m_Configuration.m_MaximumSessionLifetime)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::SessionExpired));
      return;
    }
  }
  if (peer.m_pQuic != nullptr)
  {
    if (peer.m_Info.m_State == xiiNetworkConnectionState::Connecting && !peer.m_bIncoming)
    {
      if (now >= peer.m_QuicCandidateDeadline && TryNextQuicEndpoint(peer, xiiNetworkError(xiiNetworkErrorCode::TimedOut)))
      {
        return;
      }
      if (peer.m_uiEndpointIndex + 1 < peer.m_Endpoints.GetCount())
      {
        ref_nextWake = xiiMath::Min(ref_nextWake, peer.m_QuicCandidateDeadline);
      }
    }
    const auto providerFailure = m_pQuic->GetFailure(*peer.m_pQuic);
    if (providerFailure.Failed())
    {
      Close(peer, providerFailure);
      return;
    }
    const auto uiDatagramBytes                              = m_pQuic->GetMaximumDatagramBytes(*peer.m_pQuic);
    peer.m_Info.m_Capabilities.m_uiMaximumUnreliablePayload = uiDatagramBytes > xiiNetworkFrame::HeaderCapacity ? uiDatagramBytes - xiiNetworkFrame::HeaderCapacity : 0;
    if (peer.m_Info.m_State != xiiNetworkConnectionState::Securing)
    {
      xiiUInt32 uiRetained = 0;
      for (xiiUInt32 uiIndex = 0; uiIndex < peer.m_QuicDeferred.GetCount() && !peer.m_bClosing; ++uiIndex)
      {
        auto& pending = peer.m_QuicDeferred[uiIndex];
        if (!ReceiveQuic(peer, pending))
        {
          if (uiIndex != uiRetained)
          {
            peer.m_QuicDeferred[uiRetained] = std::move(pending);
          }
          ++uiRetained;
        }
      }
      while (peer.m_QuicDeferred.GetCount() > uiRetained)
      {
        peer.m_QuicDeferred.PopBack();
      }
    }
    if (peer.m_bActivated)
    {
      if (now >= peer.m_QuicStatisticsAt)
      {
        m_pQuic->UpdateStatistics(*peer.m_pQuic, peer.m_Info.m_Statistics);
        peer.m_QuicStatisticsAt = now + xiiTime::MakeFromSeconds(1);
      }
      ref_nextWake = xiiMath::Min(ref_nextWake, peer.m_QuicStatisticsAt);
    }
  }
  DriveInput(peer);
  if (peer.m_bClosing)
  {
    m_WorkSignal.RaiseSignal();
    return;
  }
  if (peer.m_bActivated)
  {
    if (now >= peer.m_LastActivity + m_Configuration.m_IdleTimeout)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::TimedOut));
      m_WorkSignal.RaiseSignal();
      return;
    }
    ref_nextWake      = xiiMath::Min(ref_nextWake, peer.m_LastActivity + m_Configuration.m_IdleTimeout);
    const auto pingAt = peer.m_LastActivity + m_Configuration.m_HeartbeatInterval;
    if (now >= pingAt && peer.m_PingSent.IsZero())
    {
      peer.m_PingSent = now;
      ++peer.m_uiPingSequence;
      Queue(peer, xiiNetworkFrameType::Ping, {}, 0, peer.m_uiPingSequence);
    }
    if (peer.m_PingSent.IsZero())
    {
      ref_nextWake = xiiMath::Min(ref_nextWake, pingAt);
    }
    for (const auto& channel : peer.m_Channels)
    {
      if (channel.m_State == ChannelState::Opening || channel.m_State == ChannelState::Authorizing || channel.m_State == ChannelState::Closing)
      {
        ref_nextWake = xiiMath::Min(ref_nextWake, channel.m_Deadline);
      }
    }
    if (peer.m_pQuic != nullptr && peer.m_Info.m_State == xiiNetworkConnectionState::Draining)
    {
      for (auto& channel : peer.m_Channels)
      {
        if (channel.m_State == ChannelState::Open)
        {
          channel.m_State         = ChannelState::Closing;
          channel.m_Deadline      = peer.m_Deadline;
          channel.m_bNeedsControl = true;
        }
      }
    }
    ProcessChannels(peer);
  }
  DriveOutput(peer, now, ref_nextWake);
  if (peer.m_Info.m_State == xiiNetworkConnectionState::Draining && !peer.m_bClosing)
  {
    bool bQueued = peer.m_uiActiveSend != InvalidSlot || peer.m_bActiveControl || !peer.m_ControlQueue.IsEmpty() || peer.m_bCipherSendPending;
    for (const auto& channel : peer.m_Channels)
    {
      bQueued |= !channel.m_SendQueue.IsEmpty() ||
        (peer.m_pQuic != nullptr && peer.m_bActivated && channel.m_State != ChannelState::Free && channel.m_State != ChannelState::Closed);
    }
    if (!bQueued)
    {
      if (!peer.m_bDisconnectSent)
      {
        WireWriter writer;
        writer.U32(0);
        Queue(peer, xiiNetworkFrameType::Disconnect, writer);
        peer.m_bDisconnectSent = true;
        m_WorkSignal.RaiseSignal();
      }
      else if (peer.m_pQuic != nullptr)
      {
        m_pQuic->Shutdown(*peer.m_pQuic);
      }
      else if (peer.m_pTls != nullptr)
      {
        const auto error = peer.m_pTls->Shutdown();
        if (error.Succeeded())
        {
          peer.m_bTlsClosed = true;
        }
        else if (error.m_Code != xiiNetworkErrorCode::WouldBlock)
        {
          Close(peer, error);
          m_WorkSignal.RaiseSignal();
          return;
        }
        if (peer.m_bTlsClosed && peer.m_pTls->GetPendingCiphertextBytes() == 0 && !peer.m_bCipherSendPending)
        {
          Close(peer, peer.m_CloseError);
          m_WorkSignal.RaiseSignal();
        }
        else if (peer.m_pTls->GetPendingCiphertextBytes() != 0)
        {
          m_WorkSignal.RaiseSignal();
        }
      }
      else if (!peer.m_bShutdownSent)
      {
        const auto error = m_IO.ShutdownSocket(peer.m_Registration, xiiNetworkShutdown::Send);
        if (error.Failed() && error.m_Code != xiiNetworkErrorCode::InProgress)
        {
          Close(peer, error);
        }
        else if (error.Succeeded())
        {
          peer.m_bShutdownSent = true;
        }
      }
    }
  }
  Receive(peer);
  if (peer.m_Info.m_State != xiiNetworkConnectionState::Connected)
  {
    ref_nextWake = xiiMath::Min(ref_nextWake, peer.m_Deadline);
  }
}

void xiiNetworkHostRuntime::DriveOutput(Peer& peer, xiiTime now, xiiTime& ref_nextWake)
{
  if (m_pQuic != nullptr)
  {
    DriveQuicOutput(peer, now, ref_nextWake);
    return;
  }
  if (peer.m_bClosing || !peer.m_Registration.IsValid())
  {
    return;
  }
  auto finishControl = [&]() {
    auto& outbound = peer.m_ControlQueue.PeekFront();
    if (outbound.m_Header.m_Type == xiiNetworkFrameType::Hello || outbound.m_Header.m_Type == xiiNetworkFrameType::Welcome)
    {
      xiiNetworkSecurityInternal::ClearSecret(outbound.m_Payload.GetBytes());
    }
    peer.m_ControlQueue.PopFront();
    peer.m_bActiveControl = false;
  };
  for (xiiUInt32 uiIteration = 0; uiIteration < 32 && !peer.m_bClosing; ++uiIteration)
  {
    bool bProgress = false;
    if (peer.m_pTls != nullptr && !peer.m_bCipherSendPending && peer.m_pTls->GetPendingCiphertextBytes() != 0)
    {
      xiiNetworkIOBuffer buffer;
      auto               error = m_IO.AcquireBuffer(16384, buffer);
      if (error.Failed())
      {
        Close(peer, error);
        return;
      }
      const auto drained = peer.m_pTls->DrainCiphertext(buffer.GetBytes());
      if (drained.Failed())
      {
        Close(peer, drained.m_Error);
        return;
      }
      buffer.SetCount(drained.m_uiBytesTransferred).IgnoreResult();
      xiiNetworkIOBuffers buffers;
      buffers.PushBack(std::move(buffer));
      xiiNetworkIOOperation operation;
      xiiNetworkIOOptions   options;
      options.m_uiUserData = peer.m_uiAttempt;
      error                = m_IO.Send(peer.m_Registration, std::move(buffers), operation, options);
      if (error.Failed())
      {
        Close(peer, error);
        return;
      }
      peer.m_uiCipherProduced += drained.m_uiBytesTransferred;
      peer.m_bCipherSendPending = true;
      ++peer.m_uiPendingIO;
      bProgress = true;
    }
    if (peer.m_pTls != nullptr && !peer.m_pTls->IsEstablished())
    {
      return;
    }
    if (peer.m_uiActiveSend == InvalidSlot && !peer.m_bActiveControl)
    {
      if (!peer.m_ControlQueue.IsEmpty())
      {
        peer.m_bActiveControl = true;
      }
      else if (peer.m_bActivated)
      {
        for (xiiUInt32 uiChecked = 0; uiChecked < peer.m_uiMaximumChannels; ++uiChecked)
        {
          auto& channel = peer.m_Channels[peer.m_uiScheduleChannel];
          if (!channel.m_SendQueue.IsEmpty())
          {
            const auto uiSlot     = channel.m_SendQueue.PeekFront();
            const auto uiBytes    = m_Sends[uiSlot].m_uiPayloadBytes;
            auto       hostTokens = m_SendTokens;
            auto       peerTokens = peer.m_SendTokens;
            if (!hostTokens.Take(uiBytes, m_Configuration.m_uiSendBytesPerSecond, m_Configuration.m_uiSendBurstBytes, now, ref_nextWake) ||
                !peerTokens.Take(uiBytes, m_Configuration.m_uiPeerSendBytesPerSecond, m_Configuration.m_uiPeerSendBurstBytes, now, ref_nextWake))
            {
              break;
            }
            m_SendTokens        = hostTokens;
            peer.m_SendTokens   = peerTokens;
            peer.m_uiActiveSend = uiSlot;
            if (peer.m_uiScheduleWeight == 0)
            {
              peer.m_uiScheduleWeight = channel.m_Configuration.m_uiWeight;
            }
            --peer.m_uiScheduleWeight;
            if (peer.m_uiScheduleWeight == 0)
            {
              peer.m_uiScheduleChannel = peer.m_uiScheduleChannel % peer.m_uiMaximumChannels + 1;
            }
            break;
          }
          peer.m_uiScheduleWeight  = 0;
          peer.m_uiScheduleChannel = peer.m_uiScheduleChannel % peer.m_uiMaximumChannels + 1;
        }
      }
    }
    Outbound* pOutbound = peer.m_bActiveControl ? &peer.m_ControlQueue.PeekFront() : peer.m_uiActiveSend != InvalidSlot ? &m_Sends[peer.m_uiActiveSend].m_Outbound :
                                                                                                                          nullptr;
    if (pOutbound != nullptr)
    {
      auto& outbound = *pOutbound;
      if (outbound.m_uiHeaderBytes == 0)
      {
        const auto error = xiiNetworkFrame::EncodeHeader(outbound.m_Header, outbound.m_uiHeader, outbound.m_uiHeaderBytes);
        if (error.Failed())
        {
          Close(peer, error);
          return;
        }
      }
      if (peer.m_pTls != nullptr)
      {
        if (!outbound.m_bPlaintextComplete)
        {
          const auto bytes = outbound.m_uiOffset < outbound.m_uiHeaderBytes ? xiiArrayPtr<const xiiUInt8>(outbound.m_uiHeader + outbound.m_uiOffset, outbound.m_uiHeaderBytes - outbound.m_uiOffset) :
                                                                              xiiArrayPtr<const xiiUInt8>(outbound.m_Payload.GetBytes()).GetSubArray(outbound.m_uiOffset - outbound.m_uiHeaderBytes);
          const auto write = peer.m_pTls->Write(bytes);
          if (write.Failed() && write.m_Error.m_Code != xiiNetworkErrorCode::WouldBlock)
          {
            Close(peer, write.m_Error);
            return;
          }
          outbound.m_uiOffset += write.m_uiBytesTransferred;
          bProgress |= write.m_uiBytesTransferred != 0;
          if (outbound.m_uiOffset == outbound.m_uiHeaderBytes + outbound.m_Header.m_uiPayloadBytes)
          {
            outbound.m_bPlaintextComplete = true;
            outbound.m_uiCipherEnd        = peer.m_uiCipherProduced + peer.m_pTls->GetPendingCiphertextBytes();
          }
        }
        if (outbound.m_bPlaintextComplete && peer.m_uiCipherCompleted >= outbound.m_uiCipherEnd)
        {
          if (peer.m_bActiveControl)
          {
            finishControl();
          }
          else
          {
            FinishSend(peer, peer.m_uiActiveSend, {});
            peer.m_uiActiveSend = InvalidSlot;
          }
          bProgress = true;
        }
      }
      else if (!outbound.m_bSubmitted)
      {
        xiiNetworkIOBuffer header;
        auto               error = m_IO.AcquireBuffer(outbound.m_uiHeaderBytes, header);
        if (error.Failed())
        {
          Close(peer, error);
          return;
        }
        std::memcpy(header.GetBytes().GetPtr(), outbound.m_uiHeader, outbound.m_uiHeaderBytes);
        xiiNetworkIOBuffers buffers;
        buffers.PushBack(std::move(header));
        buffers.PushBack(std::move(outbound.m_Payload));
        xiiNetworkIOOperation operation;
        xiiNetworkIOOptions   options;
        options.m_uiUserData = peer.m_uiAttempt;
        error                = m_IO.Send(peer.m_Registration, std::move(buffers), operation, options);
        if (error.Failed())
        {
          Close(peer, error);
          return;
        }
        outbound.m_bSubmitted = true;
        ++peer.m_uiPendingIO;
        return;
      }
    }
    if (!bProgress)
    {
      return;
    }
    if (uiIteration == 31)
    {
      m_WorkSignal.RaiseSignal();
    }
  }
}

void xiiNetworkHostRuntime::FinishSend(Peer& peer, xiiUInt32 uiSlot, xiiNetworkError error)
{
  auto& send = m_Sends[uiSlot];
  if (send.m_bCompleted)
  {
    return;
  }
  auto& channel = peer.m_Channels[send.m_Channel.m_uiId & 0xFFFFU];
  if (channel.m_SendQueue.PeekFront() == uiSlot)
  {
    channel.m_SendQueue.PopFront();
  }
  else
  {
    xiiUInt32 uiIndex = 1;
    while (uiIndex < channel.m_SendQueue.GetCount() && channel.m_SendQueue[uiIndex] != uiSlot)
    {
      ++uiIndex;
    }
    XII_ASSERT_DEV(uiIndex < channel.m_SendQueue.GetCount(), "The accepted message retains its queue entry.");
    channel.m_SendQueue.RemoveAtAndCopy(uiIndex);
  }
  --channel.m_Statistics.m_uiQueuedMessages;
  channel.m_Statistics.m_uiQueuedBytes -= send.m_uiPayloadBytes;
  peer.m_Info.m_Statistics.m_uiQueuedSendBytes -= send.m_uiPayloadBytes;
  if (error.Succeeded())
  {
    ++channel.m_Statistics.m_uiMessagesSent;
    ++peer.m_Info.m_Statistics.m_uiMessagesSent;
    peer.m_Info.m_Statistics.m_uiPayloadBytesSent += send.m_uiPayloadBytes;
    TrackMessage(true, send.m_uiPayloadBytes);
  }
  send.m_Outbound.m_Payload.Clear();
  send.m_bCompleted = true;
  xiiNetworkEvent event;
  event.m_Type       = xiiNetworkEventType::MessageSent;
  event.m_Connection = peer.m_Handle;
  event.m_Channel    = send.m_Channel;
  event.m_Message    = send.m_Handle;
  event.m_Error      = error;
  event.m_uiUserData = send.m_uiUserData;
  Publish(std::move(event));
}

void xiiNetworkHostRuntime::Close(Peer& peer, xiiNetworkError error, bool bGraceful)
{
  if (peer.m_bTerminalPublished)
  {
    return;
  }
  if (peer.m_Info.m_State != xiiNetworkConnectionState::Draining || peer.m_CloseError.Succeeded())
  {
    peer.m_CloseError = error;
  }
  if (bGraceful && !peer.m_bClosing && peer.m_bStarted)
  {
    if (peer.m_Info.m_State != xiiNetworkConnectionState::Draining)
    {
      peer.m_Deadline = xiiTime::Now() + m_Configuration.m_CloseTimeout;
    }
    peer.m_Info.m_State = xiiNetworkConnectionState::Draining;
    peer.m_bGraceful    = true;
  }
  else
  {
    peer.m_bClosing = true;
    // Preserve a completed graceful exchange; forced/error closes detach resumable sessions only
    // when the prior protocol did not explicitly log out.
    if (peer.m_Info.m_State != xiiNetworkConnectionState::Draining)
    {
      peer.m_bGraceful = false;
    }
  }
  m_WorkSignal.RaiseSignal();
}

void xiiNetworkHostRuntime::FinishClose(Peer& peer)
{
  for (auto& channel : peer.m_Channels)
  {
    while (!channel.m_SendQueue.IsEmpty())
    {
      FinishSend(peer, channel.m_SendQueue.PeekFront(), xiiNetworkError(xiiNetworkErrorCode::Cancelled));
    }
    ReleaseEvents(channel.m_uiReservedEvents);
    channel.m_uiReservedEvents = 0;
    channel.m_State            = ChannelState::Closed;
  }
  peer.m_uiActiveSend   = InvalidSlot;
  peer.m_bActiveControl = false;
  for (auto& outbound : peer.m_ControlQueue)
  {
    if (outbound.m_Header.m_Type == xiiNetworkFrameType::Hello || outbound.m_Header.m_Type == xiiNetworkFrameType::Welcome)
    {
      xiiNetworkSecurityInternal::ClearSecret(outbound.m_Payload.GetBytes());
    }
  }
  peer.m_ControlQueue.Clear();
  peer.m_pTls.Clear();
  peer.m_CipherInput.Clear();
  peer.m_Registration     = {};
  peer.m_ReceiveOperation = {};
  peer.m_Receive.m_Decoder.Reset();
  peer.m_uiReceiveBytes.fetch_sub(peer.m_Receive.m_uiReservedBytes);
  m_uiReceiveBytes.fetch_sub(peer.m_Receive.m_uiReservedBytes);
  peer.m_Receive.m_uiReservedBytes = 0;
  if (peer.m_Receive.m_bEventReserved)
  {
    ReleaseEvents(1);
    peer.m_Receive.m_bEventReserved = false;
  }
  DetachSession(peer);
  const auto& retry = peer.m_Options.m_Reconnect;
  if (!peer.m_bIncoming && !peer.m_bCloseRequested && !m_bStopping.load() && retry.m_Mode != xiiNetworkReconnectMode::Disabled &&
      (RecoverableHostError(peer.m_CloseError.m_Code) || peer.m_bTransportLost) &&
      (retry.m_uiMaximumAttempts == 0 || peer.m_Info.m_uiReconnectAttempts < retry.m_uiMaximumAttempts))
  {
    const auto uiAttempt = ++peer.m_Info.m_uiReconnectAttempts;
    double     fDelay    = retry.m_InitialDelay.GetSeconds();
    if (retry.m_Mode == xiiNetworkReconnectMode::Exponential)
    {
      fDelay *= std::pow(2.0, static_cast<double>(xiiMath::Min(uiAttempt - 1, 32U)));
    }
    fDelay                 = xiiMath::Min(fDelay, retry.m_MaximumDelay.GetSeconds());
    xiiUInt8   uiRandom[8] = {};
    const auto randomError = xiiNetworkSecurityInternal::RandomBytes(uiRandom);
    if (randomError.Succeeded())
    {
      xiiUInt64 uiValue = 0;
      std::memcpy(&uiValue, uiRandom, sizeof(uiValue));
      fDelay *= 1.0 + retry.m_fJitterFraction * (static_cast<double>(uiValue >> 11) / 9007199254740992.0 * 2.0 - 1.0);
      peer.m_ReconnectAt  = xiiTime::Now() + xiiTime::MakeFromSeconds(fDelay);
      peer.m_Info.m_State = xiiNetworkConnectionState::Reconnecting;
      peer.m_bClosing = peer.m_bDeregistered = peer.m_bPeerEof = peer.m_bTlsClosed = false;
      peer.m_bDisconnectSent = peer.m_bShutdownSent = peer.m_bCipherSendPending = false;
      peer.m_bHelloReceived = peer.m_bWelcomeReceived = peer.m_bReadyReceived = peer.m_bActivated = false;
      peer.m_bProtocolRejected                                                                    = false;
      peer.m_bAuthorizationReady = peer.m_bAcceptAuthorization = false;
      peer.m_bTransportLost                                    = false;
      peer.m_uiCipherProduced = peer.m_uiCipherCompleted = 0;
      peer.m_uiCipherOffset                              = 0;
      peer.m_uiSessionSlot                               = InvalidSlot;
      peer.m_uiEndpointIndex                             = 0;
      if (!peer.m_sHost.IsEmpty())
      {
        peer.m_Endpoints.Clear();
      }
      peer.m_Info.m_Session               = {};
      peer.m_Info.m_bSessionResumed       = false;
      peer.m_Info.m_uiMaximumMessageBytes = m_Configuration.m_uiMaximumMessageBytes;
      peer.m_uiMaximumChannels            = m_Configuration.m_uiMaximumChannelsPerConnection;
      peer.m_uiDeliveryModes              = m_Capabilities.m_uiDeliveryModes;
      peer.m_Channels.Clear();
      peer.m_Channels.SetCount(peer.m_uiMaximumChannels + 1);
      peer.m_Receive.m_Decoder.SetMaximumPayloadBytes(8192).IgnoreResult();
      {
        XII_LOCK(m_CredentialsMutex);
        peer.m_pCredentials = m_pCredentials;
      }
      if (PeerEvent(peer, xiiNetworkEventType::ReconnectScheduled, peer.m_CloseError))
      {
        m_WorkSignal.RaiseSignal();
        return;
      }
      peer.m_CloseError = xiiNetworkError(xiiNetworkErrorCode::Backpressure);
    }
    else
    {
      peer.m_CloseError = randomError;
    }
  }
  peer.m_Info.m_State         = xiiNetworkConnectionState::Closed;
  peer.m_bTerminalPublished   = true;
  peer.m_Info.m_TerminalError = peer.m_CloseError;
  TrackTerminal(peer.m_CloseError);
  [[maybe_unused]] const bool bPublished = PeerEvent(peer, xiiNetworkEventType::Closed, peer.m_CloseError);
  XII_ASSERT_DEV(bPublished, "An admitted peer reserves its terminal event.");
  ReleaseEvents(peer.m_uiReservedEvents);
  peer.m_uiReservedEvents = 0;
}
