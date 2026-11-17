/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Inspector/InspectorPCH.h>

#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ProfilingWidget.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/RenderGraphWidget.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <initializer_list>

const int g_iDockingStateVersion = 3;

xiiQtMainWindow* xiiQtMainWindow::s_pWidget = nullptr;

xiiQtMainWindow::xiiQtMainWindow() :
  QMainWindow()
{
  s_pWidget = this;
  m_DiagnosticsClient.SetSampleHandler([](const xiiTelemetrySample& sample) {
    if (sample.m_uiSource == xiiTelemetrySources::Counters)
      xiiQtMainWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Logs)
      xiiQtLogDockWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Memory)
      xiiQtMemoryWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::GlobalEvents)
      xiiQtGlobalEventsWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Plugins)
      xiiQtPluginsWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Subsystems)
      xiiQtSubsystemsWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Input)
      xiiQtInputWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Time)
      xiiQtTimeWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Files)
      xiiQtFileWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Application)
      xiiQtMainWindow::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Reflection)
      xiiQtReflectionWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::Resources)
      xiiQtResourceWidget::ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::CVars)
      xiiQtCVarsWidget::ReceiveDiagnostics(sample);
    else if ((sample.m_uiSource == xiiTelemetrySources::Profiling || sample.m_uiSource == xiiTelemetrySources::ProfileHistory) && xiiQtProfilingWidget::s_pWidget)
      xiiQtProfilingWidget::s_pWidget->ReceiveDiagnostics(sample);
    else if (sample.m_uiSource == xiiTelemetrySources::RenderGraphs && xiiQtRenderGraphWidget::s_pWidget)
      xiiQtRenderGraphWidget::s_pWidget->ReceiveDiagnostics(sample);
  });

  setupUi(this);

  m_pDockManager = new ads::CDockManager(this);
  m_pDockManager->setConfigFlags(static_cast<ads::CDockManager::ConfigFlags>(ads::CDockManager::DockAreaHasCloseButton | ads::CDockManager::DockAreaCloseButtonClosesTab | ads::CDockManager::OpaqueSplitterResize | ads::CDockManager::AllTabsHaveCloseButton));

  QSettings Settings;
  SetAlwaysOnTop((OnTopMode)Settings.value("AlwaysOnTop", (int)WhenConnected).toInt());

  Settings.beginGroup("MainWindow");

  const bool bRestoreDockingState = Settings.value("DockingVersion") == g_iDockingStateVersion;

  if (bRestoreDockingState)
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
  }

  // The dock manager will set ownership to null on add so there is no reason to provide an owner here.
  // Setting one will actually cause memory corruptions on shutdown for unknown reasons.
  xiiQtMainWidget*         pMainWidget         = new xiiQtMainWidget(m_pDockManager);
  xiiQtLogDockWidget*      pLogWidget          = new xiiQtLogDockWidget(m_pDockManager);
  xiiQtMemoryWidget*       pMemoryWidget       = new xiiQtMemoryWidget(m_pDockManager);
  xiiQtTimeWidget*         pTimeWidget         = new xiiQtTimeWidget(m_pDockManager);
  xiiQtInputWidget*        pInputWidget        = new xiiQtInputWidget(m_pDockManager);
  xiiQtCVarsWidget*        pCVarsWidget        = new xiiQtCVarsWidget(m_pDockManager);
  xiiQtSubsystemsWidget*   pSubsystemsWidget   = new xiiQtSubsystemsWidget(m_pDockManager);
  xiiQtFileWidget*         pFileWidget         = new xiiQtFileWidget(m_pDockManager);
  xiiQtPluginsWidget*      pPluginsWidget      = new xiiQtPluginsWidget(m_pDockManager);
  xiiQtGlobalEventsWidget* pGlobalEventsWidget = new xiiQtGlobalEventsWidget(m_pDockManager);
  xiiQtReflectionWidget*   pReflectionWidget   = new xiiQtReflectionWidget(m_pDockManager);
  xiiQtResourceWidget*     pResourceWidget     = new xiiQtResourceWidget(m_pDockManager);
  auto*                    pRenderGraphWidget  = new xiiQtRenderGraphWidget(m_pDockManager);
  pRenderGraphWidget->SetClient(&m_DiagnosticsClient);
  auto* pProfilingWidget                    = new xiiQtProfilingWidget(m_pDockManager);
  pProfilingWidget->GetSession()->m_pClient = &m_DiagnosticsClient;
  auto* pRenderingWidget                    = new xiiQtProfilingWidget(m_pDockManager, true, pProfilingWidget->GetSession());
  menuWindows->addAction(pRenderingWidget->toggleViewAction());
  menuWindows->addAction(pRenderGraphWidget->toggleViewAction());
  menuWindows->addAction(pProfilingWidget->toggleViewAction());

  XII_VERIFY(nullptr != QWidget::connect(pMainWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pLogWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pTimeWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pMemoryWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pInputWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pCVarsWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pReflectionWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pSubsystemsWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pFileWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pPluginsWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pGlobalEventsWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pResourceWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");

  QMenu* pHistoryMenu = new QMenu;
  pHistoryMenu->setTearOffEnabled(true);
  pHistoryMenu->setTitle(QLatin1String("Stat Histories"));
  pHistoryMenu->setIcon(QIcon(":/Icons/Icons/StatHistory.svg"));

  for (xiiUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i] = new xiiQtStatVisWidget(m_pDockManager, this, i);
    m_pDockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);

    XII_VERIFY(nullptr != QWidget::connect(m_pStatHistoryWidgets[i], &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");

    pHistoryMenu->addAction(&m_pStatHistoryWidgets[i]->m_ShowWindowAction);

    m_pActionShowStatIn[i] = new QAction(this);

    XII_VERIFY(nullptr != QWidget::connect(m_pActionShowStatIn[i], &QAction::triggered, xiiQtMainWidget::s_pWidget, &xiiQtMainWidget::ShowStatIn), "");
  }

  // delay this until after all widgets are created
  for (xiiUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->toggleView(false); // hide
  }

  setContextMenuPolicy(Qt::NoContextMenu);

  menuWindows->addMenu(pHistoryMenu);

  pMemoryWidget->raise();

  auto* analysisArea = m_pDockManager->addDockWidget(ads::CenterDockWidgetArea, pProfilingWidget);
  m_pDockManager->addDockWidgetTabToArea(pRenderingWidget, analysisArea);
  m_pDockManager->addDockWidgetTabToArea(pRenderGraphWidget, analysisArea);
  m_pDockManager->addDockWidgetTabToArea(pLogWidget, analysisArea);
  auto* applicationArea = m_pDockManager->addDockWidget(ads::LeftDockWidgetArea, pMainWidget, analysisArea);
  auto* detailsArea     = m_pDockManager->addDockWidget(ads::RightDockWidgetArea, pCVarsWidget, analysisArea);
  for (ads::CDockWidget* widget : std::initializer_list<ads::CDockWidget*>{pGlobalEventsWidget, pInputWidget, pPluginsWidget, pReflectionWidget, pResourceWidget, pSubsystemsWidget})
    m_pDockManager->addDockWidgetTabToArea(widget, detailsArea);
  auto* bottomArea = m_pDockManager->addDockWidget(ads::BottomDockWidgetArea, pFileWidget, analysisArea);
  m_pDockManager->addDockWidgetTabToArea(pMemoryWidget, bottomArea);
  m_pDockManager->addDockWidgetTabToArea(pTimeWidget, bottomArea);
  for (ads::CDockWidget* widget : std::initializer_list<ads::CDockWidget*>{pCVarsWidget, pGlobalEventsWidget, pInputWidget, pPluginsWidget, pReflectionWidget, pResourceWidget, pSubsystemsWidget, pFileWidget, pMemoryWidget, pTimeWidget})
    widget->toggleView(false);
  m_pDockManager->setSplitterSizes(applicationArea, {260, 1180});
  pProfilingWidget->raise();

  if (bRestoreDockingState)
  {
    auto dockState = Settings.value("DockManagerState");
    if (dockState.isValid() && dockState.typeId() == QMetaType::QByteArray)
    {
      m_pDockManager->restoreState(dockState.toByteArray(), 1);
    }

    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());

    if (Settings.value("IsMaximized", isMaximized()).toBool())
    {
      showMaximized();
    }

    restoreState(Settings.value("WindowState", saveState()).toByteArray());
  }

  if (!bRestoreDockingState)
    resize(1440, 900);
  Settings.endGroup();

  for (xiiInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->Load();

  SetupNetworkTimer();
}

xiiQtMainWindow::~xiiQtMainWindow()
{
  for (xiiInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->Save();
  }
  // The dock manager does not take ownership of dock widgets.
  auto dockWidgets = m_pDockManager->dockWidgetsMap();
  for (auto it = dockWidgets.begin(); it != dockWidgets.end(); ++it)
  {
    m_pDockManager->removeDockWidget(it.value());
    delete it.value();
  }
}

void xiiQtMainWindow::closeEvent(QCloseEvent* pEvent)
{
  const bool bMaximized = isMaximized();
  if (bMaximized)
    showNormal();

  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("DockingVersion", g_iDockingStateVersion);
  Settings.setValue("DockManagerState", m_pDockManager->saveState(1));
  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", bMaximized);
  Settings.setValue("WindowPosition", pos());
  if (!bMaximized)
    Settings.setValue("WindowSize", size());

  Settings.endGroup();
}

void xiiQtMainWindow::SetupNetworkTimer()
{
  // reset the timer to fire again
  if (m_pNetworkTimer == nullptr)
    m_pNetworkTimer = new QTimer(this);

  m_pNetworkTimer->singleShot(16, this, SLOT(UpdateNetworkTimeOut()));
}

void xiiQtMainWindow::UpdateNetworkTimeOut()
{
  UpdateNetwork();

  SetupNetworkTimer();
}

void xiiQtMainWindow::UpdateNetwork()
{
  m_DiagnosticsClient.Update();
  xiiNetworkRpcResult         command;
  xiiTelemetryCommandResponse response;
  for (xiiUInt32 i = 0; i < 32 && m_DiagnosticsClient.TryGetCommandResult(command, response); ++i)
    xiiQtCVarsWidget::ReceiveCommandResult(command, response);
  bool bReady = m_DiagnosticsClient.IsReady();
  if (bReady && m_ExpectedInstance.IsValid() && m_ExpectedInstance != m_DiagnosticsClient.GetServerInfo().m_Instance)
  {
    m_DiagnosticsClient.Shutdown();
    bReady = false;
    xiiQtLogDockWidget::s_pWidget->Log("The discovered application instance changed; reconnect explicitly.");
  }
  if (bReady && m_uiCatalogRevision != m_DiagnosticsClient.GetServerInfo().m_uiCatalogRevision)
  {
    m_uiCatalogRevision = m_DiagnosticsClient.GetServerInfo().m_uiCatalogRevision;
    m_bDiagnosticsReady = false;
  }
  if (bReady && !m_bDiagnosticsReady)
  {
    ++m_uiDiagnosticsGeneration;
    xiiTelemetrySubscription counters;
    counters.m_uiSource               = xiiTelemetrySources::Counters;
    counters.m_uiGeneration           = m_uiDiagnosticsGeneration;
    counters.m_uiRequestId            = ++m_uiDiagnosticsRequest;
    counters.m_uiIntervalMilliseconds = 100;
    m_DiagnosticsClient.Subscribe(counters).IgnoreResult();
    m_DiagnosticsSubscribed.Clear();
  }
  struct SourceView
  {
    xiiUInt64         m_uiSource;
    ads::CDockWidget* m_pView;
    xiiUInt32         m_uiInterval;
  };
  const SourceView views[] = {
    {xiiTelemetrySources::Logs, xiiQtLogDockWidget::s_pWidget, 33},
    {xiiTelemetrySources::Memory, xiiQtMemoryWidget::s_pWidget, 200},
    {xiiTelemetrySources::Plugins, xiiQtPluginsWidget::s_pWidget, 1000},
    {xiiTelemetrySources::Subsystems, xiiQtSubsystemsWidget::s_pWidget, 1000},
    {xiiTelemetrySources::GlobalEvents, xiiQtGlobalEventsWidget::s_pWidget, 500},
    {xiiTelemetrySources::Input, xiiQtInputWidget::s_pWidget, 100},
    {xiiTelemetrySources::Time, xiiQtTimeWidget::s_pWidget, 33},
    {xiiTelemetrySources::Files, xiiQtFileWidget::s_pWidget, 33},
    {xiiTelemetrySources::Reflection, xiiQtReflectionWidget::s_pWidget, 5000},
    {xiiTelemetrySources::Resources, xiiQtResourceWidget::s_pWidget, 500},
    {xiiTelemetrySources::CVars, xiiQtCVarsWidget::s_pWidget, 500},
    {xiiTelemetrySources::Application, xiiQtMainWidget::s_pWidget, 33},
  };
  for (const auto& view : views)
  {
    const bool visible = m_DiagnosticsClient.IsSourceAvailable(view.m_uiSource) && view.m_pView->isVisible() && !view.m_pView->isClosed();
    if (!bReady || visible == m_DiagnosticsSubscribed.Contains(view.m_uiSource))
      continue;
    xiiTelemetrySubscription request;
    request.m_uiSource               = view.m_uiSource;
    request.m_uiGeneration           = m_uiDiagnosticsGeneration;
    request.m_uiRequestId            = ++m_uiDiagnosticsRequest;
    request.m_uiIntervalMilliseconds = view.m_uiInterval;
    request.m_bEnabled               = visible;
    if (m_DiagnosticsClient.Subscribe(request).Succeeded())
    {
      if (visible)
        m_DiagnosticsSubscribed.Insert(view.m_uiSource);
      else
        m_DiagnosticsSubscribed.Remove(view.m_uiSource);
    }
  }
  m_bDiagnosticsReady = bReady;
  bool bResetStats    = false;

  if (bReady && !(m_LastInstance == m_DiagnosticsClient.GetServerInfo().m_Instance))
  {
    m_LastInstance = m_DiagnosticsClient.GetServerInfo().m_Instance;
    bResetStats    = true;
  }
  if (bReady)
    setWindowTitle(QString("xiiInspector - %1").arg(QString::fromUtf8(m_DiagnosticsClient.GetServerInfo().m_sApplication.GetData())));
  else
    setWindowTitle("xiiInspector - disconnected");
  if (bResetStats)
  {
    xiiQtMainWidget::s_pWidget->ResetStats();
    xiiQtLogDockWidget::s_pWidget->ResetStats();
    xiiQtMemoryWidget::s_pWidget->ResetStats();
    xiiQtTimeWidget::s_pWidget->ResetStats();
    xiiQtInputWidget::s_pWidget->ResetStats();
    xiiQtCVarsWidget::s_pWidget->ResetStats();
    xiiQtReflectionWidget::s_pWidget->ResetStats();
    xiiQtFileWidget::s_pWidget->ResetStats();
    xiiQtPluginsWidget::s_pWidget->ResetStats();
    xiiQtSubsystemsWidget::s_pWidget->ResetStats();
    xiiQtGlobalEventsWidget::s_pWidget->ResetStats();
    xiiQtResourceWidget::s_pWidget->ResetStats();
    xiiQtRenderGraphWidget::s_pWidget->ResetStats();
    xiiQtProfilingWidget::s_pWidget->ResetStats();
  }

  UpdateAlwaysOnTop();

  xiiQtMainWidget::s_pWidget->UpdateStats();
  xiiQtPluginsWidget::s_pWidget->UpdateStats();
  xiiQtSubsystemsWidget::s_pWidget->UpdateStats();
  xiiQtMemoryWidget::s_pWidget->UpdateStats();
  xiiQtTimeWidget::s_pWidget->UpdateStats();
  xiiQtFileWidget::s_pWidget->UpdateStats();
  xiiQtResourceWidget::s_pWidget->UpdateStats();

  for (xiiInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->UpdateStats();
}

xiiResult xiiQtMainWindow::ConnectDiagnostics(xiiStringView sAddress, xiiUuid expectedInstance)
{
  auto resetView = [&] {
    m_ExpectedInstance  = expectedInstance;
    m_LastInstance      = {};
    m_bDiagnosticsReady = false;
    xiiQtProfilingWidget::s_pWidget->GetSession()->ResetConnection();
  };
  xiiTelemetryConfiguration config;
  config.m_sApplication = "XII Inspector";
  const bool bLocal     = sAddress.StartsWith("local:");
  if (bLocal)
  {
    sAddress.Shrink(6, 0);
    xiiNetworkLocalEndpoint endpoint;
    XII_SUCCEED_OR_RETURN(xiiNetworkLocalEndpoint::Make(sAddress, endpoint).GetResult());
    config.m_Network.m_Transport      = xiiNetworkTransportType::Local;
    config.m_Network.m_Authentication = xiiNetworkAuthenticationPolicy::LocalUser;
    resetView();
    return m_DiagnosticsClient.Connect(config, endpoint).GetResult();
  }

  if (sAddress.TrimWordStart("quic://"))
    config.m_Network.m_Transport = xiiNetworkTransportType::SecureDatagram;
  else if (!sAddress.TrimWordStart("tls://"))
    return XII_FAILURE;

  xiiNetworkEndpoint endpoint;
  XII_SUCCEED_OR_RETURN(xiiNetworkEndpoint::Parse(sAddress, endpoint).GetResult());
  const auto*              command = xiiCommandLineUtils::GetGlobalInstance();
  xiiNetworkSecurityConfig security;
  security.m_sCertificateChainFile = command->GetStringOption("-DiagnosticsCert");
  security.m_sPrivateKeyFile       = command->GetStringOption("-DiagnosticsKey");
  security.m_sTrustAnchorsFile     = command->GetStringOption("-DiagnosticsCA");
  security.m_sApplicationProtocol  = "xii-diagnostics/2";
  security.m_bUseSystemTrust       = false;
  config.m_Network.m_pCredentials  = XII_DEFAULT_NEW(xiiNetworkCredentials);
  XII_SUCCEED_OR_RETURN(config.m_Network.m_pCredentials->Load(security).GetResult());
  xiiNetworkConnectOptions options;
  options.m_sServerName = command->GetStringOption("-DiagnosticsServerName");
  resetView();
  return m_DiagnosticsClient.Connect(config, endpoint, options).GetResult();
}

void xiiQtMainWindow::DockWidgetVisibilityChanged(bool bVisible)
{
  // TODO: add menu entry for qt main widget

  ActionShowWindowLog->setChecked(!xiiQtLogDockWidget::s_pWidget->isClosed());
  ActionShowWindowMemory->setChecked(!xiiQtMemoryWidget::s_pWidget->isClosed());
  ActionShowWindowTime->setChecked(!xiiQtTimeWidget::s_pWidget->isClosed());
  ActionShowWindowInput->setChecked(!xiiQtInputWidget::s_pWidget->isClosed());
  ActionShowWindowCVar->setChecked(!xiiQtCVarsWidget::s_pWidget->isClosed());
  ActionShowWindowReflection->setChecked(!xiiQtReflectionWidget::s_pWidget->isClosed());
  ActionShowWindowSubsystems->setChecked(!xiiQtSubsystemsWidget::s_pWidget->isClosed());
  ActionShowWindowFile->setChecked(!xiiQtFileWidget::s_pWidget->isClosed());
  ActionShowWindowPlugins->setChecked(!xiiQtPluginsWidget::s_pWidget->isClosed());
  ActionShowWindowGlobalEvents->setChecked(!xiiQtGlobalEventsWidget::s_pWidget->isClosed());
  ActionShowWindowResource->setChecked(!xiiQtResourceWidget::s_pWidget->isClosed());

  for (xiiInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->m_ShowWindowAction.setChecked(!m_pStatHistoryWidgets[i]->isClosed());
}


void xiiQtMainWindow::SetAlwaysOnTop(OnTopMode Mode)
{
  m_OnTopMode = Mode;

  QSettings Settings;
  Settings.setValue("AlwaysOnTop", (int)m_OnTopMode);

  ActionNeverOnTop->setChecked((m_OnTopMode == Never) ? Qt::Checked : Qt::Unchecked);
  ActionAlwaysOnTop->setChecked((m_OnTopMode == Always) ? Qt::Checked : Qt::Unchecked);
  ActionOnTopWhenConnected->setChecked((m_OnTopMode == WhenConnected) ? Qt::Checked : Qt::Unchecked);

  UpdateAlwaysOnTop();
}

void xiiQtMainWindow::UpdateAlwaysOnTop()
{
  static bool bOnTop = false;

  bool bNewState = bOnTop;
  XII_IGNORE_UNUSED(bNewState);

  if (m_OnTopMode == Always || (m_OnTopMode == WhenConnected && m_DiagnosticsClient.IsReady()))
    bNewState = true;
  else
    bNewState = false;

  if (bOnTop != bNewState)
  {
    bOnTop = bNewState;

    hide();

    if (bOnTop)
      setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
      setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);

    show();
  }
}

void xiiQtMainWindow::ReceiveDiagnostics(const xiiTelemetrySample& sample)
{
  if (!s_pWidget || sample.m_Records.GetCount() != 1)
    return;
  const auto& row = sample.m_Records[0];
  xiiString   sSourceFile, sFunction, sExpression, sMessage;
  xiiUInt32   uiLine;
  if (!row.Read("file", sSourceFile) || !row.Read("line", uiLine) || !row.Read("function", sFunction) || !row.Read("expression", sExpression) || !row.Read("message", sMessage))
    return;
  xiiQtLogDockWidget::s_pWidget->Log("");
  xiiQtLogDockWidget::s_pWidget->Log("<<< Application Assertion >>>");
  xiiQtLogDockWidget::s_pWidget->Log("");

  xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("    Expression: '{0}'", sExpression));
  xiiQtLogDockWidget::s_pWidget->Log("");

  xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("    Message: '{0}'", sMessage));
  xiiQtLogDockWidget::s_pWidget->Log("");

  xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("   File: '{0}'", sSourceFile));

  xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("   Line: {0}", uiLine));

  xiiQtLogDockWidget::s_pWidget->Log(xiiFmt("   In Function: '{0}'", sFunction));

  xiiQtLogDockWidget::s_pWidget->Log("");

  xiiQtLogDockWidget::s_pWidget->Log(">>> Application Assertion <<<");
  xiiQtLogDockWidget::s_pWidget->Log("");
}
