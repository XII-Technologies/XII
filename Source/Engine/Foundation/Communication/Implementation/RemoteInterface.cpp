#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Utilities/ConversionUtils.h>

xiiRemoteInterface::~xiiRemoteInterface()
{
  // unfortunately we cannot do that ourselves here, because ShutdownConnection() calls virtual functions
  // and this object is already partially destructed here (derived class is already shut down)
  XII_ASSERT_DEV(m_RemoteMode == xiiRemoteMode::None, "xiiRemoteInterface::ShutdownConnection() has to be called before destroying the interface");
}

xiiResult xiiRemoteInterface::CreateConnection(xiiUInt32 uiConnectionToken, xiiRemoteMode mode, xiiStringView sServerAddress, bool bStartUpdateThread)
{
  xiiUInt32 uiPrevID = m_uiApplicationID;
  ShutdownConnection();
  m_uiApplicationID = uiPrevID;

  XII_LOCK(GetMutex());

  m_uiConnectionToken = uiConnectionToken;
  m_sServerAddress    = sServerAddress;

  if (m_uiApplicationID == 0)
  {
    // create a 'unique' ID to identify this application
    m_uiApplicationID = (xiiUInt32)xiiTime::Now().GetSeconds();
  }

  if (InternalCreateConnection(mode, sServerAddress).Failed())
  {
    ShutdownConnection();
    return XII_FAILURE;
  }

  m_RemoteMode = mode;

  UpdateRemoteInterface();

  if (bStartUpdateThread)
  {
    StartUpdateThread();
  }

  return XII_SUCCESS;
}

xiiResult xiiRemoteInterface::StartServer(xiiUInt32 uiConnectionToken, xiiStringView sAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, xiiRemoteMode::Server, sAddress, bStartUpdateThread);
}

xiiResult xiiRemoteInterface::ConnectToServer(xiiUInt32 uiConnectionToken, xiiStringView sAddress, bool bStartUpdateThread /*= true*/)
{
  return CreateConnection(uiConnectionToken, xiiRemoteMode::Client, sAddress, bStartUpdateThread);
}

xiiResult xiiRemoteInterface::WaitForConnectionToServer(xiiTime timeout /*= xiiTime::MakeFromSeconds(10)*/)
{
  if (m_RemoteMode != xiiRemoteMode::Client)
    return XII_FAILURE;

  const xiiTime tStart = xiiTime::Now();

  while (true)
  {
    UpdateRemoteInterface();

    if (IsConnectedToServer())
      return XII_SUCCESS;

    if (timeout.GetSeconds() != 0)
    {
      if (xiiTime::Now() - tStart > timeout)
        return XII_FAILURE;
    }

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
}

void xiiRemoteInterface::ShutdownConnection()
{
  StopUpdateThread();

  XII_LOCK(GetMutex());

  if (m_RemoteMode != xiiRemoteMode::None)
  {
    InternalShutdownConnection();

    m_RemoteMode        = xiiRemoteMode::None;
    m_uiApplicationID   = 0;
    m_uiConnectionToken = 0;
  }
}

void xiiRemoteInterface::UpdatePingToServer()
{
  if (m_RemoteMode == xiiRemoteMode::Server)
  {
    XII_LOCK(GetMutex());
    m_PingToServer = InternalGetPingToServer();
  }
}

void xiiRemoteInterface::UpdateRemoteInterface()
{
  XII_LOCK(GetMutex());

  InternalUpdateRemoteInterface();
}

xiiResult xiiRemoteInterface::Transmit(xiiRemoteTransmitMode tm, const xiiArrayPtr<const xiiUInt8>& data)
{
  if (m_RemoteMode == xiiRemoteMode::None)
    return XII_FAILURE;

  XII_LOCK(GetMutex());

  if (InternalTransmit(tm, data).Failed())
    return XII_FAILURE;

  // make sure the message is processed immediately
  UpdateRemoteInterface();

  return XII_SUCCESS;
}


void xiiRemoteInterface::Send(xiiUInt32 uiSystemID, xiiUInt32 uiMsgID)
{
  Send(xiiRemoteTransmitMode::Reliable, uiSystemID, uiMsgID, xiiArrayPtr<const xiiUInt8>());
}

void xiiRemoteInterface::Send(xiiRemoteTransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const xiiArrayPtr<const xiiUInt8>& data)
{
  if (m_RemoteMode == xiiRemoteMode::None)
    return;

  // if (!IsConnectedToOther())
  //  return;

  m_TempSendBuffer.SetCountUninitialized(12 + data.GetCount());
  *((xiiUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((xiiUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((xiiUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!data.IsEmpty())
  {
    xiiUInt8* pCopyDst = &m_TempSendBuffer[12];
    xiiMemoryUtils::Copy(pCopyDst, data.GetPtr(), data.GetCount());
  }

  Transmit(tm, m_TempSendBuffer).IgnoreResult();
}

void xiiRemoteInterface::Send(xiiRemoteTransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const void* pData /*= nullptr*/, xiiUInt32 uiDataBytes /*= 0*/)
{
  Send(tm, uiSystemID, uiMsgID, xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(pData), uiDataBytes));
}

void xiiRemoteInterface::Send(xiiRemoteTransmitMode tm, xiiRemoteMessage& ref_msg)
{
  Send(tm, ref_msg.GetSystemID(), ref_msg.GetMessageID(), ref_msg.m_Storage);
}

void xiiRemoteInterface::Send(xiiRemoteTransmitMode tm, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const xiiContiguousMemoryStreamStorage& data)
{
  if (m_RemoteMode == xiiRemoteMode::None)
    return;

  // if (!IsConnectedToOther())
  //  return;

  xiiArrayPtr<const xiiUInt8> range = {data.GetData(), data.GetStorageSize32()};

  m_TempSendBuffer.SetCountUninitialized(12 + range.GetCount());
  *((xiiUInt32*)&m_TempSendBuffer[0]) = m_uiApplicationID;
  *((xiiUInt32*)&m_TempSendBuffer[4]) = uiSystemID;
  *((xiiUInt32*)&m_TempSendBuffer[8]) = uiMsgID;

  if (!range.IsEmpty())
  {
    xiiUInt8* pCopyDst = &m_TempSendBuffer[12];
    xiiMemoryUtils::Copy(pCopyDst, range.GetPtr(), range.GetCount());
  }

  Transmit(tm, m_TempSendBuffer).IgnoreResult();
}

void xiiRemoteInterface::SetMessageHandler(xiiUInt32 uiSystemID, xiiRemoteMessageHandler messageHandler)
{
  m_MessageQueues[uiSystemID].m_MessageHandler = messageHandler;
}

void xiiRemoteInterface::SetUnhandledMessageHandler(xiiRemoteMessageHandler messageHandler)
{
  m_UnhandledMessageHandler = messageHandler;
}

xiiUInt32 xiiRemoteInterface::ExecuteMessageHandlers(xiiUInt32 uiSystem)
{
  XII_LOCK(m_Mutex);

  return ExecuteMessageHandlersForQueue(m_MessageQueues[uiSystem]);
}

xiiUInt32 xiiRemoteInterface::ExecuteAllMessageHandlers()
{
  XII_LOCK(m_Mutex);

  xiiUInt32 ret = 0;
  for (auto it = m_MessageQueues.GetIterator(); it.IsValid(); ++it)
  {
    ret += ExecuteMessageHandlersForQueue(it.Value());
  }

  return ret;
}

xiiUInt32 xiiRemoteInterface::ExecuteMessageHandlersForQueue(xiiRemoteMessageQueue& queue)
{
  queue.m_MessageQueueIn.Swap(queue.m_MessageQueueOut);
  const xiiUInt32 ret = queue.m_MessageQueueOut.GetCount();

  if (queue.m_MessageHandler.IsValid())
  {
    for (auto& msg : queue.m_MessageQueueOut)
    {
      queue.m_MessageHandler(msg);
    }
  }
  else if (m_UnhandledMessageHandler.IsValid())
  {
    for (auto& msg : queue.m_MessageQueueOut)
    {
      m_UnhandledMessageHandler(msg);
    }
  }

  queue.m_MessageQueueOut.Clear();

  return ret;
}

void xiiRemoteInterface::StartUpdateThread()
{
  StopUpdateThread();

  if (m_pUpdateThread == nullptr)
  {
    XII_LOCK(m_Mutex);

    m_pUpdateThread                     = XII_DEFAULT_NEW(xiiRemoteThread);
    m_pUpdateThread->m_pRemoteInterface = this;
    m_pUpdateThread->Start();
  }
}

void xiiRemoteInterface::StopUpdateThread()
{
  if (m_pUpdateThread != nullptr)
  {
    m_pUpdateThread->m_bKeepRunning = false;
    m_pUpdateThread->Join();

    XII_LOCK(m_Mutex);
    XII_DEFAULT_DELETE(m_pUpdateThread);
  }
}


void xiiRemoteInterface::ReportConnectionToServer(xiiUInt32 uiServerID)
{
  if (m_uiConnectedToServerWithID == uiServerID)
    return;

  m_uiConnectedToServerWithID = uiServerID;

  xiiRemoteEvent e;
  e.m_Type         = xiiRemoteEvent::ConnectedToServer;
  e.m_uiOtherAppID = uiServerID;
  m_RemoteEvents.Broadcast(e);
}


void xiiRemoteInterface::ReportConnectionToClient(xiiUInt32 uiApplicationID)
{
  m_iConnectionsToClients++;

  xiiRemoteEvent e;
  e.m_Type         = xiiRemoteEvent::ConnectedToClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}

void xiiRemoteInterface::ReportDisconnectedFromServer()
{
  m_uiConnectedToServerWithID = 0;

  xiiRemoteEvent e;
  e.m_Type         = xiiRemoteEvent::DisconnectedFromServer;
  e.m_uiOtherAppID = m_uiConnectedToServerWithID;
  m_RemoteEvents.Broadcast(e);
}

void xiiRemoteInterface::ReportDisconnectedFromClient(xiiUInt32 uiApplicationID)
{
  m_iConnectionsToClients--;

  xiiRemoteEvent e;
  e.m_Type         = xiiRemoteEvent::DisconnectedFromClient;
  e.m_uiOtherAppID = uiApplicationID;
  m_RemoteEvents.Broadcast(e);
}


void xiiRemoteInterface::ReportMessage(xiiUInt32 uiApplicationID, xiiUInt32 uiSystemID, xiiUInt32 uiMsgID, const xiiArrayPtr<const xiiUInt8>& data)
{
  XII_LOCK(m_Mutex);

  auto& queue = m_MessageQueues[uiSystemID];

  // store the data for later
  auto& msg             = queue.m_MessageQueueIn.ExpandAndGetRef();
  msg.m_uiApplicationID = uiApplicationID;
  msg.SetMessageID(uiSystemID, uiMsgID);
  msg.GetWriter().WriteBytes(data.GetPtr(), data.GetCount()).IgnoreResult();
}

xiiResult xiiRemoteInterface::DetermineTargetAddress(xiiStringView sConnectTo0, xiiUInt32& out_IP, xiiUInt16& out_Port)
{
  out_IP   = 0;
  out_Port = 0;

  xiiStringBuilder sConnectTo = sConnectTo0;

  const char* szColon = sConnectTo.FindLastSubString(":");
  if (szColon != nullptr)
  {
    sConnectTo.Shrink(0, xiiStringUtils::GetStringElementCount(szColon));

    xiiStringBuilder sPort = szColon + 1;

    xiiInt32 tmp;
    if (xiiConversionUtils::StringToInt(sPort, tmp).Succeeded())
      out_Port = static_cast<xiiUInt16>(tmp);
  }

  xiiInt32 ip1 = 0;
  xiiInt32 ip2 = 0;
  xiiInt32 ip3 = 0;
  xiiInt32 ip4 = 0;

  if (sConnectTo.IsEmpty() || sConnectTo.IsEqual_NoCase("localhost"))
  {
    ip1 = 127;
    ip2 = 0;
    ip3 = 0;
    ip4 = 1;
  }
  else if (sConnectTo.FindSubString(".") != nullptr)
  {
    xiiTemporaryHybridArray<xiiString, 8> IP;
    sConnectTo.Split(false, IP, ".");

    if (IP.GetCount() != 4)
      return XII_FAILURE;

    if (xiiConversionUtils::StringToInt(IP[0], ip1).Failed())
      return XII_FAILURE;
    if (xiiConversionUtils::StringToInt(IP[1], ip2).Failed())
      return XII_FAILURE;
    if (xiiConversionUtils::StringToInt(IP[2], ip3).Failed())
      return XII_FAILURE;
    if (xiiConversionUtils::StringToInt(IP[3], ip4).Failed())
      return XII_FAILURE;
  }
  else
  {
    return XII_FAILURE;
  }

  out_IP = ((ip1 & 0xFF) | (ip2 & 0xFF) << 8 | (ip3 & 0xFF) << 16 | (ip4 & 0xFF) << 24);
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiRemoteThread::xiiRemoteThread() :
  xiiThread("xiiRemoteThread")
{
}

xiiUInt32 xiiRemoteThread::Run()
{
  xiiTime lastPing;

  while (m_bKeepRunning && m_pRemoteInterface)
  {
    m_pRemoteInterface->UpdateRemoteInterface();

    // Send a Ping every once in a while
    if (m_pRemoteInterface->GetRemoteMode() == xiiRemoteMode::Client)
    {
      xiiTime tNow = xiiTime::Now();

      if (tNow - lastPing > xiiTime::MakeFromMilliseconds(500))
      {
        lastPing = tNow;

        m_pRemoteInterface->UpdatePingToServer();
      }
    }

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }

  return 0;
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_RemoteInterface);
