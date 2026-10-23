/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/EditorSplashScreen.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QWindow>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockManager.h>
#include <ads/DockWidget.h>
#include <ads/FloatingDockContainer.h>

struct xiiEditorSplashScreenTest
{
  static QProgressBar& Bar(xiiQtEditorSplashScreen& splash) { return splash.m_Progress; }
  static bool          LogoValid(xiiQtEditorSplashScreen& splash) { return splash.m_Logo.isValid(); }
  static void          Setup(xiiQtEditorApp& editor, xiiBitflags<xiiQtEditorApp::StartupFlags> flags)
  {
    editor.m_StartupFlags       = flags;
    const bool bStarting        = editor.m_bStartupInProgress;
    editor.m_bStartupInProgress = true;
    XII_SCOPE_EXIT(editor.m_bStartupInProgress = bStarting);
    editor.SetupAndShowSplashScreen();
  }
  static auto                     Flags(xiiQtEditorApp& editor) { return editor.m_StartupFlags; }
  static void                     RestoreFlags(xiiQtEditorApp& editor, xiiBitflags<xiiQtEditorApp::StartupFlags> flags) { editor.m_StartupFlags = flags; }
  static xiiQtEditorSplashScreen* Splash(xiiQtEditorApp& editor) { return editor.m_pSplashScreen; }
  static void                     Close(xiiQtEditorApp& editor) { editor.CloseSplashScreen(); }
  static void                     Attach(xiiQtEditorApp& editor) { editor.m_pSplashScreen = new xiiQtEditorSplashScreen; }
  static bool                     QuitRequested(xiiQtEditorApp& editor) { return editor.m_bStartupQuitRequested; }
  static void                     InteractiveWait(xiiQtEditorApp& editor, bool bEnable)
  {
    editor.m_bStartupInProgress    = bEnable;
    editor.m_bStartupQuitRequested = false;
    if (bEnable)
      qApp->installEventFilter(&editor);
    else
      qApp->removeEventFilter(&editor);
  }
};

class StartupWindowObserver : public QObject
{
public:
  StartupWindowObserver() { qApp->installEventFilter(this); }
  ~StartupWindowObserver() { qApp->removeEventFilter(this); }

  bool eventFilter(QObject* pObject, QEvent* pEvent) override
  {
    if ((m_bDismissDiagnostics || m_bIgnoreProjectQuestions) && pEvent->type() == QEvent::Show)
    {
      if (auto* pDialog = qobject_cast<QMessageBox*>(pObject))
      {
        ++m_uiDiagnosticShows;
        XII_TEST_BOOL(!xiiQtContainerWindow::GetContainerWindow()->isVisible());
        if (m_bIgnoreProjectQuestions)
        {
          XII_TEST_BOOL(pDialog->standardButtons().testFlag(QMessageBox::Ignore));
          QTimer::singleShot(0, pDialog, [pDialog]() { pDialog->done(QMessageBox::Ignore); });
        }
        else
          QTimer::singleShot(0, pDialog, &QDialog::accept);
      }
    }
    const bool bContainer = qobject_cast<xiiQtContainerWindow*>(pObject) != nullptr;
    const bool bFloating  = qobject_cast<ads::CFloatingDockContainer*>(pObject) != nullptr;
    if ((bContainer || bFloating) && (pEvent->type() == QEvent::Show || pEvent->type() == QEvent::WindowActivate))
    {
      if (!m_bAllowPresentation)
        xiiTestFramework::Output(xiiTestOutput::Details, "Premature %s event %d, visible=%d", pObject->metaObject()->className(), static_cast<int>(pEvent->type()), static_cast<QWidget*>(pObject)->isVisible());
      XII_TEST_BOOL(m_bAllowPresentation);
      XII_TEST_BOOL(!xiiQtEditorApp::GetSingleton()->IsStartingUp());
      if (pEvent->type() == QEvent::Show)
      {
        if (bContainer)
          ++m_uiContainerShows;
        else
          ++m_uiFloatingShows;
      }
    }
    return false;
  }

  bool      m_bAllowPresentation      = false;
  bool      m_bDismissDiagnostics     = false;
  bool      m_bIgnoreProjectQuestions = false;
  xiiUInt32 m_uiDiagnosticShows       = 0;
  xiiUInt32 m_uiContainerShows        = 0;
  xiiUInt32 m_uiFloatingShows         = 0;
};

XII_CREATE_SIMPLE_TEST_GROUP(EditorSplash);

XII_CREATE_SIMPLE_TEST(EditorSplash, AppearanceAndScaling)
{
  const QString sVersion = QCoreApplication::applicationVersion();
  QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
  XII_SCOPE_EXIT(QCoreApplication::setApplicationVersion(sVersion));
  const QPalette previous = qApp->palette();
  XII_SCOPE_EXIT(qApp->setPalette(previous));
  const QString sOutput = QString::fromUtf8(xiiTestFramework::GetInstance()->GetAbsOutputPath()) + QStringLiteral("/SplashPreviews");
  XII_TEST_BOOL(QDir().mkpath(sOutput));
  for (bool bDark : {false, true})
  {
    QPalette palette = previous;
    palette.setColor(QPalette::Window, bDark ? QColor("#202020") : Qt::white);
    palette.setColor(QPalette::WindowText, bDark ? Qt::white : Qt::black);
    qApp->setPalette(palette);
    xiiQtEditorSplashScreen splash;
    splash.SetStage(QStringLiteral("Loading project plugins"), QStringLiteral("xiiEditorPluginScene"), 3, 8);
    splash.show();
    QApplication::processEvents();
    XII_TEST_BOOL(xiiEditorSplashScreenTest::LogoValid(splash));
    XII_TEST_BOOL((splash.windowFlags() & Qt::WindowType_Mask) == Qt::SplashScreen);
    XII_TEST_BOOL(!splash.windowFlags().testFlag(Qt::WindowStaysOnTopHint));
    XII_TEST_BOOL(splash.testAttribute(Qt::WA_ShowWithoutActivating));
    XII_TEST_BOOL(!splash.testAttribute(Qt::WA_QuitOnClose));
    XII_TEST_BOOL(splash.parentWidget() == nullptr);
    XII_TEST_BOOL(splash.screen()->availableGeometry().contains(splash.geometry()));
    for (qreal fScale : {1.0, 1.5, 2.0, 3.0})
    {
      QImage image(QSize(qRound(splash.width() * fScale), qRound(splash.height() * fScale)), QImage::Format_ARGB32_Premultiplied);
      image.setDevicePixelRatio(fScale);
      image.fill(Qt::transparent);
      splash.render(&image);
      XII_TEST_BOOL(image.pixelColor(qRound(8 * fScale), qRound(8 * fScale)) == Qt::black);
      const QString sName = QStringLiteral("/%1-%2x.png").arg(bDark ? QStringLiteral("dark") : QStringLiteral("light")).arg(fScale);
      XII_TEST_BOOL(image.save(sOutput + sName));
    }
  }
}

XII_CREATE_SIMPLE_TEST(EditorSplash, ActualWorkCounts)
{
  xiiQtEditorSplashScreen splash;
  auto&                   progress = xiiEditorSplashScreenTest::Bar(splash);
  splash.SetStage(QStringLiteral("Initializing subsystems"));
  XII_TEST_INT(progress.maximum(), 0);
  XII_TEST_INT(progress.minimum(), 0);
  splash.SetStage(QStringLiteral("Loading plugins"), QString::fromUtf8("D:/Projects/日本語 and spaces"), 2, 7);
  XII_TEST_INT(progress.maximum(), 7);
  XII_TEST_INT(progress.value(), 2);
  XII_TEST_BOOL(!progress.isTextVisible()); // Counts measure this stage; there is no invented overall percentage.
  XII_TEST_BOOL(splash.accessibleDescription().contains(QString::fromUtf8("日本語")));
  splash.SetStage(QStringLiteral("Loading plugins"), {}, 99, 7);
  XII_TEST_INT(progress.value(), 7);
  splash.SetStage(QStringLiteral("Initializing rendering"));
  XII_TEST_INT(progress.maximum(), 0);
  splash.SetStage(QStringLiteral("Long detail"), QString(2048, QChar(0x4e2d)));
  XII_TEST_BOOL(splash.accessibleDescription().contains(QString(2048, QChar(0x4e2d))));
}

XII_CREATE_SIMPLE_TEST(EditorSplash, ModalAndDismissal)
{
  QWidget editor;
  editor.show();
  xiiQtEditorSplashScreen splash;
  splash.SetEditorWindow(&editor);
  splash.show();
  XII_TEST_BOOL(splash.windowHandle()->transientParent() == nullptr);
  QDialog accidentalParent(&splash);
  accidentalParent.ensurePolished();
  XII_TEST_BOOL(accidentalParent.parentWidget() == &editor);
  QDialog dialog(&editor);
  dialog.setModal(true);
  dialog.show();
  QApplication::processEvents();
  XII_TEST_BOOL(!splash.isVisible());
  XII_TEST_BOOL(dialog.parentWidget() == &editor);
  dialog.close();
  splash.show();
  QTimer::singleShot(0, qApp, [&]() {
    auto* pModal = qobject_cast<QDialog*>(QApplication::activeModalWidget());
    XII_TEST_BOOL(pModal != nullptr);
    if (pModal)
    {
      XII_TEST_BOOL(pModal->parentWidget() != &splash);
      pModal->accept();
    }
  });
  xiiQtUiServices::MessageBoxWarning("Startup diagnostic handoff test.");
  XII_TEST_BOOL(splash.isVisible());
  splash.show();
  QMouseEvent click(QEvent::MouseButtonPress, QPointF(10, 10), QPointF(10, 10), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(&splash, &click);
  XII_TEST_BOOL(!splash.isVisible());
  splash.SetStage(QStringLiteral("Continuing initialization"));
  XII_TEST_BOOL(!splash.isVisible());
  QDialog laterDialog(&editor);
  laterDialog.show();
  laterDialog.accept();
  XII_TEST_BOOL(!splash.isVisible());
  XII_TEST_BOOL(editor.isVisible());
}

XII_CREATE_SIMPLE_TEST(EditorSplash, StartupPolicy)
{
  const QString sOrganization = QCoreApplication::organizationName();
  const QString sApplication  = QCoreApplication::applicationName();
  QCoreApplication::setOrganizationName(QStringLiteral("XII Startup Tests"));
  QCoreApplication::setApplicationName(QStringLiteral("SplashPolicy"));
  XII_SCOPE_EXIT(QCoreApplication::setOrganizationName(sOrganization); QCoreApplication::setApplicationName(sApplication));
  auto&      editor = *xiiQtEditorApp::GetSingleton();
  const auto flags  = xiiEditorSplashScreenTest::Flags(editor);
  XII_SCOPE_EXIT(xiiEditorSplashScreenTest::Close(editor); xiiEditorSplashScreenTest::RestoreFlags(editor, flags));
  for (auto flag : {xiiQtEditorApp::StartupFlags::UnitTest, xiiQtEditorApp::StartupFlags::Headless, xiiQtEditorApp::StartupFlags::Background})
  {
    xiiEditorSplashScreenTest::Setup(editor, flag);
    XII_TEST_BOOL(xiiEditorSplashScreenTest::Splash(editor) == nullptr);
  }
  auto& command   = *xiiCommandLineUtils::GetGlobalInstance();
  auto  arguments = command.GetCommandLineArray();
  XII_SCOPE_EXIT(command.SetCommandLine(arguments));
  xiiDynamicArray<xiiString> noSplash;
  noSplash.PushBack("xiiEditor");
  noSplash.PushBack("-NoSplash");
  command.SetCommandLine(noSplash);
  xiiEditorSplashScreenTest::Setup(editor, {});
  XII_TEST_BOOL(xiiEditorSplashScreenTest::Splash(editor) == nullptr);
  noSplash.PopBack();
  command.SetCommandLine(noSplash);
  QSettings      settings;
  const QString  sKey        = QStringLiteral("EditorPreferences/ShowSplashScreen");
  const bool     bHadSetting = settings.contains(sKey);
  const QVariant previous    = settings.value(sKey);
  XII_SCOPE_EXIT(if (bHadSetting) settings.setValue(sKey, previous); else settings.remove(sKey));
  settings.setValue(sKey, false);
  xiiEditorSplashScreenTest::Setup(editor, {});
  XII_TEST_BOOL(xiiEditorSplashScreenTest::Splash(editor) == nullptr);
  settings.setValue(sKey, true);
  xiiEditorSplashScreenTest::Setup(editor, {});
  XII_TEST_BOOL(xiiEditorSplashScreenTest::Splash(editor) != nullptr);
  xiiEditorSplashScreenTest::Close(editor);
}

XII_CREATE_SIMPLE_TEST(EditorSplash, StartupAndQuit)
{
  StartupWindowObserver observer;
  auto&                 editor       = *xiiQtEditorApp::GetSingleton();
  auto*                 pApplication = xiiApplication::GetApplicationInstance();
  const xiiString       sName        = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(sName));
  // Reuse this preference directory on the next process launch to exercise warm startup.
  xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/StartupPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  QPointer<xiiQtEditorSplashScreen>                splash;
  xiiEvent<const xiiEditorAppEvent&>::Unsubscriber subscription;
  xiiQtEditorApp::m_Events.AddEventHandler([&](const xiiEditorAppEvent& e) {
    if (e.m_Type != xiiEditorAppEvent::Type::EditorStarted)
      return;
    XII_TEST_BOOL(editor.IsStartingUp());
    XII_TEST_BOOL(!xiiEditorSplashScreenTest::QuitRequested(editor));
    xiiEditorSplashScreenTest::Attach(editor);
    splash = xiiEditorSplashScreenTest::Splash(editor);
    splash->show();
    editor.UpdateStartupProgress(QStringLiteral("Ready to open selected project"));
    auto*       pWindow = xiiQtContainerWindow::GetContainerWindow();
    QCloseEvent close;
    QApplication::sendEvent(pWindow, &close);
    XII_TEST_BOOL(!close.isAccepted());
    QEvent quit(QEvent::Quit);
    QApplication::sendEvent(qApp, &quit);
    XII_TEST_BOOL(xiiEditorSplashScreenTest::QuitRequested(editor));
  },
                                           subscription);
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::SafeMode | xiiQtEditorApp::StartupFlags::NoRecent, preferences);
  XII_TEST_BOOL(!editor.IsStartingUp());
  XII_TEST_BOOL(splash.isNull());
  XII_TEST_INT(observer.m_uiContainerShows, 0);
  XII_TEST_INT(editor.RunEditor(), 0); // A quit before exec must not be lost.
  QApplication::processEvents();
  xiiActionManager::SaveShortcutAssignment();
  xiiStringBuilder shortcuts(preferences, "/Settings/Shortcuts.ddl");
  XII_TEST_BOOL(xiiOSFile::ExistsFile(shortcuts));
  editor.ShutdownEditor();
}
XII_CREATE_SIMPLE_TEST(EditorSplash, ResponsiveDisconnectedProcess)
{
  xiiEditorProcessCommunicationChannel channel;
  bool                                 bProcessedEvents = false;
  const QStringList                    arguments        = {QStringLiteral("-noGui"), QStringLiteral("-filter"), QStringLiteral("NoStartupTestChildTests"), QStringLiteral("-NoSplash")};
  const xiiString                      exe              = xiiOSFile::GetApplicationPath();
  XII_TEST_RESULT(channel.StartClientProcess(exe.GetData(), arguments, false));
  XII_SCOPE_EXIT(channel.CloseConnection());
  QTimer::singleShot(0, qApp, [&]() { bProcessedEvents = true; });
  XII_TEST_BOOL(channel.WaitForStartup().Failed()); // This short-lived child does not connect to the IPC server.
  XII_TEST_BOOL(bProcessedEvents);
}

XII_CREATE_SIMPLE_TEST(EditorSplash, QuitWhileWaitingForProcess)
{
  auto& editor = *xiiQtEditorApp::GetSingleton();
  xiiEditorSplashScreenTest::InteractiveWait(editor, true);
  XII_SCOPE_EXIT(xiiEditorSplashScreenTest::InteractiveWait(editor, false));
  xiiEditorProcessCommunicationChannel channel;
  const xiiString                      exe = xiiOSFile::GetApplicationPath();
  XII_TEST_RESULT(channel.StartClientProcess(exe.GetData(), {QStringLiteral("-noGui"), QStringLiteral("-filter"), QStringLiteral("NoStartupTestChildTests")}, false));
  XII_SCOPE_EXIT(channel.CloseConnection());
  QTimer::singleShot(0, qApp, []() {
    QEvent quit(QEvent::Quit);
    QApplication::sendEvent(qApp, &quit);
  });
  XII_TEST_BOOL(channel.WaitForStartup().Failed());
  XII_TEST_BOOL(xiiEditorSplashScreenTest::QuitRequested(editor));
}

XII_CREATE_SIMPLE_TEST(EditorSplash, StartupIpcResponse)
{
  auto& editor = *xiiQtEditorApp::GetSingleton();
  xiiEditorSplashScreenTest::InteractiveWait(editor, true);
  XII_SCOPE_EXIT(xiiEditorSplashScreenTest::InteractiveWait(editor, false));
  xiiEditorProcessCommunicationChannel channel;
  const xiiString                      exe = xiiOSFile::GetApplicationPath();
  XII_TEST_RESULT(channel.StartClientProcess(exe.GetData(), {QStringLiteral("-splashIpcTestPeer")}, false));
  XII_SCOPE_EXIT(channel.CloseConnection());
  XII_TEST_RESULT(channel.WaitForStartup());
  // Includes a response already queued before the GUI starts waiting, avoiding a lost wakeup.
  XII_TEST_RESULT(channel.WaitForStartup(xiiGetStaticRTTI<xiiProjectReadyMsgToEditor>()));
}

XII_CREATE_SIMPLE_TEST_GROUP(EditorStartupVisibility);

XII_CREATE_SIMPLE_TEST(EditorStartupVisibility, DeferredWorkspace)
{
  StartupWindowObserver observer;
  xiiQtContainerWindow  window;
  window.SetWindowPresentationEnabled(false);
  const QPointer<QWindow> focusWindow = QGuiApplication::focusWindow();
  QWidget*                pBase       = &window;
  pBase->show();
  QMetaObject::invokeMethod(&window, "show", Qt::QueuedConnection);
  QMetaObject::invokeMethod(&window, "SlotRestoreLayout", Qt::QueuedConnection);
  QApplication::processEvents();
  XII_TEST_BOOL(!window.isVisible());
  XII_TEST_BOOL(QGuiApplication::focusWindow() == focusWindow);
  XII_TEST_INT(observer.m_uiContainerShows, 0);

  auto* pManager = window.GetDockManager();
  auto* pDock    = new ads::CDockWidget(pManager, QStringLiteral("Startup floating fixture"));
  pDock->setObjectName(QStringLiteral("StartupFloatingFixture"));
  pDock->setWidget(new QLabel(QStringLiteral("Restored tool panel")));
  auto* pFloating = pManager->addDockWidgetFloating(pDock);
  pFloating->show();
  XII_TEST_BOOL(!pFloating->isVisible());
  const QByteArray state = pManager->saveState(1);
  XII_TEST_BOOL(pManager->restoreState(state, 1));
  QApplication::processEvents();
  for (auto* pWindow : pManager->floatingWidgets())
    XII_TEST_BOOL(!pWindow->isVisible());
  XII_TEST_INT(observer.m_uiFloatingShows, 0);

  // Pending floating entries must not survive their Qt-owned window's destruction.
  auto* pDiscarded = new ads::CFloatingDockContainer(pManager);
  pDiscarded->show();
  delete pDiscarded;
  window.setWindowState(Qt::WindowNoState);
  observer.m_bAllowPresentation = true;
  window.SetWindowPresentationEnabled(true);
  window.show();
  QApplication::processEvents();
  XII_TEST_INT(observer.m_uiContainerShows, 1);
  XII_TEST_INT(observer.m_uiFloatingShows, 1);
  XII_TEST_BOOL(pManager->floatingWidgets().front()->isVisible());
  QApplication::processEvents();
  XII_TEST_INT(observer.m_uiContainerShows, 1);
}

static void RunStartupVisibility(bool bOpenMissingProject, bool bOpenProject = false, bool bInteractive = false)
{
  StartupWindowObserver observer;
  observer.m_bAllowPresentation      = true;
  observer.m_bDismissDiagnostics     = bOpenMissingProject;
  observer.m_bIgnoreProjectQuestions = bInteractive;
  auto&           editor             = *xiiQtEditorApp::GetSingleton();
  auto*           pApplication       = xiiApplication::GetApplicationInstance();
  const xiiString sName              = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(sName));
  xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/StartupVisibilityPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  auto& command   = *xiiCommandLineUtils::GetGlobalInstance();
  auto  arguments = command.GetCommandLineArray();
  XII_SCOPE_EXIT(command.SetCommandLine(arguments));
  const xiiStringBuilder project(preferences, bOpenMissingProject ? "/MissingProject/xiiProject" : "/StartupProject/xiiProject");
  const bool             bRestoreRecentProject = bInteractive && xiiOSFile::ExistsFile(xiiStringBuilder(preferences, "/Settings/RecentProjects.txt"));
  if (bInteractive && !xiiOSFile::ExistsFile(project))
  {
    // Prepare an existing empty project through ToolsFoundation. A default scene would
    // exercise asset transformation/rendering rather than the startup IPC handshake.
    XII_TEST_BOOL(xiiToolsProject::CreateProject(project).Succeeded());
    xiiToolsProject::CloseProject();
  }
  if (bOpenMissingProject || bOpenProject)
  {
    xiiDynamicArray<xiiString> projectArguments;
    projectArguments.PushBack("xiiEditor");
    if (!bRestoreRecentProject)
    {
      projectArguments.PushBack(bOpenProject && !xiiOSFile::ExistsFile(project) ? "-newproject" : "-project");
      projectArguments.PushBack(project);
    }
    command.SetCommandLine(projectArguments);
  }
  QPointer<xiiQtEditorSplashScreen>                splash;
  xiiEvent<const xiiEditorAppEvent&>::Unsubscriber subscription;
  xiiQtEditorApp::m_Events.AddEventHandler([&](const xiiEditorAppEvent& e) {
    if (e.m_Type != xiiEditorAppEvent::Type::EditorStarted)
      return;
    auto* pWindow = xiiQtContainerWindow::GetContainerWindow();
    XII_TEST_BOOL(!pWindow->isVisible());
    XII_TEST_BOOL(!pWindow->IsWindowPresentationEnabled());
    if (!xiiEditorSplashScreenTest::Splash(editor))
      xiiEditorSplashScreenTest::Attach(editor);
    splash = xiiEditorSplashScreenTest::Splash(editor);
    splash->SetEditorWindow(pWindow);
    splash->show();
    xiiQtEditorApp::ShowEditorWindow(pWindow); // Tray/notification activation cannot bypass startup.
    pWindow->ScheduleRestoreWindowLayout();
    pWindow->ScheduleRestoreWindowLayout();
    QMetaObject::invokeMethod(pWindow, "show", Qt::QueuedConnection);
    QMetaObject::invokeMethod(pWindow, "SlotRestoreLayout", Qt::QueuedConnection);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    XII_TEST_BOOL(!pWindow->isVisible());
    XII_TEST_BOOL(splash->isVisible());
    XII_TEST_INT(observer.m_uiContainerShows, 0);

    // A real saved ADS workspace with a floating tool must be restored before presentation.
    auto* pDock = new ads::CDockWidget(pWindow->GetDockManager(), QStringLiteral("Startup tool"));
    pDock->setObjectName(QStringLiteral("StartupTool"));
    pDock->setWidget(new QLabel(QStringLiteral("Session restoration fixture")));
    pWindow->GetDockManager()->addDockWidgetFloating(pDock);
    const QString sLayout = QString::fromUtf8(preferences.GetData()) + QStringLiteral("/layout.settings");
    if (!QFile::exists(sLayout))
    {
      QWidget geometry;
      geometry.setGeometry(60, 60, 1100, 700);
      geometry.setWindowState(Qt::WindowMaximized);
      QSettings settings(sLayout, QSettings::IniFormat);
      settings.beginGroup(QStringLiteral("ContainerWnd_xiiEditor"));
      settings.setValue(QStringLiteral("WindowGeometry"), geometry.saveGeometry());
      settings.setValue(QStringLiteral("WindowState"), pWindow->saveState());
      settings.setValue(QStringLiteral("DockManagerState"), pWindow->GetDockManager()->saveState(1));
    }
  },
                                           subscription);
  class StartupLog : public xiiTestLogInterface
  {
  public:
    void HandleLogMessage(const xiiLoggingEventData& e) override
    {
      if (e.m_EventType == xiiLogMsgType::WarningMsg && e.m_sText.FindSubString("Ignored send message of type xiiSyncWithProcessMsgToEngine"))
        ++m_uiRejectedSyncMessages;
      xiiTestLogInterface::HandleLogMessage(e);
    }
    xiiUInt32 m_uiRejectedSyncMessages = 0;
  } log;
  xiiTestLogSystemScope                                                  logScope(&log);
  xiiUInt32                                                              uiStartupTicks = 0;
  xiiEvent<const xiiEditorEngineProcessConnection::Event&>::Unsubscriber engineSubscription;
  if (bInteractive)
  {
    xiiEditorEngineProcessConnection::s_Events.AddEventHandler([&](const xiiEditorEngineProcessConnection::Event& e) {
      if (e.m_Type != xiiEditorEngineProcessConnection::Event::Type::ProcessStarted)
        return;
      XII_TEST_BOOL(editor.IsStartingUp());
      XII_TEST_BOOL(!editor.IsInUnitTestMode());
      XII_TEST_BOOL(!xiiEditorEngineProcessConnection::GetSingleton()->IsEngineSetup());
      // Exercise a redraw tick before setup deterministically, as well as real queued ticks
      // during the interactive connection/project-ready waits. Never wait on a rejected sync.
      xiiQtUiServices::TickEvent tick;
      tick.m_Type = xiiQtUiServices::TickEvent::Type::EndFrame;
      xiiQtUiServices::s_TickEvent.Broadcast(tick);
      ++uiStartupTicks;
    },
                                                               engineSubscription);
  }
  editor.StartupEditor(bInteractive ? xiiBitflags<xiiQtEditorApp::StartupFlags>(xiiQtEditorApp::StartupFlags::Default) : xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent, preferences);
  XII_SCOPE_EXIT(editor.ShutdownEditor());
  XII_TEST_INT(log.m_uiRejectedSyncMessages, 0);
  auto* pWindow = xiiQtContainerWindow::GetContainerWindow();
  XII_TEST_BOOL(!editor.IsStartingUp());
  XII_TEST_BOOL(splash.isNull());
  XII_TEST_BOOL(pWindow->isVisible());
  XII_TEST_BOOL(pWindow->isMaximized());
  if (bOpenProject)
  {
    XII_TEST_BOOL(xiiToolsProject::IsProjectOpen());
    XII_TEST_BOOL(xiiEditorEngineProcessConnection::GetSingleton()->IsEngineSetup());
    XII_TEST_STRING(xiiToolsProject::GetSingleton()->GetProjectFile(), project);
    if (bInteractive)
    {
      XII_TEST_INT(uiStartupTicks, 1);
      auto&                                                                channel       = xiiEditorEngineProcessConnection::GetSingleton()->GetCommunicationChannel();
      bool                                                                 bSynchronized = false;
      xiiEvent<const xiiProcessCommunicationChannel::Event&>::Unsubscriber syncSubscription;
      channel.m_Events.AddEventHandler([&](const xiiProcessCommunicationChannel::Event& e) {
        if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiSyncWithProcessMsgToEditor>())
          bSynchronized = true;
      },
                                       syncSubscription);
      xiiQtUiServices::TickEvent tick;
      tick.m_Type = xiiQtUiServices::TickEvent::Type::EndFrame;
      xiiQtUiServices::s_TickEvent.Broadcast(tick);
      if (!bSynchronized)
        XII_TEST_RESULT(channel.WaitForMessage(xiiGetStaticRTTI<xiiSyncWithProcessMsgToEditor>(), xiiTime::MakeFromSeconds(5)));
      XII_TEST_BOOL(bSynchronized);
    }
  }
  XII_TEST_INT(observer.m_uiContainerShows, 1);
  XII_TEST_INT(observer.m_uiFloatingShows, 1);
  if (!bInteractive)
    XII_TEST_INT(observer.m_uiDiagnosticShows, bOpenMissingProject ? 1 : 0);
  QApplication::processEvents();
  XII_TEST_INT(observer.m_uiContainerShows, 1);
  pWindow->SaveWindowLayout();
  xiiActionManager::SaveShortcutAssignment();
}

XII_CREATE_SIMPLE_TEST(EditorStartupVisibility, StartupRevealsOnce)
{
  RunStartupVisibility(false);
}

XII_CREATE_SIMPLE_TEST_GROUP(EditorStartupFailureVisibility);

XII_CREATE_SIMPLE_TEST(EditorStartupFailureVisibility, MissingProject)
{
  RunStartupVisibility(true);
}

XII_CREATE_SIMPLE_TEST_GROUP(EditorProjectStartupVisibility);

XII_CREATE_SIMPLE_TEST(EditorProjectStartupVisibility, ProjectLoading)
{
  RunStartupVisibility(false, true);
}

XII_CREATE_SIMPLE_TEST_GROUP(EditorInteractiveProjectStartup);

XII_CREATE_SIMPLE_TEST(EditorInteractiveProjectStartup, ProjectLoading)
{
  // First run opens an existing project. A second fresh process with the same output directory
  // reopens it from the real recent-project history, without explicit project arguments.
  RunStartupVisibility(false, true, true);
}
