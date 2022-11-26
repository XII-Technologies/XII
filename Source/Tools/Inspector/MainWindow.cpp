#include <Inspector/InspectorPCH.h>

#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

const int g_iDockingStateVersion = 1;

xiiQtMainWindow* xiiQtMainWindow::s_pWidget = nullptr;

xiiQtMainWindow::xiiQtMainWindow() :
  QMainWindow()
{
  s_pWidget = this;

  setupUi(this);

  m_DockManager = new ads::CDockManager(this);
  m_DockManager->setConfigFlags(
    static_cast<ads::CDockManager::ConfigFlags>(ads::CDockManager::DockAreaHasCloseButton | ads::CDockManager::DockAreaCloseButtonClosesTab |
                                                ads::CDockManager::OpaqueSplitterResize | ads::CDockManager::AllTabsHaveCloseButton));

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
  xiiQtMainWidget*         pMainWidget          = new xiiQtMainWidget();
  xiiQtLogDockWidget*      pLogWidget           = new xiiQtLogDockWidget();
  xiiQtMemoryWidget*       pMemoryWidget        = new xiiQtMemoryWidget();
  xiiQtTimeWidget*         pTimeWidget          = new xiiQtTimeWidget();
  xiiQtInputWidget*        pInputWidget         = new xiiQtInputWidget();
  xiiQtCVarsWidget*        pCVarsWidget         = new xiiQtCVarsWidget();
  xiiQtSubsystemsWidget*   pSubsystemsWidget    = new xiiQtSubsystemsWidget();
  xiiQtFileWidget*         pFileWidget          = new xiiQtFileWidget();
  xiiQtPluginsWidget*      pPluginsWidget       = new xiiQtPluginsWidget();
  xiiQtGlobalEventsWidget* pGlobalEventesWidget = new xiiQtGlobalEventsWidget();
  xiiQtReflectionWidget*   pReflectionWidget    = new xiiQtReflectionWidget();
  xiiQtDataWidget*         pDataWidget          = new xiiQtDataWidget();
  xiiQtResourceWidget*     pResourceWidget      = new xiiQtResourceWidget();

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
  XII_VERIFY(
    nullptr != QWidget::connect(pGlobalEventesWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pDataWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");
  XII_VERIFY(nullptr != QWidget::connect(pResourceWidget, &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");

  QMenu* pHistoryMenu = new QMenu;
  pHistoryMenu->setTearOffEnabled(true);
  pHistoryMenu->setTitle(QLatin1String("Stat Histories"));
  pHistoryMenu->setIcon(QIcon(":/Icons/Icons/StatHistory.png"));

  for (xiiUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i] = new xiiQtStatVisWidget(this, i);
    m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);

    XII_VERIFY(
      nullptr != QWidget::connect(m_pStatHistoryWidgets[i], &ads::CDockWidget::viewToggled, this, &xiiQtMainWindow::DockWidgetVisibilityChanged), "");

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

  m_DockManager->addDockWidget(ads::LeftDockWidgetArea, pMainWidget);
  m_DockManager->addDockWidget(ads::CenterDockWidgetArea, pLogWidget);

  m_DockManager->addDockWidget(ads::RightDockWidgetArea, pCVarsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pGlobalEventesWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pDataWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pInputWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPluginsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pReflectionWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pResourceWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pSubsystemsWidget);

  m_DockManager->addDockWidget(ads::BottomDockWidgetArea, pFileWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pMemoryWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pTimeWidget);


  pLogWidget->raise();
  pCVarsWidget->raise();

  if (bRestoreDockingState)
  {
    auto dockState = Settings.value("DockManagerState");
    if (dockState.isValid() && dockState.type() == QVariant::ByteArray)
    {
      m_DockManager->restoreState(dockState.toByteArray(), 1);
    }

    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());

    if (Settings.value("IsMaximized", isMaximized()).toBool())
    {
      showMaximized();
    }

    restoreState(Settings.value("WindowState", saveState()).toByteArray());
  }

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
  auto dockWidgets = m_DockManager->dockWidgetsMap();
  for (auto it = dockWidgets.begin(); it != dockWidgets.end(); ++it)
  {
    m_DockManager->removeDockWidget(it.value());
    delete it.value();
  }
}

void xiiQtMainWindow::closeEvent(QCloseEvent* event)
{
  const bool bMaximized = isMaximized();
  if (bMaximized)
    showNormal();

  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("DockingVersion", g_iDockingStateVersion);
  Settings.setValue("DockManagerState", m_DockManager->saveState(1));
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

  m_pNetworkTimer->singleShot(40, this, SLOT(UpdateNetworkTimeOut()));
}

void xiiQtMainWindow::UpdateNetworkTimeOut()
{
  UpdateNetwork();

  SetupNetworkTimer();
}

void xiiQtMainWindow::UpdateNetwork()
{
  bool bResetStats = false;

  {
    static xiiUInt32 uiServerID = 0;
    static bool      bConnected = false;
    static xiiString sLastServerName;

    if (xiiTelemetry::IsConnectedToServer())
    {
      if (uiServerID != xiiTelemetry::GetServerID())
      {
        uiServerID  = xiiTelemetry::GetServerID();
        bResetStats = true;

        xiiStringBuilder s;
        s.Format("Connected to new Server with ID {0}", uiServerID);

        xiiQtLogDockWidget::s_pWidget->Log(s.GetData());
      }
      else if (!bConnected)
      {
        xiiQtLogDockWidget::s_pWidget->Log("Reconnected to Server.");
      }

      if (sLastServerName != xiiTelemetry::GetServerName())
      {
        sLastServerName = xiiTelemetry::GetServerName();
        setWindowTitle(QString("xiiInspector - %1").arg(sLastServerName.GetData()));
      }

      bConnected = true;
    }
    else
    {
      if (bConnected)
      {
        xiiQtLogDockWidget::s_pWidget->Log("Lost Connection to Server.");
        setWindowTitle(QString("xiiInspector - disconnected"));
        sLastServerName.Clear();
      }

      bConnected = false;
    }
  }

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
    xiiQtDataWidget::s_pWidget->ResetStats();
    xiiQtResourceWidget::s_pWidget->ResetStats();
  }

  UpdateAlwaysOnTop();

  xiiQtMainWidget::s_pWidget->UpdateStats();
  xiiQtPluginsWidget::s_pWidget->UpdateStats();
  xiiQtSubsystemsWidget::s_pWidget->UpdateStats();
  xiiQtMemoryWidget::s_pWidget->UpdateStats();
  xiiQtTimeWidget::s_pWidget->UpdateStats();
  xiiQtFileWidget::s_pWidget->UpdateStats();
  xiiQtResourceWidget::s_pWidget->UpdateStats();
  // xiiQtDataWidget::s_pWidget->UpdateStats();

  for (xiiInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->UpdateStats();

  xiiTelemetry::PerFrameUpdate();
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
  ActionShowWindowData->setChecked(!xiiQtDataWidget::s_pWidget->isClosed());
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

  if (m_OnTopMode == Always || (m_OnTopMode == WhenConnected && xiiTelemetry::IsConnectedToServer()))
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
      setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint | Qt::WindowStaysOnBottomHint);

    show();
  }
}

void xiiQtMainWindow::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  xiiTelemetryMessage Msg;

  while (xiiTelemetry::RetrieveMessage(' APP', Msg) == XII_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case 'ASRT':
      {
        xiiString sSourceFile, sFunction, sExpression, sMessage;
        xiiUInt32 uiLine = 0;

        Msg.GetReader() >> sSourceFile;
        Msg.GetReader() >> uiLine;
        Msg.GetReader() >> sFunction;
        Msg.GetReader() >> sExpression;
        Msg.GetReader() >> sMessage;

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
      break;
    }
  }
}
