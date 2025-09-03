#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/IpcProcessMessageProtocol.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>

xiiResult xiiEditorProcessCommunicationChannel::StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const xiiRTTI* pFirstAllowedMessageType, xiiUInt32 uiMemSize)
{
  XII_LOG_BLOCK("xiiProcessCommunicationChannel::StartClientProcess");

  XII_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");
  XII_ASSERT_DEV(m_pClientProcess == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  static xiiUInt64 uiUniqueHash = 0;
  xiiOsProcessID   PID          = xiiProcess::GetCurrentProcessID();
  uiUniqueHash                  = xiiHashingUtils::xxHash64(&PID, sizeof(PID), uiUniqueHash);
  xiiTime time                  = xiiTime::Now();
  uiUniqueHash                  = xiiHashingUtils::xxHash64(&time, sizeof(time), uiUniqueHash);
  xiiStringBuilder sMemName;
  sMemName.SetFormat("{0}", xiiArgU(uiUniqueHash, 16, false, 16, true));
  ++uiUniqueHash;

  if (bRemote)
  {
    m_pChannel = xiiIpcChannel::CreateNetworkChannel("172.16.80.3:1050", xiiIpcChannel::Mode::Client);
  }
  else
  {
    m_pChannel = xiiIpcChannel::CreatePipeChannel(sMemName, xiiIpcChannel::Mode::Server);
  }
  m_pProtocol = XII_DEFAULT_NEW(xiiIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(xiiMakeDelegate(&xiiProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();
  for (xiiUInt32 i = 0; i < 100; ++i)
  {
    if (m_pChannel->GetConnectionState() == xiiIpcChannel::ConnectionState::Connecting)
      break;

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  if (m_pChannel->GetConnectionState() != xiiIpcChannel::ConnectionState::Connecting)
  {
    xiiLog::Error("Failed to start IPC server");
    CloseConnection();
    return XII_FAILURE;
  }

  xiiStringBuilder sPath = szProcess;

  if (!sPath.IsAbsolutePath())
  {
    sPath = xiiOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }

  sPath.MakeCleanPath();

  xiiStringBuilder sPID;
  xiiConversionUtils::ToString((xiiUInt64)QCoreApplication::applicationPid(), sPID);

  QStringList arguments;
  arguments << "-IPC";
  arguments << QLatin1String(sMemName.GetData());
  arguments << "-PID";
  arguments << sPID.GetData();
  arguments.append(args);

  m_pClientProcess = new QProcess();

  if (!bRemote)
  {
    m_pClientProcess->start(QString::fromUtf8(sPath.GetData()), arguments, QIODevice::OpenModeFlag::NotOpen);

    if (!m_pClientProcess->waitForStarted())
    {
      delete m_pClientProcess;
      m_pClientProcess = nullptr;

      m_pProtocol.Clear();
      m_pChannel.Clear();

      xiiLog::Error("Failed to start process '{0}'", sPath);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

bool xiiEditorProcessCommunicationChannel::IsClientAlive() const
{
  if (m_pClientProcess == nullptr)
    return false;

  bool bRunning = m_pClientProcess->state() != QProcess::NotRunning;
  bool bNoError = m_pClientProcess->error() == QProcess::UnknownError;

  return bRunning && bNoError;
}

void xiiEditorProcessCommunicationChannel::CloseConnection()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(xiiMakeDelegate(&xiiProcessCommunicationChannel::MessageFunc, this));
    m_pProtocol.Clear();
  }
  m_pChannel.Clear();

  if (m_pClientProcess)
  {
    m_pClientProcess->close();
    delete m_pClientProcess;
    m_pClientProcess = nullptr;
  }
}

xiiString xiiEditorProcessCommunicationChannel::GetStdoutContents()
{
  if (m_pClientProcess)
  {
    QByteArray output = m_pClientProcess->readAllStandardOutput();
    return xiiString(xiiStringView((const char*)output.data(), output.size()));
  }
  return xiiString();
}

//////////////////////////////////////////////////////////////////////////

xiiResult xiiEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  XII_LOG_BLOCK("xiiEditorProcessRemoteCommunicationChannel::ConnectToServer");

  XII_ASSERT_DEV(m_pChannel == nullptr, "ProcessCommunication object already in use");

  m_pFirstAllowedMessageType = nullptr;

  m_pChannel  = xiiIpcChannel::CreateNetworkChannel(szAddress, xiiIpcChannel::Mode::Client);
  m_pProtocol = XII_DEFAULT_NEW(xiiIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(xiiMakeDelegate(&xiiProcessCommunicationChannel::MessageFunc, this));
  m_pChannel->Connect();

  return XII_SUCCESS;
}

bool xiiEditorProcessRemoteCommunicationChannel::IsConnected() const
{
  return m_pChannel->IsConnected();
}

void xiiEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  if (m_pProtocol)
  {
    m_pProtocol->m_MessageEvent.RemoveEventHandler(xiiMakeDelegate(&xiiProcessCommunicationChannel::MessageFunc, this));
    m_pProtocol.Clear();
  }
  m_pChannel.Clear();
}

void xiiEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_pChannel && !m_pChannel->IsConnected())
  {
    m_pChannel->Connect();
  }
}
