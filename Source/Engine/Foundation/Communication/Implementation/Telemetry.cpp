#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Threading/ThreadUtils.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
#  include <enet/enet.h>
#endif

class xiiTelemetryThread;

xiiTelemetry::xiiEventTelemetry               xiiTelemetry::s_TelemetryEvents;
xiiUInt32                                     xiiTelemetry::s_uiApplicationID     = 0;
xiiUInt32                                     xiiTelemetry::s_uiServerID          = 0;
xiiUInt16                                     xiiTelemetry::s_uiPort              = 1040;
bool                                          xiiTelemetry::s_bConnectedToServer  = false;
bool                                          xiiTelemetry::s_bConnectedToClient  = false;
bool                                          xiiTelemetry::s_bAllowNetworkUpdate = true;
xiiTime                                       xiiTelemetry::s_PingToServer;
xiiString                                     xiiTelemetry::s_sServerName;
xiiString                                     xiiTelemetry::s_sServerIP;
static bool                                   g_bInitialized                 = false;
xiiTelemetry::ConnectionMode                  xiiTelemetry::s_ConnectionMode = xiiTelemetry::None;
xiiMap<xiiUInt64, xiiTelemetry::MessageQueue> xiiTelemetry::s_SystemMessages;

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
static ENetAddress g_pServerAddress;
static ENetHost*   g_pHost               = nullptr;
static ENetPeer*   g_pConnectionToServer = nullptr;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

void xiiTelemetry::UpdateServerPing()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  enet_peer_ping(g_pConnectionToServer);
  xiiTelemetry::s_PingToServer = xiiTime::Milliseconds(g_pConnectionToServer->lastRoundTripTime);
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::UpdateNetwork()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  if (!s_bAllowNetworkUpdate)
    return;

  s_bAllowNetworkUpdate = false;

  ENetEvent NetworkEvent;

  while (true)
  {
    XII_LOCK(GetTelemetryMutex());

    const xiiInt32 iStatus = enet_host_service(g_pHost, &NetworkEvent, 0);

    if (iStatus <= 0)
    {
      s_bAllowNetworkUpdate = true;
      return;
    }

    switch (NetworkEvent.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
      {
        if ((xiiTelemetry::s_ConnectionMode == xiiTelemetry::Server) && (NetworkEvent.peer->eventData != 'XIBC'))
        {
          enet_peer_disconnect(NetworkEvent.peer, 0);
          break;
        }

        if (s_ConnectionMode == Client)
        {
          char szHostIP[64] = "<unknown>";
          // char szHostName[64] = "<unknown>";

          enet_address_get_host_ip(&NetworkEvent.peer->address, szHostIP, 63);

          // Querying host IP and name can take a lot of time which can lead to timeouts
          // enet_address_get_host(&NetworkEvent.peer->address, szHostName, 63);

          xiiTelemetry::s_sServerIP = szHostIP;
          // xiiTelemetry::s_ServerName = szHostName;

          // now we are waiting for the server to send its ID
        }
        else
        {
          // got a new client, send the server ID to it
          s_bConnectedToClient = true; // we need this fake state, otherwise Broadcast will queue the message instead of sending it
          Broadcast(xiiTelemetry::Reliable, 'XIBC', 'XIID', &s_uiApplicationID, sizeof(xiiUInt32));
          s_bConnectedToClient = false;

          // then wait for its acknowledgment message
        }
      }
      break;

      case ENET_EVENT_TYPE_DISCONNECT:
      {
        if (s_ConnectionMode == Client)
        {
          s_bConnectedToServer = false;

          // First wait a bit to ensure that the Server could shut down, if this was a legitimate disconnect
          xiiThreadUtils::Sleep(xiiTime::Seconds(1));

          // Now try to reconnect. If the Server still exists, fine, connect to that.
          // If it does not exist anymore, this will connect to the next best Server that can be found.
          g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'XIBC');

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromServer;

          s_TelemetryEvents.Broadcast(e);
        }
        else
        {
          /// \todo This assumes we only connect to a single client ...
          s_bConnectedToClient = false;

          TelemetryEventData e;
          e.m_EventType = TelemetryEventData::DisconnectedFromClient;

          s_TelemetryEvents.Broadcast(e);
        }
      }
      break;

      case ENET_EVENT_TYPE_RECEIVE:
      {
        const xiiUInt32 uiSystemID = *((xiiUInt32*)&NetworkEvent.packet->data[0]);
        const xiiUInt32 uiMsgID    = *((xiiUInt32*)&NetworkEvent.packet->data[4]);
        const xiiUInt8* pData      = &NetworkEvent.packet->data[8];

        if (uiSystemID == 'XIBC')
        {
          switch (uiMsgID)
          {
            case 'XIID':
            {
              s_uiServerID = *((xiiUInt32*)pData);

              // connection to server is finalized
              s_bConnectedToServer = true;

              // acknowledge that the ID has been received
              SendToServer('XIBC', 'AKID', nullptr, 0);

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToServer;

              s_TelemetryEvents.Broadcast(e);

              FlushOutgoingQueues();
            }
            break;
            case 'AKID':
            {
              // the client received the server ID -> the connection has been established properly

              /// \todo This assumes we only connect to a single client ...
              s_bConnectedToClient = true;

              // go tell the others about it
              TelemetryEventData e;
              e.m_EventType = TelemetryEventData::ConnectedToClient;

              s_TelemetryEvents.Broadcast(e);

              SendServerName();
              FlushOutgoingQueues();
            }
            break;

            case 'NAME':
            {
              s_sServerName = reinterpret_cast<const char*>(pData);
            }
            break;
          }
        }
        else
        {
          MessageQueue& Queue = s_SystemMessages[uiSystemID];

          if (Queue.m_bAcceptMessages)
          {
            Queue.m_IncomingQueue.PushBack();
            xiiTelemetryMessage& Msg = Queue.m_IncomingQueue.PeekBack();

            Msg.SetMessageID(uiSystemID, uiMsgID);

            XII_ASSERT_DEV((xiiUInt32)NetworkEvent.packet->dataLength >= 8, "Message Length Invalid: {0}", (xiiUInt32)NetworkEvent.packet->dataLength);

            Msg.GetWriter().WriteBytes(pData, NetworkEvent.packet->dataLength - 8).IgnoreResult();
          }
        }

        enet_packet_destroy(NetworkEvent.packet);
      }
      break;

      default:
        break;
    }
  }

  s_bAllowNetworkUpdate = true;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::SetServerName(xiiStringView sName)
{
  if (s_ConnectionMode == ConnectionMode::Client)
    return;

  if (s_sServerName == sName)
    return;

  s_sServerName = sName;

  SendServerName();
}

void xiiTelemetry::SendServerName()
{
  if (!IsConnectedToOther())
    return;

  char data[48];
  xiiStringUtils::Copy(data, XII_ARRAY_SIZE(data), s_sServerName.GetData());

  Broadcast(xiiTelemetry::Reliable, 'XIBC', 'NAME', data, XII_ARRAY_SIZE(data));
}

xiiResult xiiTelemetry::RetrieveMessage(xiiUInt32 uiSystemID, xiiTelemetryMessage& out_message)
{
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return XII_FAILURE;

  XII_LOCK(GetTelemetryMutex());

  // check again while inside the lock
  if (s_SystemMessages[uiSystemID].m_IncomingQueue.IsEmpty())
    return XII_FAILURE;

  out_message = s_SystemMessages[uiSystemID].m_IncomingQueue.PeekFront();
  s_SystemMessages[uiSystemID].m_IncomingQueue.PopFront();

  return XII_SUCCESS;
}

void xiiTelemetry::InitializeAsServer()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pServerAddress.host = ENET_HOST_ANY;
  g_pServerAddress.port = s_uiPort;

  g_pHost = enet_host_create(&g_pServerAddress, 32, 2, 0, 0);
#else
  xiiLog::SeriousWarning("Enet is not compiled into this build, xiiTelemetry::InitializeAsServer() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

xiiResult xiiTelemetry::InitializeAsClient(xiiStringView sConnectTo0)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  g_pHost = enet_host_create(nullptr, 1, 2, 0, 0);

  xiiStringBuilder sConnectTo = sConnectTo0;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, xiiStringUtils::GetStringElementCount(szColon));

    xiiStringBuilder sPort = szColon + 1;
    s_uiPort              = static_cast<xiiUInt16>(atoi(sPort.GetData()));
  }

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
    enet_address_set_host(&g_pServerAddress, "localhost");
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    xiiHybridArray<xiiString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return XII_FAILURE;

    const xiiUInt32 ip1 = atoi(IP[0].GetData()) & 0xFF;
    const xiiUInt32 ip2 = atoi(IP[1].GetData()) & 0xFF;
    const xiiUInt32 ip3 = atoi(IP[2].GetData()) & 0xFF;
    const xiiUInt32 ip4 = atoi(IP[3].GetData()) & 0xFF;

    const xiiUInt32 uiIP = (ip1 | ip2 << 8 | ip3 << 16 | ip4 << 24);

    g_pServerAddress.host = uiIP;
  }
  else
    enet_address_set_host(&g_pServerAddress, sConnectTo.GetData());

  g_pServerAddress.port = s_uiPort;

  g_pConnectionToServer = nullptr;
  g_pConnectionToServer = enet_host_connect(g_pHost, &g_pServerAddress, 2, 'XIBC');

  if (g_pConnectionToServer)
    return XII_SUCCESS;
#else
  xiiLog::SeriousWarning("Enet is not compiled into this build, xiiTelemetry::InitializeAsClient() will be ignored.");
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT

  return XII_FAILURE;
}

xiiResult xiiTelemetry::OpenConnection(ConnectionMode Mode, xiiStringView sConnectTo)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  CloseConnection();

  if (!g_bInitialized)
  {
    if (enet_initialize() != 0)
    {
      xiiLog::Error("Enet could not be initialized.");
      return XII_FAILURE;
    }

    g_bInitialized = true;
  }

  s_uiApplicationID = (xiiUInt32)xiiTime::Now().GetSeconds();

  switch (Mode)
  {
    case xiiTelemetry::Server:
      InitializeAsServer();
      break;
    case xiiTelemetry::Client:
      if (InitializeAsClient(sConnectTo) == XII_FAILURE)
      {
        CloseConnection();
        return XII_FAILURE;
      }
      break;
    default:
      break;
  }

  s_ConnectionMode = Mode;

  xiiTelemetry::UpdateNetwork();

  StartTelemetryThread();

  return XII_SUCCESS;
#else
  xiiLog::SeriousWarning("Enet is not compiled into this build, xiiTelemetry::OpenConnection() will be ignored.");
  return XII_FAILURE;
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::Transmit(TransmitMode tm, const void* pData, xiiUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  XII_LOCK(GetTelemetryMutex());

  ENetPacket* pPacket = enet_packet_create(pData, uiDataBytes, (tm == Reliable) ? ENET_PACKET_FLAG_RELIABLE : 0);
  enet_host_broadcast(g_pHost, 0, pPacket);

  // make sure the message is processed immediately
  xiiTelemetry::UpdateNetwork();
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::Send(TransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const void* pData, xiiUInt32 uiDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
    QueueOutgoingMessage(tm, uiSystemID, uiMsgID, pData, uiDataBytes);
  else
  {
    // when we do have a connection, just send the message out

    xiiHybridArray<xiiUInt8, 64> TempData;
    TempData.SetCountUninitialized(8 + uiDataBytes);
    *((xiiUInt32*)&TempData[0]) = uiSystemID;
    *((xiiUInt32*)&TempData[4]) = uiMsgID;

    if (pData && uiDataBytes > 0)
      xiiMemoryUtils::Copy((xiiUInt8*)&TempData[8], (xiiUInt8*)pData, uiDataBytes);

    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::Send(TransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, xiiStreamReader& Stream, xiiInt32 iDataBytes)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  if (!g_pHost)
    return;

  const xiiUInt32 uiStackSize = 1024;

  xiiHybridArray<xiiUInt8, uiStackSize + 8> TempData;
  TempData.SetCountUninitialized(8);
  *((xiiUInt32*)&TempData[0]) = uiSystemID;
  *((xiiUInt32*)&TempData[4]) = uiMsgID;

  // if we don't know how much to take out of the stream, read the data piece by piece from the input stream
  if (iDataBytes < 0)
  {
    while (true)
    {
      const xiiUInt32 uiOffset = TempData.GetCount();
      TempData.SetCountUninitialized(uiOffset + uiStackSize); // no allocation the first time

      const xiiUInt32 uiRead = static_cast<xiiUInt32>(Stream.ReadBytes(&TempData[uiOffset], uiStackSize));

      if (uiRead < uiStackSize)
      {
        // resize the array down to its actual size
        TempData.SetCountUninitialized(uiOffset + uiRead);
        break;
      }
    }
  }
  else
  {
    TempData.SetCountUninitialized(8 + iDataBytes);

    if (iDataBytes > 0)
      Stream.ReadBytes(&TempData[8], iDataBytes);
  }

  // in case we have no connection to a peer, queue the message
  if (!IsConnectedToOther())
  {
    if (TempData.GetCount() > 8)
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, &TempData[8], TempData.GetCount() - 8);
    else
      QueueOutgoingMessage(tm, uiSystemID, uiMsgID, nullptr, 0);
  }
  else
  {
    // when we do have a connection, just send the message out
    Transmit(tm, &TempData[0], TempData.GetCount());
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}

void xiiTelemetry::CloseConnection()
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  s_bConnectedToServer  = false;
  s_bConnectedToClient  = false;
  s_ConnectionMode      = None;
  s_uiServerID          = 0;
  g_pConnectionToServer = nullptr;

  StopTelemetryThread();

  // prevent other threads from interfering
  XII_LOCK(GetTelemetryMutex());

  UpdateNetwork();
  xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));

  if (g_pHost)
  {
    // send all peers that we are disconnecting
    for (xiiUInt32 i = (xiiUInt32)g_pHost->connectedPeers; i > 0; --i)
      enet_peer_disconnect(&g_pHost->peers[i - 1], 0);

    // process the network messages (e.g. send the disconnect messages)
    UpdateNetwork();
    xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));
  }

  // finally close the network connection
  if (g_pHost)
  {
    enet_host_destroy(g_pHost);
    g_pHost = nullptr;
  }

  if (g_bInitialized)
  {
    enet_deinitialize();
    g_bInitialized = false;
  }

  // if there are any queued messages, throw them away
  for (auto it = s_SystemMessages.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_IncomingQueue.Clear();
    it.Value().m_OutgoingQueue.Clear();
  }
#endif // BUILDSYSTEM_ENABLE_ENET_SUPPORT
}


XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Telemetry);
