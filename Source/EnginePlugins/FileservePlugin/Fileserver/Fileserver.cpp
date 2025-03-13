#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Client/FileserveClient.h>
#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/RemoteInterfaceEnet.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/CommandLineUtils.h>

XII_IMPLEMENT_SINGLETON(xiiFileserver);

xiiFileserver::xiiFileserver() :
  m_SingletonRegistrar(this)
{
  // once a server exists, the client should stay inactive
  xiiFileserveClient::DisabledFileserveClient();

  // check whether the fileserve port was reconfigured through the command line
  m_uiPort = static_cast<xiiUInt16>(xiiCommandLineUtils::GetGlobalInstance()->GetIntOption("-fs_port", m_uiPort));
}

void xiiFileserver::StartServer()
{
  if (m_pNetwork)
    return;

  xiiStringBuilder tmp;

  m_pNetwork = xiiRemoteInterfaceEnet::Make();
  m_pNetwork->StartServer('XIFS', xiiConversionUtils::ToString(m_uiPort, tmp), false).IgnoreResult();
  m_pNetwork->SetMessageHandler('FSRV', xiiMakeDelegate(&xiiFileserver::NetworkMsgHandler, this));
  m_pNetwork->SetUnhandledMessageHandler(xiiMakeDelegate(&xiiFileserver::UnknownNetworkMsgHandler, this));
  m_pNetwork->m_RemoteEvents.AddEventHandler(xiiMakeDelegate(&xiiFileserver::NetworkEventHandler, this));

  xiiFileserverEvent e;
  e.m_Type = xiiFileserverEvent::Type::ServerStarted;
  m_Events.Broadcast(e);
}

void xiiFileserver::StopServer()
{
  if (!m_pNetwork)
    return;

  m_pNetwork->ShutdownConnection();
  m_pNetwork.Clear();

  xiiFileserverEvent e;
  e.m_Type = xiiFileserverEvent::Type::ServerStopped;
  m_Events.Broadcast(e);
}

bool xiiFileserver::UpdateServer()
{
  if (!m_pNetwork)
    return false;

  m_pNetwork->UpdateRemoteInterface();
  return m_pNetwork->ExecuteAllMessageHandlers() > 0;
}

bool xiiFileserver::IsServerRunning() const
{
  return m_pNetwork != nullptr;
}

void xiiFileserver::SetPort(xiiUInt16 uiPort)
{
  XII_ASSERT_DEV(m_pNetwork == nullptr, "The port cannot be changed after the server was started");
  m_uiPort = uiPort;
}


void xiiFileserver::BroadcastReloadResourcesCommand()
{
  if (!IsServerRunning())
    return;

  m_pNetwork->Send('FSRV', 'RLDR');
}

void xiiFileserver::NetworkMsgHandler(xiiRemoteMessage& msg)
{
  auto& client = DetermineClient(msg);

  if (msg.GetMessageID() == 'HELO')
    return;

  if (msg.GetMessageID() == 'RUTR')
  {
    // 'are you there' is used to check whether a certain address is a proper Fileserver
    m_pNetwork->Send('FSRV', ' YES');

    xiiFileserverEvent e;
    e.m_Type = xiiFileserverEvent::Type::AreYouThereRequest;
    m_Events.Broadcast(e);
    return;
  }

  if (msg.GetMessageID() == 'READ')
  {
    HandleFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLH')
  {
    HandleUploadFileHeader(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLD')
  {
    HandleUploadFileTransfer(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UPLF')
  {
    HandleUploadFileFinished(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'DELF')
  {
    HandleDeleteFileRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == ' MNT')
  {
    HandleMountRequest(client, msg);
    return;
  }

  if (msg.GetMessageID() == 'UMNT')
  {
    HandleUnmountRequest(client, msg);
    return;
  }

  xiiLog::Error("Unknown FSRV message: '{0}' - {1} bytes", msg.GetMessageID(), msg.GetMessageData().GetCount());
}

void xiiFileserver::UnknownNetworkMsgHandler(xiiRemoteMessage& msg)
{
  auto it = m_CustomMessageHandlers.Find(msg.GetSystemID());
  if (!it.IsValid() || !it.Value().IsValid())
    return;

  auto& client = DetermineClient(msg);

  it.Value()(client, msg, *m_pNetwork, xiiMakeDelegate(&xiiFileserver::LogCustomActivity, this));
}

void xiiFileserver::SetCustomMessageHandler(xiiUInt32 uiSystemID, ClientMessageHandler handler)
{
  m_CustomMessageHandlers[uiSystemID] = handler;
}

void xiiFileserver::NetworkEventHandler(const xiiRemoteEvent& e)
{
  switch (e.m_Type)
  {
    case xiiRemoteEvent::DisconnectedFromClient:
    {
      if (m_Clients.Contains(e.m_uiOtherAppID))
      {
        xiiFileserverEvent se;
        se.m_Type       = xiiFileserverEvent::Type::ClientDisconnected;
        se.m_uiClientID = e.m_uiOtherAppID;

        m_Events.Broadcast(se);

        m_Clients[e.m_uiOtherAppID].m_bLostConnection = true;
      }
    }
    break;

    default:
      break;
  }
}

xiiFileserveClientContext& xiiFileserver::DetermineClient(xiiRemoteMessage& msg)
{
  xiiFileserveClientContext& client = m_Clients[msg.GetApplicationID()];

  if (client.m_uiApplicationID != msg.GetApplicationID())
  {
    client.m_uiApplicationID = msg.GetApplicationID();

    xiiFileserverEvent e;
    e.m_Type       = xiiFileserverEvent::Type::ClientConnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }
  else if (client.m_bLostConnection)
  {
    client.m_bLostConnection = false;

    xiiFileserverEvent e;
    e.m_Type       = xiiFileserverEvent::Type::ClientReconnected;
    e.m_uiClientID = client.m_uiApplicationID;
    m_Events.Broadcast(e);
  }

  return client;
}

void xiiFileserver::HandleMountRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiStringBuilder sDataDir, sRootName, sMountPoint, sRedir;
  xiiUInt16        uiDataDirID = 0xffff;

  msg.GetReader() >> sDataDir;
  msg.GetReader() >> sRootName;
  msg.GetReader() >> sMountPoint;
  msg.GetReader() >> uiDataDirID;

  XII_ASSERT_DEV(uiDataDirID >= client.m_MountedDataDirs.GetCount(), "Data dir ID should be larger than previous IDs");

  client.m_MountedDataDirs.SetCount(xiiMath::Max<xiiUInt32>(uiDataDirID + 1, client.m_MountedDataDirs.GetCount()));
  auto& dir           = client.m_MountedDataDirs[uiDataDirID];
  dir.m_sPathOnClient = sDataDir;
  dir.m_sRootName     = sRootName;
  dir.m_sMountPoint   = sMountPoint;

  xiiFileserverEvent e;

  if (xiiFileSystem::ResolveSpecialDirectory(sDataDir, sRedir).Succeeded())
  {
    dir.m_bMounted      = true;
    dir.m_sPathOnServer = sRedir;
    e.m_Type            = xiiFileserverEvent::Type::MountDataDir;
  }
  else
  {
    dir.m_bMounted = false;
    e.m_Type       = xiiFileserverEvent::Type::MountDataDirFailed;
  }

  e.m_uiClientID       = client.m_uiApplicationID;
  e.m_szName           = sRootName;
  e.m_szPath           = sDataDir;
  e.m_szRedirectedPath = sRedir;
  m_Events.Broadcast(e);
}


void xiiFileserver::HandleUnmountRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  XII_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  auto& dir      = client.m_MountedDataDirs[uiDataDirID];
  dir.m_bMounted = false;

  xiiFileserverEvent e;
  e.m_Type       = xiiFileserverEvent::Type::UnmountDataDir;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath     = dir.m_sPathOnClient;
  e.m_szName     = dir.m_sRootName;
  m_Events.Broadcast(e);
}

void xiiFileserver::HandleFileRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID       = 0;
  bool      bForceThisDataDir = false;

  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> bForceThisDataDir;

  xiiStringBuilder sRequestedFile;
  msg.GetReader() >> sRequestedFile;

  xiiUuid downloadGuid;
  msg.GetReader() >> downloadGuid;

  xiiFileserveClientContext::FileStatus status;
  msg.GetReader() >> status.m_iTimestamp;
  msg.GetReader() >> status.m_uiHash;

  xiiFileserverEvent e;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = sRequestedFile;
  e.m_uiSentTotal = 0;

  const xiiFileserveFileState filestate = client.GetFileStatus(uiDataDirID, sRequestedFile, status, m_SendToClient, bForceThisDataDir);

  {
    e.m_Type        = xiiFileserverEvent::Type::FileDownloadRequest;
    e.m_uiSizeTotal = m_SendToClient.GetCount();
    e.m_FileState   = filestate;
    m_Events.Broadcast(e);
  }

  if (filestate == xiiFileserveFileState::Different)
  {
    xiiUInt32       uiNextByte = 0;
    const xiiUInt32 uiFileSize = m_SendToClient.GetCount();

    // send the file over in multiple packages of 1KB each
    // send at least one package, even for empty files
    do
    {
      const xiiUInt16 uiChunkSize = (xiiUInt16)xiiMath::Min<xiiUInt32>(1024, m_SendToClient.GetCount() - uiNextByte);

      xiiRemoteMessage ret;
      ret.GetWriter() << downloadGuid;
      ret.GetWriter() << uiChunkSize;
      ret.GetWriter() << uiFileSize;

      if (!m_SendToClient.IsEmpty())
        ret.GetWriter().WriteBytes(&m_SendToClient[uiNextByte], uiChunkSize).IgnoreResult();

      ret.SetMessageID('FSRV', 'DWNL');
      m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, ret);

      uiNextByte += uiChunkSize;

      // reuse previous values
      {
        e.m_Type        = xiiFileserverEvent::Type::FileDownloading;
        e.m_uiSentTotal = uiNextByte;
        m_Events.Broadcast(e);
      }
    } while (uiNextByte < m_SendToClient.GetCount());
  }

  // final answer to client
  {
    xiiRemoteMessage ret('FSRV', 'DWNF');
    ret.GetWriter() << downloadGuid;
    ret.GetWriter() << (xiiInt8)filestate;
    ret.GetWriter() << status.m_iTimestamp;
    ret.GetWriter() << status.m_uiHash;
    ret.GetWriter() << uiDataDirID;

    m_pNetwork->Send(xiiRemoteTransmitMode::Reliable, ret);
  }

  // reuse previous values
  {
    e.m_Type = xiiFileserverEvent::Type::FileDownloadFinished;
    m_Events.Broadcast(e);
  }
}

void xiiFileserver::HandleDeleteFileRequest(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID = 0xffff;
  msg.GetReader() >> uiDataDirID;

  xiiStringBuilder sFile;
  msg.GetReader() >> sFile;

  XII_ASSERT_DEV(uiDataDirID < client.m_MountedDataDirs.GetCount(), "Invalid data dir ID to unmount");

  xiiFileserverEvent e;
  e.m_Type       = xiiFileserverEvent::Type::FileDeleteRequest;
  e.m_uiClientID = client.m_uiApplicationID;
  e.m_szPath     = sFile;
  m_Events.Broadcast(e);

  const auto& dd = client.m_MountedDataDirs[uiDataDirID];

  xiiStringBuilder sAbsPath;
  sAbsPath = dd.m_sPathOnServer;
  sAbsPath.AppendPath(sFile);

  xiiOSFile::DeleteFile(sAbsPath).IgnoreResult();
}

void xiiFileserver::HandleUploadFileHeader(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUInt16 uiDataDirID = 0;

  msg.GetReader() >> m_FileUploadGuid;
  msg.GetReader() >> m_uiFileUploadSize;
  msg.GetReader() >> uiDataDirID;
  msg.GetReader() >> m_sCurFileUpload;

  m_SentFromClient.Clear();
  m_SentFromClient.Reserve(m_uiFileUploadSize);

  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileUploadRequest;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = m_sCurFileUpload;
  e.m_uiSentTotal = 0;
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void xiiFileserver::HandleUploadFileTransfer(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  xiiUInt16 uiChunkSize = 0;
  msg.GetReader() >> uiChunkSize;

  const xiiUInt32 uiStartPos = m_SentFromClient.GetCount();
  m_SentFromClient.SetCountUninitialized(uiStartPos + uiChunkSize);
  msg.GetReader().ReadBytes(&m_SentFromClient[uiStartPos], uiChunkSize);

  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileUploading;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = m_sCurFileUpload;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_uiFileUploadSize;

  m_Events.Broadcast(e);
}

void xiiFileserver::HandleUploadFileFinished(xiiFileserveClientContext& client, xiiRemoteMessage& msg)
{
  xiiUuid transferGuid;
  msg.GetReader() >> transferGuid;

  if (transferGuid != m_FileUploadGuid)
    return;

  xiiUInt16 uiDataDirID = 0;
  msg.GetReader() >> uiDataDirID;

  xiiStringBuilder sFile;
  msg.GetReader() >> sFile;

  xiiStringBuilder sOutputFile;
  sOutputFile = client.m_MountedDataDirs[uiDataDirID].m_sPathOnServer;
  sOutputFile.AppendPath(sFile);

  {
    xiiOSFile file;
    if (file.Open(sOutputFile, xiiFileOpenMode::Write).Failed())
    {
      xiiLog::Error("Could not write uploaded file to '{0}'", sOutputFile);
      return;
    }

    if (!m_SentFromClient.IsEmpty())
    {
      file.Write(m_SentFromClient.GetData(), m_SentFromClient.GetCount()).IgnoreResult();
    }
  }

  xiiFileserverEvent e;
  e.m_Type        = xiiFileserverEvent::Type::FileUploadFinished;
  e.m_uiClientID  = client.m_uiApplicationID;
  e.m_szPath      = sFile;
  e.m_uiSentTotal = m_SentFromClient.GetCount();
  e.m_uiSizeTotal = m_SentFromClient.GetCount();

  m_Events.Broadcast(e);

  // send a response when all data has been transmitted
  // this ensures the client side updates the network until all data has been fully transmitted
  m_pNetwork->Send('FSRV', 'UACK');
}

void xiiFileserver::LogCustomActivity(const char* szText)
{
  xiiFileserverEvent e;
  e.m_szName = szText;
  e.m_Type   = xiiFileserverEvent::Type::LogCustomActivity;
  m_Events.Broadcast(e);
}

xiiResult xiiFileserver::SendConnectionInfo(const char* szClientAddress, xiiUInt16 uiMyPort, const xiiArrayPtr<xiiStringBuilder>& myIPs, xiiTime timeout)
{
  xiiStringBuilder sAddress = szClientAddress;
  sAddress.Append(":2042"); // hard-coded port

  xiiUniquePtr<xiiRemoteInterfaceEnet> network = xiiRemoteInterfaceEnet::Make();
  XII_SUCCEED_OR_RETURN(network->ConnectToServer('XIIP', sAddress, false));

  if (network->WaitForConnectionToServer(timeout).Failed())
  {
    network->ShutdownConnection();
    return XII_FAILURE;
  }

  const xiiUInt8 uiCount = static_cast<xiiUInt8>(myIPs.GetCount());

  xiiRemoteMessage msg('FSRV', 'MYIP');
  msg.GetWriter() << uiMyPort;
  msg.GetWriter() << uiCount;

  for (const auto& info : myIPs)
  {
    msg.GetWriter() << info;
  }

  network->Send(xiiRemoteTransmitMode::Reliable, msg);

  // make sure the message is out, before we shut down
  for (xiiUInt32 i = 0; i < 10; ++i)
  {
    network->UpdateRemoteInterface();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
  }

  network->ShutdownConnection();
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(FileservePlugin, FileservePlugin_Fileserver_Fileserver);
