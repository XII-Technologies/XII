/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Network/NetworkLocalEndpoint.h>
#include <QEventLoop>
#include <QThread>
#include <QTimer>

xiiEditorProcessCommunicationChannel::~xiiEditorProcessCommunicationChannel()
{
  CloseConnection();
}

xiiResult xiiEditorProcessCommunicationChannel::WaitForStartup(const xiiRTTI* pResponseType)
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Interactive startup waits require the GUI thread.");
  if (!HasTransport() || !m_pClientProcess)
    return XII_FAILURE;

  QEventLoop loop;
  auto*      pEditor = xiiQtEditorApp::GetSingleton();
  QObject::connect(pEditor, &xiiQtEditorApp::StartupQuitRequested, &loop, &QEventLoop::quit);
  QObject::connect(m_pClientProcess, &QProcess::finished, &loop, &QEventLoop::quit);
  QObject::connect(m_pClientProcess, &QProcess::errorOccurred, &loop, &QEventLoop::quit);

  QTimer deadline;
  deadline.setSingleShot(true);
  QObject::connect(&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
  deadline.start(pResponseType ? 60000 : 30000);

  bool                                 bReceived = false;
  xiiEvent<const Event&>::Unsubscriber subscription;
  if (pResponseType)
  {
    m_Events.AddEventHandler([&](const Event& e) {
      if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom(pResponseType))
      {
        bReceived                       = true;
        e.m_bInterruptMessageProcessing = true;
      }
    },
                             subscription);
  }

  bool bComplete = false;
  auto pump      = [&] {
    ProcessMessages();
    bComplete = pResponseType ? bReceived : IsConnected();
    if (bComplete || !IsClientAlive() || (pResponseType && !HasPendingConnection()))
      loop.quit();
  };
  QTimer network;
  QObject::connect(&network, &QTimer::timeout, &loop, pump);
  network.start(4);
  pump();
  if (!bComplete && IsClientAlive() && !pEditor->WasStartupQuitRequested() && (!pResponseType || HasPendingConnection()))
    loop.exec(QEventLoop::ExcludeUserInputEvents);

  return bComplete && !pEditor->WasStartupQuitRequested() ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiEditorProcessCommunicationChannel::StartClientProcess(const char* szProcess, const QStringList& args, xiiProcessMessageDomain domain, const xiiRTTI* pFirstAllowedMessageType)
{
  XII_ASSERT_DEV(!HasTransport() && m_pClientProcess == nullptr, "ProcessCommunication object already in use.");

  const auto id = xiiUuid::MakeUuid();
  xiiUInt64  uiLow, uiHigh;
  id.GetValues(uiLow, uiHigh);
  xiiStringBuilder sName;
  sName.SetFormat("xii-editor-{}", xiiArgU(uiLow, 16, true, 16));
  xiiNetworkLocalEndpoint endpoint;
  XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(sName, endpoint).GetResult());
  XII_SUCCEED_OR_RETURN(StartLocalListener(endpoint, domain));
  m_pFirstAllowedMessageType = pFirstAllowedMessageType;

  xiiStringBuilder sPath = szProcess;
  if (!sPath.IsAbsolutePath())
  {
    sPath = xiiOSFile::GetApplicationDirectory();
    sPath.AppendPath(szProcess);
  }
  sPath.MakeCleanPath();

  QStringList arguments;
  arguments << "-IPC" << QLatin1String(sName.GetData());
  arguments << "-PID" << QString::number(QCoreApplication::applicationPid());
  arguments.append(args);
  m_pClientProcess = new QProcess();
  m_pClientProcess->start(QString::fromUtf8(sPath.GetData()), arguments, QIODevice::NotOpen);
  if (!m_pClientProcess->waitForStarted(10000))
  {
    xiiLog::Error("Failed to start process '{}'.", sPath);
    CloseConnection();
    return XII_FAILURE;
  }
  SetExpectedProcessId(static_cast<xiiUInt32>(m_pClientProcess->processId()));
  return XII_SUCCESS;
}

bool xiiEditorProcessCommunicationChannel::IsClientAlive() const
{
  return m_pClientProcess && m_pClientProcess->state() != QProcess::NotRunning && m_pClientProcess->error() == QProcess::UnknownError;
}

void xiiEditorProcessCommunicationChannel::CloseConnection()
{
  CloseTransport();
  if (m_pClientProcess)
  {
    if (m_pClientProcess->state() != QProcess::NotRunning)
      m_pClientProcess->waitForFinished(1000);
    m_pClientProcess->close();
    delete m_pClientProcess;
    m_pClientProcess = nullptr;
  }
}

xiiString xiiEditorProcessCommunicationChannel::GetStdoutContents()
{
  if (m_pClientProcess)
  {
    const QByteArray output = m_pClientProcess->readAllStandardOutput();
    return xiiString(xiiStringView(output.constData(), static_cast<xiiUInt32>(output.size())));
  }
  return {};
}

xiiResult xiiEditorProcessRemoteCommunicationChannel::ConnectToServer(const char* szAddress)
{
  m_sAddress = szAddress;
  m_RetryAt  = xiiTime::Now() + xiiTime::MakeFromSeconds(1);
  return ConnectRemote(m_sAddress);
}

void xiiEditorProcessRemoteCommunicationChannel::CloseConnection()
{
  CloseTransport();
  m_sAddress.Clear();
}

void xiiEditorProcessRemoteCommunicationChannel::TryConnect()
{
  if (m_sAddress.IsEmpty() || IsConnected() || HasPendingConnection() || xiiTime::Now() < m_RetryAt)
    return;
  m_RetryAt = xiiTime::Now() + xiiTime::MakeFromSeconds(1);
  ConnectRemote(m_sAddress).IgnoreResult();
}
