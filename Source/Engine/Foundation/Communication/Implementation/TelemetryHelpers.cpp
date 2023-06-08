#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Profiling/Profiling.h>

void xiiTelemetry::QueueOutgoingMessage(TransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const void* pData, xiiUInt32 uiDataBytes)
{
  // unreliable packages can just be dropped
  if (tm == xiiTelemetry::Unreliable)
    return;

  XII_LOCK(GetTelemetryMutex());

  // add a new message to the queue
  MessageQueue& Queue = s_SystemMessages[uiSystemID];
  Queue.m_OutgoingQueue.PushBack();

  // and fill it out properly
  xiiTelemetryMessage& msg = Queue.m_OutgoingQueue.PeekBack();
  msg.SetMessageID(uiSystemID, uiMsgID);

  if (uiDataBytes > 0)
  {
    msg.GetWriter().WriteBytes(pData, uiDataBytes).IgnoreResult();
  }

  // if our outgoing queue has grown too large, dismiss older messages
  if (Queue.m_OutgoingQueue.GetCount() > Queue.m_uiMaxQueuedOutgoing)
    Queue.m_OutgoingQueue.PopFront(Queue.m_OutgoingQueue.GetCount() - Queue.m_uiMaxQueuedOutgoing);
}

void xiiTelemetry::FlushOutgoingQueues()
{
  static bool bRecursion = false;

  if (bRecursion)
    return;

  // if there is no connection to anyone (yet), don't do anything
  if (!IsConnectedToOther())
    return;

  bRecursion = true;

  XII_LOCK(GetTelemetryMutex());

  // go through all system types
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_OutgoingQueue.IsEmpty())
      continue;

    const xiiUInt32 uiCurCount = it.Value().m_OutgoingQueue.GetCount();

    // send all messages that are queued for this system
    for (xiiUInt32 i = 0; i < uiCurCount; ++i)
      Send(xiiTelemetry::Reliable, it.Value().m_OutgoingQueue[i]); // Send() will already update the network

    // check that they have not been queue again
    XII_ASSERT_DEV(it.Value().m_OutgoingQueue.GetCount() == uiCurCount, "Implementation Error: When queued messages are flushed, they should not get queued again.");

    it.Value().m_OutgoingQueue.Clear();
  }

  bRecursion = false;
}


xiiResult xiiTelemetry::ConnectToServer(const char* szConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return OpenConnection(Client, szConnectTo);
#else
  xiiLog::SeriousWarning("Enet is not compiled into this build, xiiTelemetry::ConnectToServer() will be ignored.");
  return XII_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::CreateServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (OpenConnection(Server).Failed())
  {
    xiiLog::Error("xiiTelemetry: Failed to open a connection as a server.");
    s_ConnectionMode = ConnectionMode::None;
  }
#else
  xiiLog::SeriousWarning("Enet is not compiled into this build, xiiTelemetry::CreateServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::AcceptMessagesForSystem(xiiUInt32 uiSystemID, bool bAccept, ProcessMessagesCallback callback, void* pPassThrough)
{
  XII_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_bAcceptMessages = bAccept;
  s_SystemMessages[uiSystemID].m_Callback        = callback;
  s_SystemMessages[uiSystemID].m_pPassThrough    = pPassThrough;
}

void xiiTelemetry::PerFrameUpdate()
{
  XII_PROFILE_SCOPE("Telemetry.PerFrameUpdate");
  XII_LOCK(GetTelemetryMutex());

  // Call each callback to process the incoming messages
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_IncomingQueue.IsEmpty() && it.Value().m_Callback)
      it.Value().m_Callback(it.Value().m_pPassThrough);
  }

  TelemetryEventData e;
  e.m_EventType = TelemetryEventData::PerFrameUpdate;

  const bool bAllowUpdate = s_bAllowNetworkUpdate;
  s_bAllowNetworkUpdate   = false;
  s_TelemetryEvents.Broadcast(e);
  s_bAllowNetworkUpdate = bAllowUpdate;
}

void xiiTelemetry::SetOutgoingQueueSize(xiiUInt32 uiSystemID, xiiUInt16 uiMaxQueued)
{
  XII_LOCK(GetTelemetryMutex());

  s_SystemMessages[uiSystemID].m_uiMaxQueuedOutgoing = uiMaxQueued;
}


bool xiiTelemetry::IsConnectedToOther()
{
  return ((s_ConnectionMode == Client && IsConnectedToServer()) || (s_ConnectionMode == Server && IsConnectedToClient()));
}

void xiiTelemetry::Broadcast(TransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const void* pData, xiiUInt32 uiDataBytes)
{
  if (s_ConnectionMode != xiiTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void xiiTelemetry::Broadcast(TransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, xiiStreamReader& ref_stream, xiiInt32 iDataBytes)
{
  if (s_ConnectionMode != xiiTelemetry::Server)
    return;

  Send(tm, uiSystemID, uiMsgID, ref_stream, iDataBytes);
}

void xiiTelemetry::Broadcast(TransmitMode tm, xiiTelemetryMessage& ref_msg)
{
  if (s_ConnectionMode != xiiTelemetry::Server)
    return;

  Send(tm, ref_msg);
}

void xiiTelemetry::SendToServer(xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const void* pData, xiiUInt32 uiDataBytes)
{
  if (s_ConnectionMode != xiiTelemetry::Client)
    return;

  Send(xiiTelemetry::Reliable, uiSystemID, uiMsgID, pData, uiDataBytes);
}

void xiiTelemetry::SendToServer(xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, xiiStreamReader& ref_stream, xiiInt32 iDataBytes)
{
  if (s_ConnectionMode != xiiTelemetry::Client)
    return;

  Send(xiiTelemetry::Reliable, uiSystemID, uiMsgID, ref_stream, iDataBytes);
}

void xiiTelemetry::SendToServer(xiiTelemetryMessage& ref_msg)
{
  if (s_ConnectionMode != xiiTelemetry::Client)
    return;

  Send(xiiTelemetry::Reliable, ref_msg);
}

void xiiTelemetry::Send(TransmitMode tm, xiiTelemetryMessage& msg)
{
  Send(tm, msg.GetSystemID(), msg.GetMessageID(), msg.GetReader(), (xiiInt32)msg.m_Storage.GetStorageSize32());
}



XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_TelemetryHelpers);
