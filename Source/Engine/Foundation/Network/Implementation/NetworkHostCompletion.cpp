/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Threading/Lock.h>
#include <utility>

using namespace xiiNetworkHostInternal;

namespace
{
  constexpr xiiUInt64 ListenerTag     = 1ULL << 63;
  constexpr xiiUInt64 DatagramSendTag = 1ULL << 62;
} // namespace

void xiiNetworkHostRuntime::CompleteIO(xiiNetworkIOResult&& result)
{
  if ((result.m_uiUserData & ListenerTag) != 0)
  {
    const auto uiSlot   = static_cast<xiiUInt32>(result.m_uiUserData & ~ListenerTag);
    auto&      listener = m_Listeners[uiSlot];
    bool       bAccept  = false;
    {
      XII_LOCK(listener.m_Mutex);
      XII_ASSERT_DEV(listener.m_uiPendingIO != 0, "Listener operations retain their generation until native completion.");
      --listener.m_uiPendingIO;
      if (result.m_Kind == xiiNetworkIOOperationKind::Accept)
      {
        bAccept = result.Succeeded() && !listener.m_bClosing && !m_bStopping.load();
      }
      else
      {
        listener.m_bReceivePending = false;
        if (!listener.m_bClosing && !m_bStopping.load())
        {
          const auto uiBytes = result.m_uiBytesTransferred;
          if (uiBytes <= m_Configuration.m_uiMaximumReceiveBytes - m_uiReceiveBytes.load() && ReserveEvent())
          {
            xiiNetworkEvent event;
            event.m_Type     = xiiNetworkEventType::Datagram;
            event.m_Listener = listener.m_Handle;
            event.m_Endpoint = result.m_Endpoint;
            event.m_Error    = result.m_Error;
            if (!result.m_Buffers.IsEmpty())
            {
              event.m_Payload = std::move(result.m_Buffers[0]);
              event.m_Payload.SetCount(uiBytes).IgnoreResult();
            }
            m_uiReceiveBytes.fetch_add(event.m_Payload.GetCount());
            TrackMessage(false, event.m_Payload.GetCount());
            Publish(std::move(event));
          }
          else
          {
            ++m_uiDroppedDatagrams;
          }
        }
      }
    }
    if (bAccept)
    {
      XII_LOCK(m_AdmissionMutex);
      Peer* pPeer = nullptr;
      if (!m_bStopping.load() && AllocatePeer(true, pPeer).Succeeded())
      {
        XII_LOCK(pPeer->m_Mutex);
        auto error = ConfigureTcp(result.m_AcceptedSocket);
        if (error.Succeeded())
        {
          error = m_IO.Register(std::move(result.m_AcceptedSocket), pPeer->m_Registration);
        }
        pPeer->m_bInUse = true;
        if (error.Failed())
        {
          Close(*pPeer, error);
        }
      }
    }
    return;
  }
  if ((result.m_uiUserData & DatagramSendTag) != 0)
  {
    const auto uiSlot   = static_cast<xiiUInt32>(result.m_uiUserData & ~DatagramSendTag);
    auto&      send     = m_Sends[uiSlot];
    auto&      listener = m_Listeners[send.m_Listener.m_uiSlot];
    XII_LOCK(listener.m_Mutex);
    --listener.m_uiPendingIO;
    listener.m_uiMessageSlot = InvalidSlot;
    FinishDatagram(uiSlot, result.m_Error);
    return;
  }
  for (auto& peer : m_Peers)
  {
    XII_LOCK(peer.m_Mutex);
    if (!peer.m_bInUse || peer.m_uiAttempt != result.m_uiUserData)
    {
      continue;
    }
    XII_ASSERT_DEV(peer.m_uiPendingIO != 0, "Peer generation is retained through every native completion.");
    --peer.m_uiPendingIO;
    if (result.m_Kind == xiiNetworkIOOperationKind::Receive)
    {
      peer.m_ReceiveOperation = {};
    }
    if (peer.m_bClosing)
    {
      return;
    }
    if (result.m_Kind == xiiNetworkIOOperationKind::Resolve)
    {
      if (result.Failed())
      {
        Close(peer, result.m_Error);
        return;
      }
      peer.m_Endpoints = std::move(result.m_ResolvedEndpoints);
      if (peer.m_Endpoints.IsEmpty())
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::NameNotFound));
        return;
      }
      if (peer.m_Endpoints.GetCount() > 32)
      {
        peer.m_Endpoints.SetCount(32);
      }
      peer.m_bStarted = false;
      return;
    }
    if (result.m_Kind == xiiNetworkIOOperationKind::Connect)
    {
      if (result.Failed())
      {
        if (peer.m_uiEndpointIndex + 1 < peer.m_Endpoints.GetCount())
        {
          m_IO.Deregister(peer.m_Registration).IgnoreResult();
          peer.m_Registration = {};
          ++peer.m_uiEndpointIndex;
          peer.m_bStarted = false;
        }
        else
        {
          Close(peer, result.m_Error);
        }
      }
      else
      {
        StartTransport(peer);
      }
      return;
    }
    if (result.m_Kind == xiiNetworkIOOperationKind::Receive)
    {
      peer.m_Info.m_Statistics.m_uiWireBytesReceived += result.m_uiBytesTransferred;
      if (result.m_uiBytesTransferred != 0)
      {
        peer.m_CipherInput = std::move(result.m_Buffers[0]);
        peer.m_CipherInput.SetCount(result.m_uiBytesTransferred).IgnoreResult();
        peer.m_uiCipherOffset = 0;
      }
      if (result.m_Error.m_Code == xiiNetworkErrorCode::ConnectionClosed)
      {
        peer.m_bPeerEof       = true;
        peer.m_bTransportLost = peer.m_Info.m_State != xiiNetworkConnectionState::Draining;
      }
      else if (result.Failed())
      {
        peer.m_bTransportLost = true;
        Close(peer, result.m_Error);
      }
      return;
    }
    if (result.m_Kind == xiiNetworkIOOperationKind::Send)
    {
      peer.m_Info.m_Statistics.m_uiWireBytesSent += result.m_uiBytesTransferred;
      if (peer.m_pTls != nullptr)
      {
        peer.m_bCipherSendPending = false;
        peer.m_uiCipherCompleted += result.m_uiBytesTransferred;
      }
      else if (result.Succeeded())
      {
        if (peer.m_bActiveControl)
        {
          const auto type = peer.m_ControlQueue.PeekFront().m_Header.m_Type;
          if (type == xiiNetworkFrameType::Hello || type == xiiNetworkFrameType::Welcome)
          {
            for (auto& buffer : result.m_Buffers)
            {
              xiiNetworkSecurityInternal::ClearSecret(buffer.GetBytes());
            }
          }
          peer.m_ControlQueue.PopFront();
          peer.m_bActiveControl = false;
        }
        else
        {
          FinishSend(peer, peer.m_uiActiveSend, {});
          peer.m_uiActiveSend = InvalidSlot;
        }
      }
      if (result.Failed())
      {
        peer.m_bTransportLost = true;
        Close(peer, result.m_Error);
      }
      return;
    }
  }
}

void xiiNetworkHostRuntime::DriveListener(Listener& listener, xiiTime now, xiiTime& ref_nextWake)
{
  if (listener.m_bTerminalPublished)
  {
    if (listener.m_bTerminalConsumed)
    {
      listener.m_bInUse = false;
    }
    return;
  }
  if (listener.m_bClosing)
  {
    if (!listener.m_bDeregistered)
    {
      if (listener.m_pQuic != nullptr)
      {
        m_pQuic->CloseListener(listener.m_pQuic);
        listener.m_pQuic = nullptr;
      }
      else
      {
        m_IO.Deregister(listener.m_Registration).IgnoreResult();
      }
      listener.m_bDeregistered = true;
    }
    while (!listener.m_SendQueue.IsEmpty())
    {
      const auto uiSlot = listener.m_SendQueue.PeekFront();
      listener.m_SendQueue.PopFront();
      FinishDatagram(uiSlot, xiiNetworkError(xiiNetworkErrorCode::Cancelled));
    }
    if (listener.m_uiPendingIO == 0)
    {
      xiiNetworkEvent event;
      event.m_Type     = xiiNetworkEventType::ListenerClosed;
      event.m_Listener = listener.m_Handle;
      event.m_Error    = listener.m_Error;
      Publish(std::move(event));
      listener.m_bTerminalPublished = true;
    }
    return;
  }
  if (listener.m_pQuic != nullptr)
  {
    return;
  }
  xiiNetworkIOOptions options;
  options.m_uiUserData = ListenerTag | listener.m_Handle.m_uiSlot;
  if (m_Configuration.m_Transport == xiiNetworkTransportType::NativeUdp)
  {
    if (!listener.m_bReceivePending)
    {
      xiiNetworkIOBuffer buffer;
      auto               error = m_IO.AcquireBuffer(m_Capabilities.m_uiMaximumUnreliablePayload, buffer);
      if (error.Succeeded())
      {
        xiiNetworkIOBuffers buffers;
        buffers.PushBack(std::move(buffer));
        xiiNetworkIOOperation operation;
        error = m_IO.ReceiveFrom(listener.m_Registration, std::move(buffers), operation, options);
      }
      if (error.Failed())
      {
        listener.m_Error    = error;
        listener.m_bClosing = true;
        m_WorkSignal.RaiseSignal();
        return;
      }
      listener.m_bReceivePending = true;
      ++listener.m_uiPendingIO;
    }
    if (listener.m_uiMessageSlot == InvalidSlot && !listener.m_SendQueue.IsEmpty())
    {
      const auto uiSlot       = listener.m_SendQueue.PeekFront();
      auto&      send         = m_Sends[uiSlot];
      auto       hostTokens   = m_SendTokens;
      auto       socketTokens = listener.m_SendTokens;
      if (!hostTokens.Take(send.m_uiPayloadBytes, m_Configuration.m_uiSendBytesPerSecond, m_Configuration.m_uiSendBurstBytes, now, ref_nextWake) ||
          !socketTokens.Take(send.m_uiPayloadBytes, m_Configuration.m_uiPeerSendBytesPerSecond, m_Configuration.m_uiPeerSendBurstBytes, now, ref_nextWake))
      {
        return;
      }
      m_SendTokens          = hostTokens;
      listener.m_SendTokens = socketTokens;
      xiiNetworkIOBuffers buffers;
      buffers.PushBack(std::move(send.m_Outbound.m_Payload));
      xiiNetworkIOOperation operation;
      options.m_uiUserData = DatagramSendTag | uiSlot;
      const auto error     = m_IO.SendTo(listener.m_Registration, send.m_Destination, std::move(buffers), operation, options);
      listener.m_SendQueue.PopFront();
      if (error.Failed())
      {
        FinishDatagram(uiSlot, error);
        m_WorkSignal.RaiseSignal();
      }
      else
      {
        listener.m_uiMessageSlot = uiSlot;
        ++listener.m_uiPendingIO;
      }
    }
  }
  else
  {
    while (listener.m_uiPendingIO < 4)
    {
      xiiNetworkIOOperation operation;
      const auto            error = m_IO.Accept(listener.m_Registration, operation, options);
      if (error.Failed())
      {
        if (error.m_Code != xiiNetworkErrorCode::Backpressure)
        {
          listener.m_Error    = error;
          listener.m_bClosing = true;
          m_WorkSignal.RaiseSignal();
        }
        break;
      }
      ++listener.m_uiPendingIO;
    }
  }
}

xiiNetworkError xiiNetworkHost::SendDatagram(xiiNetworkListener handle, const xiiNetworkEndpoint& destination, xiiNetworkIOBuffer&& payload, xiiNetworkMessage& out_message, xiiUInt64 uiUserData)
{
  auto& runtime   = *m_pRuntime;
  auto* pListener = runtime.Find(handle);
  if (pListener == nullptr)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  XII_LOCK(pListener->m_Mutex);
  if (!pListener->m_bInUse || pListener->m_Handle != handle)
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidHandle);
  }
  if (runtime.m_Configuration.m_Transport != xiiNetworkTransportType::NativeUdp)
  {
    return xiiNetworkError(xiiNetworkErrorCode::UnsupportedOperation);
  }
  if (!IsRunning() || pListener->m_bClosing)
  {
    return xiiNetworkError(xiiNetworkErrorCode::ShuttingDown);
  }
  if (!runtime.OwnsBuffer(payload) || !destination.IsValid() || destination.GetAddress().GetFamily() != pListener->m_Endpoint.GetAddress().GetFamily())
  {
    return xiiNetworkError(xiiNetworkErrorCode::InvalidArgument);
  }
  if (payload.GetCount() > runtime.m_Capabilities.m_uiMaximumUnreliablePayload || payload.GetCount() > runtime.m_Configuration.m_uiMaximumMessageBytes)
  {
    return xiiNetworkError(xiiNetworkErrorCode::MessageTooLarge);
  }
  if (payload.GetCount() > runtime.m_Configuration.m_uiMaximumPeerSendBytes - pListener->m_uiQueuedBytes)
  {
    return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
  }
  const auto error = runtime.AllocateSend({}, std::move(payload), uiUserData, out_message);
  if (error.Succeeded())
  {
    auto& send         = runtime.m_Sends[out_message.m_uiSlot];
    send.m_Listener    = handle;
    send.m_Destination = destination;
    pListener->m_uiQueuedBytes += send.m_uiPayloadBytes;
    pListener->m_SendQueue.PushBack(out_message.m_uiSlot);
    runtime.m_WorkSignal.RaiseSignal();
  }
  return error;
}

void xiiNetworkHostRuntime::FinishDatagram(xiiUInt32 uiSlot, xiiNetworkError error)
{
  auto& send     = m_Sends[uiSlot];
  auto& listener = m_Listeners[send.m_Listener.m_uiSlot];
  listener.m_uiQueuedBytes -= send.m_uiPayloadBytes;
  if (error.Succeeded())
  {
    TrackMessage(true, send.m_uiPayloadBytes);
  }
  send.m_Outbound.m_Payload.Clear();
  send.m_bCompleted = true;
  xiiNetworkEvent event;
  event.m_Type       = xiiNetworkEventType::MessageSent;
  event.m_Listener   = send.m_Listener;
  event.m_Message    = send.m_Handle;
  event.m_Error      = error;
  event.m_uiUserData = send.m_uiUserData;
  Publish(std::move(event));
}
