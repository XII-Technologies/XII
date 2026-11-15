/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>

using namespace xiiNetworkHostInternal;

void xiiNetworkHostRuntime::ProcessChannels(Peer& peer)
{
  xiiUInt32 uiControls = 0;
  for (xiiUInt32 uiSlot = 1; uiSlot <= peer.m_uiMaximumChannels && uiControls < 4 && peer.m_ControlQueue.GetCount() < 8; ++uiSlot)
  {
    auto& channel = peer.m_Channels[uiSlot];
    if ((channel.m_State == ChannelState::Opening || channel.m_State == ChannelState::Authorizing || channel.m_State == ChannelState::Closing) && xiiTime::Now() >= channel.m_Deadline)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::TimedOut));
      return;
    }
    if (!channel.m_bNeedsControl)
    {
      continue;
    }
    if (channel.m_State == ChannelState::Opening)
    {
      WireWriter writer;
      writer.U8(static_cast<xiiUInt8>(channel.m_Configuration.m_Delivery));
      writer.U8(channel.m_Configuration.m_uiWeight);
      writer.U32(channel.m_Configuration.m_uiMaximumQueuedMessages);
      writer.U64(channel.m_Configuration.m_uiMaximumQueuedBytes);
      writer.U32(channel.m_Configuration.m_uiService);
      Queue(peer, xiiNetworkFrameType::OpenChannel, writer, channel.m_uiId);
      channel.m_bNeedsControl = false;
      ++uiControls;
    }
    else if (channel.m_State == ChannelState::Authorizing)
    {
      WireWriter writer;
      writer.U32(static_cast<xiiUInt32>(channel.m_bAccept ? xiiNetworkErrorCode::Success : xiiNetworkErrorCode::PermissionDenied));
      Queue(peer, xiiNetworkFrameType::ChannelAccepted, writer, channel.m_uiId);
      channel.m_State         = channel.m_bAccept ? ChannelState::Open : ChannelState::Closed;
      channel.m_bNeedsControl = false;
      if (!ChannelEvent(peer, channel, channel.m_bAccept ? xiiNetworkEventType::ChannelOpened : xiiNetworkEventType::ChannelClosed,
                        channel.m_bAccept ? xiiNetworkError() : xiiNetworkError(xiiNetworkErrorCode::PermissionDenied)))
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
      }
      if (!channel.m_bAccept)
      {
        ReleaseEvents(channel.m_uiReservedEvents);
        channel.m_uiReservedEvents = 0;
      }
      ++uiControls;
    }
    else if (channel.m_State == ChannelState::Closing && channel.m_SendQueue.IsEmpty())
    {
      const auto      mode       = channel.m_Configuration.m_Delivery;
      const bool      bReliable  = mode == xiiNetworkDeliveryMode::ReliableOrdered || mode == xiiNetworkDeliveryMode::ReliableUnordered;
      const xiiUInt64 uiReceived = mode == xiiNetworkDeliveryMode::ReliableOrdered ? channel.m_uiLastReceiveSequence : channel.m_Statistics.m_uiMessagesReceived;
      if (bReliable && channel.m_bHasPeerFinalSequence && uiReceived > channel.m_uiPeerFinalSequence)
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
        return;
      }
      const bool bReceivedAll = !bReliable || (channel.m_bHasPeerFinalSequence && uiReceived == channel.m_uiPeerFinalSequence);
      if (channel.m_bCloseReceived && !bReceivedAll)
      {
        continue;
      }
      WireWriter writer;
      if (channel.m_bCloseReceived)
      {
        writer.U8(1);
        writer.U64(channel.m_uiNextSendSequence - 1);
        Queue(peer, xiiNetworkFrameType::CloseChannel, writer, channel.m_uiId);
        channel.m_bCloseAcknowledged = true;
      }
      else if (!channel.m_bCloseSent)
      {
        writer.U8(0);
        writer.U64(channel.m_uiNextSendSequence - 1);
        Queue(peer, xiiNetworkFrameType::CloseChannel, writer, channel.m_uiId);
        channel.m_bCloseSent = true;
      }
      channel.m_bNeedsControl = channel.m_bCloseAcknowledged && !bReceivedAll;
      if (channel.m_bCloseAcknowledged && bReceivedAll)
      {
        FinishQuicChannel(peer, channel);
        channel.m_State = ChannelState::Closed;
        if (!ChannelEvent(peer, channel, xiiNetworkEventType::ChannelClosed))
        {
          Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
        }
        ReleaseEvents(channel.m_uiReservedEvents);
        channel.m_uiReservedEvents = 0;
      }
      ++uiControls;
    }
  }
  if (uiControls == 4)
  {
    m_WorkSignal.RaiseSignal();
  }
}

void xiiNetworkHostRuntime::ProcessChannelFrame(Peer& peer, xiiNetworkFrame&& frame)
{
  WireReader reader(frame.m_Payload.GetBytes());
  const auto uiId         = frame.m_Header.m_uiChannel;
  const auto uiSlot       = uiId & 0xFFFFU;
  const auto uiGeneration = uiId >> 16;
  if (frame.m_Header.m_uiSequence != 0 || uiSlot == 0 || uiSlot > peer.m_uiMaximumChannels || uiGeneration == 0)
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
    return;
  }
  auto& channel = peer.m_Channels[uiSlot];
  if (frame.m_Header.m_Type == xiiNetworkFrameType::OpenChannel)
  {
    const auto delivery   = static_cast<xiiNetworkDeliveryMode>(reader.U8());
    const auto uiWeight   = reader.U8();
    const auto uiMessages = reader.U32();
    const auto uiBytes    = reader.U64();
    const auto uiService  = reader.U32();
    if (!reader.Done() || uiSlot == 1 || (uiSlot & 1U) != (peer.m_bIncoming ? 1U : 0U) ||
        (channel.m_State != ChannelState::Free && channel.m_State != ChannelState::Closed) || !channel.m_SendQueue.IsEmpty() ||
        channel.m_uiReservedEvents != 0 || uiGeneration != static_cast<xiiUInt32>(channel.m_uiGeneration) + 1 ||
        uiWeight == 0 || uiWeight > 16 || uiMessages == 0 || uiBytes == 0)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
      return;
    }
    xiiNetworkErrorCode code = xiiNetworkErrorCode::Success;
    if ((peer.m_uiDeliveryModes & xiiNetworkDeliveryMask(delivery)) == 0)
    {
      code = xiiNetworkErrorCode::UnsupportedCapability;
    }
    if (peer.m_bIncoming && !peer.m_Authorization.m_bMayOpenChannels)
    {
      code = xiiNetworkErrorCode::PermissionDenied;
    }
    if (code != xiiNetworkErrorCode::Success)
    {
      WireWriter writer;
      writer.U32(static_cast<xiiUInt32>(code));
      Queue(peer, xiiNetworkFrameType::ChannelAccepted, writer, uiId);
      channel.m_uiGeneration = static_cast<xiiUInt16>(uiGeneration);
      channel.m_uiId         = uiId;
      channel.m_State        = ChannelState::Closed;
      return;
    }
    const xiiUInt32 uiRequired = m_Configuration.m_bRequireChannelAuthorization ? 3U : 2U;
    xiiUInt32       uiReserved = 0;
    while (uiReserved < uiRequired && ReserveEvent())
    {
      ++uiReserved;
    }
    if (uiReserved != uiRequired)
    {
      ReleaseEvents(uiReserved);
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
      return;
    }
    channel.m_uiReservedEvents                        = uiReserved;
    channel.m_uiGeneration                            = static_cast<xiiUInt16>(uiGeneration);
    channel.m_uiId                                    = uiId;
    channel.m_Configuration.m_Delivery                = delivery;
    channel.m_Configuration.m_uiWeight                = uiWeight;
    channel.m_Configuration.m_uiMaximumQueuedMessages = xiiMath::Min(uiMessages, m_Configuration.m_uiMaximumQueuedMessages);
    channel.m_Configuration.m_uiMaximumQueuedBytes    = xiiMath::Min(uiBytes, m_Configuration.m_uiMaximumPeerSendBytes);
    channel.m_Configuration.m_uiService               = uiService;
    channel.m_Statistics                              = {};
    channel.m_uiNextSendSequence                      = 1;
    channel.m_uiLastReceiveSequence                   = 0;
    channel.m_bLocal                                  = false;
    channel.m_bCloseReceived = channel.m_bCloseSent = channel.m_bCloseAcknowledged = false;
    channel.m_State                                                                = ChannelState::Authorizing;
    channel.m_Deadline                                                             = xiiTime::Now() + m_Configuration.m_AuthenticationTimeout;
    channel.m_bHasPeerFinalSequence                                                = false;
    channel.m_pOrderedReceive                                                      = nullptr;
    channel.m_bAccept                                                              = !m_Configuration.m_bRequireChannelAuthorization;
    channel.m_bNeedsControl                                                        = !m_Configuration.m_bRequireChannelAuthorization;
    if (m_Configuration.m_bRequireChannelAuthorization && !ChannelEvent(peer, channel, xiiNetworkEventType::ChannelRequested))
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
    }
    return;
  }
  if (channel.m_uiId != uiId)
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::ChannelClosed));
    return;
  }
  if (frame.m_Header.m_Type == xiiNetworkFrameType::ChannelAccepted)
  {
    const auto uiCode = reader.U32();
    if (!reader.Done() || uiCode > static_cast<xiiUInt32>(xiiNetworkErrorCode::UnsupportedCapability) || channel.m_State != ChannelState::Opening || !channel.m_bLocal)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
      return;
    }
    const auto error = xiiNetworkError(static_cast<xiiNetworkErrorCode>(uiCode));
    channel.m_State  = error.Succeeded() ? ChannelState::Open : ChannelState::Closed;
    if (!ChannelEvent(peer, channel, error.Succeeded() ? xiiNetworkEventType::ChannelOpened : xiiNetworkEventType::ChannelClosed, error))
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
    }
    if (error.Failed())
    {
      ReleaseEvents(channel.m_uiReservedEvents);
      channel.m_uiReservedEvents = 0;
    }
    return;
  }
  if (frame.m_Header.m_Type == xiiNetworkFrameType::CloseChannel)
  {
    const auto uiAcknowledged  = reader.U8();
    const auto uiFinalSequence = reader.U64();
    if (reader.Done() && uiAcknowledged == 1 && channel.m_State == ChannelState::Closed && channel.m_bCloseSent && channel.m_bCloseReceived && channel.m_bCloseAcknowledged && channel.m_uiPeerFinalSequence == uiFinalSequence)
    {
      return;
    }
    if (!reader.Done() || uiAcknowledged > 1 || channel.m_State == ChannelState::Free || channel.m_State == ChannelState::Closed ||
        (uiAcknowledged == 1 && !channel.m_bCloseSent))
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
      return;
    }
    channel.m_State                 = ChannelState::Closing;
    channel.m_Deadline              = xiiTime::Now() + m_Configuration.m_CloseTimeout;
    channel.m_uiPeerFinalSequence   = uiFinalSequence;
    channel.m_bHasPeerFinalSequence = true;
    channel.m_bCloseReceived        = uiAcknowledged == 0;
    channel.m_bCloseAcknowledged    = uiAcknowledged == 1;
    channel.m_bNeedsControl         = true;
    return;
  }
  Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
}
