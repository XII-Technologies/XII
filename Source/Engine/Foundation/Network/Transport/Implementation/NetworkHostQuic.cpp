/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Threading/Lock.h>

#include <utility>

using namespace xiiNetworkHostInternal;

bool xiiNetworkHostRuntime::ProcessQuic()
{
  xiiNetworkQuicEvent event;
  xiiUInt32           uiCount = 0;
  for (; uiCount < 256 && m_pQuic->Poll(event); ++uiCount)
  {
    CompleteQuic(std::move(event));
  }
  if (uiCount == 256)
  {
    m_WorkSignal.RaiseSignal();
  }
  return uiCount != 0;
}

void xiiNetworkHostRuntime::CompleteQuic(xiiNetworkQuicEvent&& event)
{
  auto& connection = *event.m_pConnection;
  if (event.m_Type == xiiNetworkQuicEventType::Incoming)
  {
    Peer* pPeer = nullptr;
    {
      XII_LOCK(m_AdmissionMutex);
      auto* pListener = Find(event.m_Listener);
      if (pListener != nullptr && !m_bStopping.load())
      {
        XII_LOCK(pListener->m_Mutex);
        if (pListener->m_bInUse && !pListener->m_bClosing && pListener->m_Handle == event.m_Listener)
        {
          AllocatePeer(true, pPeer).IgnoreResult();
        }
      }
      if (pPeer != nullptr)
      {
        XII_LOCK(pPeer->m_Mutex);
        pPeer->m_bInUse           = true;
        pPeer->m_bStarted         = true;
        pPeer->m_uiAttempt        = m_uiNextAttempt.fetch_add(1);
        pPeer->m_AttemptStartedAt = xiiTime::Now();
        pPeer->m_pQuic            = &connection;
        pPeer->m_Info.m_State     = xiiNetworkConnectionState::Securing;
        const auto error          = m_pQuic->Accept(connection, pPeer->m_pCredentials, pPeer->m_Handle);
        if (error.Failed())
        {
          Close(*pPeer, error);
        }
      }
    }
    if (pPeer == nullptr)
    {
      xiiUInt64 uiBytes  = 0;
      xiiUInt32 uiEvents = 0;
      m_pQuic->Close(&connection, uiBytes, uiEvents);
    }
    return;
  }

  const auto owner = m_pQuic->GetOwner(connection);
  auto*      pPeer = Find(owner);
  if (pPeer == nullptr)
  {
    return;
  }
  auto& peer = *pPeer;
  if (event.m_Type == xiiNetworkQuicEventType::StreamClosed)
  {
    {
      XII_LOCK(peer.m_Mutex);
      for (xiiUInt32 uiIndex = 0; uiIndex < peer.m_QuicDeferred.GetCount(); ++uiIndex)
      {
        if (peer.m_QuicDeferred[uiIndex].m_pStream == event.m_pStream)
        {
          // A stream can lose a pended receive only when the provider aborts it.
          Close(peer, xiiNetworkError(xiiNetworkErrorCode::ConnectionAborted));
          peer.m_QuicDeferred[uiIndex].m_pStream = nullptr;
        }
      }
      auto& receive = m_pQuic->GetReceiveState(*event.m_pStream);
      if (auto* pChannel = Find(peer, {peer.m_Handle, receive.m_uiBoundChannel, peer.m_uiAttempt});
          pChannel != nullptr && pChannel->m_pOrderedReceive == &receive)
      {
        pChannel->m_pOrderedReceive         = nullptr;
        pChannel->m_bOrderedReceiveFinished = true;
      }
      if (!peer.m_bClosing && receive.m_bRequireControlFrames)
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed));
      }
      if (!peer.m_bClosing && receive.m_bRequireDataFrames && !receive.m_Decoder.IsAtFrameBoundary())
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
      }
    }
    xiiUInt64 uiBytes  = 0;
    xiiUInt32 uiEvents = 0;
    m_pQuic->CloseStream(event.m_pStream, uiBytes, uiEvents);
    peer.m_uiReceiveBytes.fetch_sub(uiBytes);
    m_uiReceiveBytes.fetch_sub(uiBytes);
    ReleaseEvents(uiEvents);
    return;
  }

  XII_LOCK(peer.m_Mutex);
  if (peer.m_pQuic != &connection || peer.m_Handle != owner || peer.m_bClosing)
  {
    return;
  }
  switch (event.m_Type)
  {
    case xiiNetworkQuicEventType::Certificate:
    {
      const auto error = m_pQuic->VerifyCertificate(connection, event, peer.m_Certificate);
      if (error.Failed())
      {
        Close(peer, error);
      }
      break;
    }
    case xiiNetworkQuicEventType::Connected:
    {
      const auto error = m_pQuic->GetEndpoints(connection, peer.m_Info.m_LocalEndpoint, peer.m_Info.m_RemoteEndpoint);
      if (error.Failed())
      {
        Close(peer, error);
      }
      else
      {
        BeginProtocol(peer);
      }
      break;
    }
    case xiiNetworkQuicEventType::Receive:
      if (!ReceiveQuic(peer, event))
      {
        peer.m_QuicDeferred.PushBack(std::move(event));
      }
      break;
    case xiiNetworkQuicEventType::Datagram:
      ReceiveQuicDatagram(peer, event);
      break;
    case xiiNetworkQuicEventType::Send:
    {
      auto&      outbound = *static_cast<Outbound*>(event.m_pSend);
      const auto uiSlot   = outbound.m_uiMessageSlot;
      if (uiSlot == InvalidSlot)
      {
        if (outbound.m_Header.m_Type == xiiNetworkFrameType::Hello || outbound.m_Header.m_Type == xiiNetworkFrameType::Welcome)
        {
          xiiNetworkSecurityInternal::ClearSecret(outbound.m_Payload.GetBytes());
        }
        peer.m_ControlQueue.PopFront();
        if (event.m_Error.Failed())
        {
          Close(peer, event.m_Error);
        }
      }
      else
      {
        const auto mode = peer.m_Channels[outbound.m_Header.m_uiChannel & 0xFFFFU].m_Configuration.m_Delivery;
        FinishSend(peer, uiSlot, event.m_Error);
        if (event.m_Error.Failed() && (mode == xiiNetworkDeliveryMode::ReliableOrdered || mode == xiiNetworkDeliveryMode::ReliableUnordered))
        {
          Close(peer, event.m_Error);
        }
      }
      break;
    }
    case xiiNetworkQuicEventType::Shutdown:
    {
      const bool bDraining = peer.m_Info.m_State == xiiNetworkConnectionState::Draining;
      if (event.m_bFin || event.m_Error.Failed())
      {
        peer.m_bTransportLost = !bDraining && (event.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed || event.m_Error.m_Code == xiiNetworkErrorCode::TimedOut || event.m_Error.m_Code == xiiNetworkErrorCode::ConnectionAborted);
        Close(peer, bDraining ? peer.m_CloseError : (event.m_Error.Failed() ? event.m_Error : xiiNetworkError(xiiNetworkErrorCode::ConnectionClosed)));
      }
      break;
    }
    case xiiNetworkQuicEventType::Incoming:
    case xiiNetworkQuicEventType::StreamClosed:
      break;
  }
}

bool xiiNetworkHostRuntime::ReceiveQuic(Peer& peer, xiiNetworkQuicEvent& event)
{
  auto& receive = m_pQuic->GetReceiveState(*event.m_pStream);
  if (receive.m_bRequireDataFrames)
  {
    if (!peer.m_bActivated)
    {
      return false;
    }
    if (receive.m_uiFrames == 0 && receive.m_Decoder.GetState() == xiiNetworkFrameDecodeState::Header)
    {
      receive.m_Decoder.SetMaximumPayloadBytes(peer.m_Info.m_uiMaximumMessageBytes).IgnoreResult();
    }
  }
  while (event.m_uiSpanIndex < event.m_Spans.GetCount() && !peer.m_bClosing)
  {
    auto&           bytes      = event.m_Spans[event.m_uiSpanIndex];
    const xiiUInt32 uiConsumed = Parse(peer, receive, bytes);
    if (receive.m_bAwaitingChannel)
    {
      bytes = bytes.GetSubArray(uiConsumed);
      return false;
    }
    ++event.m_uiSpanIndex;
  }
  if (event.m_bFin && !peer.m_bClosing && (!receive.m_Decoder.IsAtFrameBoundary() || (receive.m_bSingleFrame && receive.m_uiFrames != 1) || (receive.m_bRequireDataFrames && receive.m_uiFrames == 0)))
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
  }
  m_pQuic->CompleteReceive(*event.m_pStream, event.m_uiReceiveBytes);
  return true;
}

void xiiNetworkHostRuntime::ReceiveQuicDatagram(Peer& peer, xiiNetworkQuicEvent& event)
{
  const auto& header   = event.m_FrameHeader;
  auto*       pChannel = Find(peer, {peer.m_Handle, header.m_uiChannel, peer.m_uiAttempt});
  if (!peer.m_bActivated || pChannel == nullptr || pChannel->m_State != ChannelState::Open ||
      (pChannel->m_Configuration.m_Delivery != xiiNetworkDeliveryMode::UnreliableSequenced && pChannel->m_Configuration.m_Delivery != xiiNetworkDeliveryMode::UnreliableUnsequenced) ||
      header.m_uiSequence == 0 || header.m_uiPayloadBytes > peer.m_Info.m_uiMaximumMessageBytes)
  {
    ++peer.m_Info.m_Statistics.m_uiLocallyDroppedDatagrams;
    ++m_uiDroppedDatagrams;
    return;
  }
  const auto uiBytes = header.m_uiPayloadBytes;
  if (uiBytes > m_Configuration.m_uiMaximumReceiveBytes - m_uiReceiveBytes.load() ||
      uiBytes > m_Configuration.m_uiMaximumPeerReceiveBytes - peer.m_uiReceiveBytes.load() || !ReserveEvent())
  {
    ++peer.m_Info.m_Statistics.m_uiLocallyDroppedDatagrams;
    ++m_uiDroppedDatagrams;
    return;
  }
  peer.m_uiReceiveBytes.fetch_add(uiBytes);
  m_uiReceiveBytes.fetch_add(uiBytes);
  xiiNetworkFrame frame;
  frame.m_Header  = header;
  frame.m_Payload = std::move(event.m_Bytes);
  xiiNetworkReceiveState receive;
  receive.m_uiReservedBytes = uiBytes;
  receive.m_bEventReserved  = true;
  Frame(peer, std::move(frame), receive);
  if (receive.m_bEventReserved)
  {
    peer.m_uiReceiveBytes.fetch_sub(uiBytes);
    m_uiReceiveBytes.fetch_sub(uiBytes);
    ReleaseEvents(1);
  }
}

void xiiNetworkHostRuntime::DriveQuicOutput(Peer& peer, xiiTime now, xiiTime& ref_nextWake)
{
  if (peer.m_pQuic == nullptr || peer.m_bClosing)
  {
    return;
  }
  auto submit = [&](Outbound& outbound, xiiNetworkDeliveryMode mode) {
    if (outbound.m_uiHeaderBytes == 0)
    {
      const auto error = xiiNetworkFrame::EncodeHeader(outbound.m_Header, outbound.m_uiHeader, outbound.m_uiHeaderBytes);
      if (error.Failed())
      {
        return error;
      }
    }
    const xiiUInt32 uiNativeChannel = outbound.m_uiMessageSlot == InvalidSlot ? 0 : outbound.m_Header.m_uiChannel;
    auto            error           = m_pQuic->Send(*peer.m_pQuic, uiNativeChannel, mode, {outbound.m_uiHeader, outbound.m_uiHeaderBytes}, outbound.m_Payload.GetBytes(), &outbound);
    if (error.Succeeded())
    {
      outbound.m_bSubmitted = true;
    }
    return error;
  };
  if (!peer.m_ControlQueue.IsEmpty() && !peer.m_ControlQueue.PeekFront().m_bSubmitted)
  {
    const auto error = submit(peer.m_ControlQueue.PeekFront(), xiiNetworkDeliveryMode::ReliableOrdered);
    if (error.Failed() && error.m_Code != xiiNetworkErrorCode::WouldBlock)
    {
      Close(peer, error);
      return;
    }
  }
  if (!peer.m_bActivated)
  {
    return;
  }
  xiiUInt32 uiEmpty = 0;
  xiiUInt32 uiSent  = 0;
  while (uiSent < 32 && uiEmpty < peer.m_uiMaximumChannels)
  {
    auto&      channel    = peer.m_Channels[peer.m_uiScheduleChannel];
    const auto mode       = channel.m_Configuration.m_Delivery;
    Outbound*  pOutbound  = nullptr;
    xiiUInt32  uiInFlight = 0;
    for (const auto uiSlot : channel.m_SendQueue)
    {
      auto& outbound = m_Sends[uiSlot].m_Outbound;
      if (!outbound.m_bSubmitted)
      {
        pOutbound = &outbound;
        break;
      }
      if (mode == xiiNetworkDeliveryMode::ReliableOrdered && ++uiInFlight == 32)
      {
        break;
      }
    }
    bool bSent = false;
    if (pOutbound != nullptr)
    {
      auto       hostTokens = m_SendTokens;
      auto       peerTokens = peer.m_SendTokens;
      const auto uiBytes    = pOutbound->m_Header.m_uiPayloadBytes;
      if (hostTokens.Take(uiBytes, m_Configuration.m_uiSendBytesPerSecond, m_Configuration.m_uiSendBurstBytes, now, ref_nextWake) &&
          peerTokens.Take(uiBytes, m_Configuration.m_uiPeerSendBytesPerSecond, m_Configuration.m_uiPeerSendBurstBytes, now, ref_nextWake))
      {
        const auto error = submit(*pOutbound, mode);
        if (error.Succeeded())
        {
          m_SendTokens      = hostTokens;
          peer.m_SendTokens = peerTokens;
          bSent             = true;
          ++uiSent;
          uiEmpty = 0;
        }
        else if (error.m_Code != xiiNetworkErrorCode::WouldBlock)
        {
          if (mode == xiiNetworkDeliveryMode::ReliableOrdered || mode == xiiNetworkDeliveryMode::ReliableUnordered)
          {
            Close(peer, error);
            return;
          }
          FinishSend(peer, pOutbound->m_uiMessageSlot, error);
        }
      }
    }
    if (bSent)
    {
      if (peer.m_uiScheduleWeight == 0)
      {
        peer.m_uiScheduleWeight = channel.m_Configuration.m_uiWeight;
      }
      if (--peer.m_uiScheduleWeight != 0)
      {
        continue;
      }
    }
    else
    {
      ++uiEmpty;
    }
    peer.m_uiScheduleWeight  = 0;
    peer.m_uiScheduleChannel = peer.m_uiScheduleChannel % peer.m_uiMaximumChannels + 1;
  }
}

void xiiNetworkHostRuntime::FinishQuicChannel(Peer& peer, Channel& channel)
{
  if (peer.m_pQuic != nullptr)
  {
    m_pQuic->FinishChannel(*peer.m_pQuic, channel.m_uiId);
  }
}

void xiiNetworkHostRuntime::CloseQuicPeer(Peer& peer)
{
  xiiNetworkQuicConnection* pConnection = nullptr;
  {
    XII_LOCK(peer.m_Mutex);
    if (peer.m_bClosing)
    {
      pConnection = peer.m_pQuic;
    }
  }
  if (pConnection == nullptr)
  {
    return;
  }
  xiiUInt64 uiBytes  = 0;
  xiiUInt32 uiEvents = 0;
  {
    XII_LOCK(peer.m_Mutex);
    m_pQuic->UpdateStatistics(*pConnection, peer.m_Info.m_Statistics);
  }
  m_pQuic->Close(pConnection, uiBytes, uiEvents);
  {
    XII_LOCK(peer.m_Mutex);
    peer.m_pQuic = nullptr;
    peer.m_QuicDeferred.Clear();
    peer.m_uiReceiveBytes.fetch_sub(uiBytes);
    m_uiReceiveBytes.fetch_sub(uiBytes);
    ReleaseEvents(uiEvents);
  }
  m_WorkSignal.RaiseSignal();
}
