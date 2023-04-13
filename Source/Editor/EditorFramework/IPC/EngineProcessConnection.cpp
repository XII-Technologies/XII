#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


XII_IMPLEMENT_SINGLETON(xiiEditorEngineProcessConnection);

xiiEvent<const xiiEditorEngineProcessConnection::Event&> xiiEditorEngineProcessConnection::s_Events;

xiiEditorEngineProcessConnection::xiiEditorEngineProcessConnection() :
  m_SingletonRegistrar(this)
{
  m_bProcessShouldBeRunning = false;
  m_bProcessCrashed         = false;
  m_bClientIsConfigured     = false;

  m_IPC.m_Events.AddEventHandler(xiiMakeDelegate(&xiiEditorEngineProcessConnection::HandleIPCEvent, this));
}

xiiEditorEngineProcessConnection::~xiiEditorEngineProcessConnection()
{
  m_IPC.m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiEditorEngineProcessConnection::HandleIPCEvent, this));
}

void xiiEditorEngineProcessConnection::HandleIPCEvent(const xiiProcessCommunicationChannel::Event& e)
{
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiSyncWithProcessMsgToEditor>())
  {
    const xiiSyncWithProcessMsgToEditor* msg = static_cast<const xiiSyncWithProcessMsgToEditor*>(e.m_pMessage);
    m_uiRedrawCountReceived                  = msg->m_uiRedrawCount;
  }
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineDocumentMsg>())
  {
    const xiiEditorEngineDocumentMsg* pMsg = static_cast<const xiiEditorEngineDocumentMsg*>(e.m_pMessage);

    xiiAssetDocument* pDocument = m_DocumentByGuid[pMsg->m_DocumentGuid];

    if (pDocument)
    {
      pDocument->HandleEngineMessage(pMsg);
    }
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineMsg>())
  {
    Event ee;
    ee.m_pMsg = static_cast<const xiiEditorEngineMsg*>(e.m_pMessage);
    ee.m_Type = Event::Type::ProcessMessage;

    s_Events.Broadcast(ee);
  }
}

void xiiEditorEngineProcessConnection::UIServicesTickEventHandler(const xiiQtUiServices::TickEvent& e)
{
  if (e.m_Type == xiiQtUiServices::TickEvent::Type::EndFrame)
  {
    if (!IsProcessCrashed())
    {
      xiiSyncWithProcessMsgToEngine sm;
      sm.m_uiRedrawCount = m_uiRedrawCountSent + 1;
      SendMessage(&sm);

      if (m_uiRedrawCountSent > m_uiRedrawCountReceived)
      {
        WaitForMessage(xiiGetStaticRTTI<xiiSyncWithProcessMsgToEditor>(), xiiTime::Seconds(2.0)).IgnoreResult();
      }

      ++m_uiRedrawCountSent;
    }
  }
}

xiiEditorEngineConnection* xiiEditorEngineProcessConnection::CreateEngineConnection(xiiAssetDocument* pDocument)
{
  xiiEditorEngineConnection* pConnection = new xiiEditorEngineConnection(pDocument);

  m_DocumentByGuid[pDocument->GetGuid()] = pDocument;

  pDocument->SendDocumentOpenMessage(true);

  return pConnection;
}

void xiiEditorEngineProcessConnection::DestroyEngineConnection(xiiAssetDocument* pDocument)
{
  pDocument->SendDocumentOpenMessage(false);

  m_DocumentByGuid.Remove(pDocument->GetGuid());

  delete pDocument->GetEditorEngineConnection();
}

void xiiEditorEngineProcessConnection::Initialize(const xiiRTTI* pFirstAllowedMessageType)
{
  XII_PROFILE_SCOPE("Initialize");
  if (m_IPC.IsClientAlive())
    return;

  xiiLog::Dev("Starting Client Engine Process");

  XII_ASSERT_DEBUG(m_TickEventSubscriptionID == 0, "A previous subscription is still in place. ShutdownProcess not called?");
  m_TickEventSubscriptionID = xiiQtUiServices::s_TickEvent.AddEventHandler(xiiMakeDelegate(&xiiEditorEngineProcessConnection::UIServicesTickEventHandler, this));

  m_bProcessShouldBeRunning = true;
  m_bProcessCrashed         = false;
  m_bClientIsConfigured     = false;

  QStringList args;
  if (m_bProcessShouldWaitForDebugger)
  {
    args << "-debug";
  }

  if (!m_sRenderer.IsEmpty())
  {
    args << "-renderer";
    args << m_sRenderer.GetData();
  }

  {
    xiiStringBuilder sWndCfgPath = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sWndCfgPath.AppendPath("RuntimeConfigs/Window.ddl");

    if (xiiFileSystem::ExistsFile(sWndCfgPath))
    {
      args << "-wnd";
      args << sWndCfgPath.GetData();
    }
  }

  // set up the EditorEngineProcess telemetry server on a different port
  {
    args << "-TelemetryPort";
    args << xiiCommandLineUtils::GetGlobalInstance()->GetStringOption("-TelemetryPort", 0, "1050");
  }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const char* EditorEngineProcessExecutableName = "EditorEngineProcess.exe";
#elif XII_ENABLED(XII_PLATFORM_LINUX)
  const char* EditorEngineProcessExecutableName = "EditorEngineProcess";
#else
#  error Platform not supported
#endif


  if (m_IPC.StartClientProcess(EditorEngineProcessExecutableName, args, false, pFirstAllowedMessageType).Failed())
  {
    m_bProcessCrashed = true;
  }
  else
  {
    Event e;
    e.m_Type = Event::Type::ProcessStarted;
    s_Events.Broadcast(e);
  }
}

void xiiEditorEngineProcessConnection::ActivateRemoteProcess(const xiiAssetDocument* pDocument, xiiUInt32 uiViewID)
{
  // make sure process is started
  if (!ConnectToRemoteProcess())
    return;

  // resend entire document
  {
    // open document message
    {
      xiiDocumentOpenMsgToEngine msg;
      msg.m_DocumentGuid  = pDocument->GetGuid();
      msg.m_bDocumentOpen = true;
      msg.m_sDocumentType = pDocument->GetDocumentTypeDescriptor()->m_sDocumentTypeName;
      m_pRemoteProcess->SendMessage(&msg);
    }

    if (pDocument->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocument>())
    {
      xiiAssetDocument*                  pAssetDoc = (xiiAssetDocument*)pDocument;
      xiiDocumentOpenResponseMsgToEditor response;
      response.m_DocumentGuid = pDocument->GetGuid();
      pAssetDoc->HandleEngineMessage(&response);
    }
  }

  // send activation message
  {
    xiiActivateRemoteViewMsgToEngine msg;
    msg.m_DocumentGuid = pDocument->GetGuid();
    msg.m_uiViewID     = uiViewID;
    m_pRemoteProcess->SendMessage(&msg);
  }
}

bool xiiEditorEngineProcessConnection::ConnectToRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    if (m_pRemoteProcess->IsConnected())
      return true;

    ShutdownRemoteProcess();
  }

  xiiQtRemoteConnectionDlg dlg(QApplication::activeWindow());

  if (dlg.exec() == QDialog::Rejected)
    return false;

  m_pRemoteProcess = XII_DEFAULT_NEW(xiiEditorProcessRemoteCommunicationChannel);
  m_pRemoteProcess->ConnectToServer(dlg.GetResultingAddress().toUtf8().data()).IgnoreResult();

  xiiQtWaitForOperationDlg waitDialog(QApplication::activeWindow());
  waitDialog.m_OnIdle = [this]() -> bool {
    if (m_pRemoteProcess->IsConnected())
      return false;

    m_pRemoteProcess->TryConnect();
    return true;
  };

  const int iRet = waitDialog.exec();

  if (iRet == QDialog::Accepted)
  {
    // Send project setup.
    xiiSetupProjectMsgToEngine msg;
    msg.m_sProjectDir       = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig  = m_FileSystemConfig;
    msg.m_PluginConfig      = m_PluginConfig;
    msg.m_sFileserveAddress = dlg.GetResultingFsAddress().toUtf8().data();
    msg.m_sAssetProfile     = xiiAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    m_pRemoteProcess->SendMessage(&msg);
  }

  return iRet == QDialog::Accepted;
}


void xiiEditorEngineProcessConnection::ShutdownRemoteProcess()
{
  if (m_pRemoteProcess != nullptr)
  {
    xiiLog::Info("Shutting down Remote Engine Process");
    m_pRemoteProcess->CloseConnection();

    m_pRemoteProcess = nullptr;
  }
}

void xiiEditorEngineProcessConnection::ShutdownProcess()
{
  if (!m_bProcessShouldBeRunning)
    return;

  ShutdownRemoteProcess();

  xiiLog::Info("Shutting down Engine Process");

  if (m_TickEventSubscriptionID != 0)
    xiiQtUiServices::s_TickEvent.RemoveEventHandler(m_TickEventSubscriptionID);

  m_bClientIsConfigured     = false;
  m_bProcessShouldBeRunning = false;
  m_IPC.CloseConnection();

  Event e;
  e.m_Type = Event::Type::ProcessShutdown;
  s_Events.Broadcast(e);
}

void xiiEditorEngineProcessConnection::SendMessage(xiiProcessMessage* pMessage)
{
  m_IPC.SendMessage(pMessage);

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->SendMessage(pMessage);
  }
}

xiiResult xiiEditorEngineProcessConnection::WaitForMessage(const xiiRTTI* pMessageType, xiiTime tTimeout, xiiProcessCommunicationChannel::WaitForMessageCallback* pCallback)
{
  XII_PROFILE_SCOPE(pMessageType->GetTypeName());
  return m_IPC.WaitForMessage(pMessageType, tTimeout, pCallback);
}

xiiResult xiiEditorEngineProcessConnection::WaitForDocumentMessage(const xiiUuid& assetGuid, const xiiRTTI* pMessageType, xiiTime tTimeout, xiiProcessCommunicationChannel::WaitForMessageCallback* pCallback /*= nullptr*/)
{
  if (!m_bProcessShouldBeRunning)
  {
    return XII_FAILURE; // if the process is not running, we can't wait for a message
  }
  XII_ASSERT_DEBUG(pMessageType->IsDerivedFrom(xiiGetStaticRTTI<xiiEditorEngineDocumentMsg>()), "The type of the message to wait for must be a document message.");
  struct WaitData
  {
    xiiUuid                                                 m_AssetGuid;
    xiiProcessCommunicationChannel::WaitForMessageCallback* m_pCallback;
  };

  WaitData data;
  data.m_AssetGuid = assetGuid;
  data.m_pCallback = pCallback;

  xiiProcessCommunicationChannel::WaitForMessageCallback callback = [&data](xiiProcessMessage* pMsg) -> bool {
    xiiEditorEngineDocumentMsg* pMsg2 = xiiDynamicCast<xiiEditorEngineDocumentMsg*>(pMsg);
    if (pMsg2 && data.m_AssetGuid == pMsg2->m_DocumentGuid)
    {
      if (data.m_pCallback && data.m_pCallback->IsValid() && !(*data.m_pCallback)(pMsg))
      {
        return false;
      }
      return true;
    }
    return false;
  };

  return m_IPC.WaitForMessage(pMessageType, tTimeout, &callback);
}

xiiResult xiiEditorEngineProcessConnection::RestartProcess()
{
  XII_PROFILE_SCOPE("RestartProcess");
  XII_LOG_BLOCK("Restarting Engine Process");

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage("Reloading Engine Process...", xiiTime::Seconds(5));

  ShutdownProcess();

  Initialize(xiiGetStaticRTTI<xiiSetupProjectMsgToEngine>());

  if (m_bProcessCrashed)
  {
    xiiLog::Error("Engine process crashed during startup.");
    ShutdownProcess();
    return XII_FAILURE;
  }

  xiiLog::Dev("Waiting for IPC connection");

  if (m_IPC.WaitForConnection(xiiTime()).Failed())
  {
    xiiLog::Error("Engine process did not connect. Engine process output:\n{}", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return XII_FAILURE;
  }

  {
    // Send project setup.
    xiiSetupProjectMsgToEngine msg;
    msg.m_sProjectDir      = xiiToolsProject::GetSingleton()->GetProjectDirectory();
    msg.m_FileSystemConfig = m_FileSystemConfig;
    msg.m_PluginConfig     = m_PluginConfig;
    msg.m_sAssetProfile    = xiiAssetCurator::GetSingleton()->GetActiveAssetProfile()->GetConfigName();

    SendMessage(&msg);
  }

  xiiLog::Dev("Waiting for Engine Process response");

  if (WaitForMessage(xiiGetStaticRTTI<xiiProjectReadyMsgToEditor>(), xiiTime()).Failed())
  {
    xiiLog::Error("Failed to restart the engine process. Engine Process Output:\n", m_IPC.GetStdoutContents());
    ShutdownProcess();
    return XII_FAILURE;
  }

  xiiLog::Dev("Transmitting open documents to Engine Process");

  xiiHybridArray<xiiAssetDocument*, 6> docs;
  docs.Reserve(m_DocumentByGuid.GetCount());

  // Resend all open documents. Make sure to send main documents before child documents.
  for (auto it = m_DocumentByGuid.GetIterator(); it.IsValid(); ++it)
  {
    docs.PushBack(it.Value());
  }
  docs.Sort([](const xiiAssetDocument* a, const xiiAssetDocument* b) {
    if (a->IsMainDocument() != b->IsMainDocument())
      return a->IsMainDocument();
    return a < b;
  });

  for (xiiAssetDocument* pDoc : docs)
  {
    pDoc->SendDocumentOpenMessage(true);
  }

  xiiAssetCurator::GetSingleton()->InvalidateAssetsWithTransformState(xiiAssetInfo::TransformState::TransformError);

  xiiLog::Success("Engine Process is running");

  m_bClientIsConfigured = true;

  Event e;
  e.m_Type = Event::Type::ProcessRestarted;
  s_Events.Broadcast(e);

  return XII_SUCCESS;
}

void xiiEditorEngineProcessConnection::Update()
{
  if (!m_bProcessShouldBeRunning)
    return;

  if (!m_IPC.IsClientAlive())
  {
    ShutdownProcess();
    m_bProcessCrashed = true;

    Event e;
    e.m_Type = Event::Type::ProcessCrashed;
    s_Events.Broadcast(e);

    return;
  }

  m_IPC.ProcessMessages();

  if (m_pRemoteProcess)
  {
    m_pRemoteProcess->ProcessMessages();
  }
}

void xiiEditorEngineConnection::SendMessage(xiiEditorEngineDocumentMsg* pMessage)
{
  XII_ASSERT_DEV(this != nullptr, "No connection between editor and engine was created. This typically happens when an asset document does "
                                  "not enable the engine-connection through the constructor of xiiAssetDocument.");

  pMessage->m_DocumentGuid = m_pDocument->GetGuid();

  xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(pMessage);
}

void xiiEditorEngineConnection::SendHighlightObjectMessage(xiiViewHighlightMsgToEngine* pMessage)
{
  // without this check there will be so many messages, that the editor comes to a crawl (< 10 FPS)
  // This happens because Qt sends hundreds of mouse-move events and since each 'SendMessageToEngine'
  // requires a round-trip to the engine process, doing this too often will be sloooow

  static xiiUuid LastHighlightGuid;

  if (LastHighlightGuid == pMessage->m_HighlightObject)
    return;

  LastHighlightGuid = pMessage->m_HighlightObject;
  SendMessage(pMessage);
}
