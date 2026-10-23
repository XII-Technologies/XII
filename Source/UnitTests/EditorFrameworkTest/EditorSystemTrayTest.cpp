/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/GUI/EditorSystemTray.h>

#if QT_CONFIG(systemtrayicon)

#  include <EditorFramework/Assets/AssetProcessor.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <Foundation/Application/Application.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Types/ScopeExit.h>
#  include <QAction>
#  include <QApplication>
#  include <QCloseEvent>
#  include <QDialog>
#  include <QDir>
#  include <QFile>
#  include <QIcon>
#  include <QTemporaryDir>
#  include <QTimer>
#  include <atomic>
#  include <thread>

struct xiiEditorSystemTrayTest
{
  static QSystemTrayIcon&       Icon(xiiQtEditorSystemTray& tray) { return tray.m_Icon; }
  static QMenu&                 Menu(xiiQtEditorSystemTray& tray) { return tray.m_Menu; }
  static QMenu&                 Recent(xiiQtEditorSystemTray& tray) { return *tray.m_pRecentProjects; }
  static QAction&               Exit(xiiQtEditorSystemTray& tray) { return *tray.m_pExit; }
  static QAction&               Open(xiiQtEditorSystemTray& tray) { return *tray.m_pOpenProject; }
  static void                   Refresh(xiiQtEditorSystemTray& tray) { tray.RefreshMenu(); }
  static xiiQtEditorSystemTray& Attach(xiiQtEditorApp& editor)
  {
    editor.m_pSystemTray = XII_DEFAULT_NEW(xiiQtEditorSystemTray, editor, *xiiQtContainerWindow::GetContainerWindow(), *editor.m_pProgressbar);
    return *editor.m_pSystemTray;
  }
};

namespace
{
  class TrayTestWindow : public QWidget
  {
  public:
    void closeEvent(QCloseEvent* pEvent) override
    {
      ++m_uiCloseRequests;
      pEvent->setAccepted(m_bAcceptClose);
    }
    xiiUInt32 m_uiCloseRequests = 0;
    bool      m_bAcceptClose    = false;
  };

  QString MakeProject(QTemporaryDir& root, const QString& sName)
  {
    const QString sDirectory = root.path() + QLatin1Char('/') + sName;
    XII_TEST_BOOL(QDir().mkpath(sDirectory));
    const QString sProject = sDirectory + QStringLiteral("/xiiProject");
    QFile         file(sProject);
    XII_TEST_BOOL(file.open(QIODevice::WriteOnly));
    return sProject;
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorSystemTray);

XII_CREATE_SIMPLE_TEST(EditorSystemTray, StatusAndOperations)
{
  auto&                 editor = *xiiQtEditorApp::GetSingleton();
  TrayTestWindow        window;
  xiiProgress           progress;
  xiiQtEditorSystemTray tray(editor, window, progress);
  auto&                 icon = xiiEditorSystemTrayTest::Icon(tray);
  QApplication::processEvents();
  XII_TEST_BOOL(icon.toolTip() == QStringLiteral("XII Editor"));
  Q_EMIT editor.ToolExecutionStarted(QStringLiteral("Compiling shaders"));
  Q_EMIT editor.ToolExecutionStarted(QStringLiteral("Building C++ project"));
  Q_EMIT editor.ToolExecutionStarted(QStringLiteral("Compiling shaders"));
  XII_TEST_BOOL(icon.toolTip().contains(QStringLiteral("Compiling shaders (2)")));
  XII_TEST_BOOL(icon.toolTip().contains(QStringLiteral("Building C++ project")));
  Q_EMIT editor.ToolExecutionFinished(true, QStringLiteral("Compiling shaders"));
  XII_TEST_BOOL(!icon.toolTip().contains(QStringLiteral("(2)")));
  Q_EMIT editor.ToolExecutionFinished(false, QStringLiteral("Compiling shaders"));
  XII_TEST_BOOL(!icon.toolTip().contains(QStringLiteral("Compiling shaders")));
  XII_TEST_BOOL(icon.toolTip().contains(QStringLiteral("Building C++ project")));
  {
    xiiProgressRange range("Importing Asset", true, &progress);
    range.SetCompletion(0.5);
    XII_TEST_BOOL(icon.toolTip().contains(QStringLiteral("Importing Asset")));
    progress.UserClickedCancel();
    XII_TEST_BOOL(icon.toolTip().contains(QStringLiteral("Cancelling: Importing Asset")));
    xiiEditorSystemTrayTest::Refresh(tray);
    XII_TEST_BOOL(!xiiEditorSystemTrayTest::Exit(tray).isEnabled());
  }
  XII_TEST_BOOL(!icon.toolTip().contains(QStringLiteral("Importing Asset")));
  Q_EMIT editor.ToolExecutionFinished(true, QStringLiteral("Building C++ project"));
  XII_TEST_BOOL(icon.toolTip() == QStringLiteral("XII Editor"));
  Q_EMIT editor.ToolExecutionStarted();
  XII_TEST_BOOL(icon.toolTip().contains(QStringLiteral("External tool")));
  Q_EMIT editor.ToolExecutionFinished(false);
  XII_TEST_BOOL(icon.toolTip() == QStringLiteral("XII Editor"));

  QString sLongName(114, QLatin1Char('x'));
  sLongName += QString::fromUtf8("😀 followed by more text");
  Q_EMIT editor.ToolExecutionStarted(sLongName);
  XII_TEST_BOOL(icon.toolTip().size() <= 127);
  XII_TEST_BOOL(icon.toolTip().endsWith(QChar(0x2026)));
  const QString sPrefix = icon.toolTip().chopped(1);
  XII_TEST_BOOL(!sPrefix.back().isHighSurrogate());
  Q_EMIT editor.ToolExecutionFinished(true, sLongName);

  QTemporaryDir    directory;
  const QByteArray missingShader = (directory.path() + QStringLiteral("/missing/xiiShaderCompilerTool")).toUtf8();
  bool             bSawShader    = false;
  bool             bSawFinish    = false;
  QObject::connect(&editor, &xiiQtEditorApp::ToolExecutionStarted, &tray, [&](const QString& sName) {
    bSawShader = sName == QStringLiteral("Compiling shaders") && icon.toolTip().contains(sName);
  });
  QObject::connect(&editor, &xiiQtEditorApp::ToolExecutionFinished, &tray, [&](bool bSuccess, const QString& sName) {
    bSawFinish = !bSuccess && sName == QStringLiteral("Compiling shaders");
  });
  XII_TEST_BOOL(editor.ExecuteTool(missingShader.constData(), {}, 2).Failed());
  XII_TEST_BOOL(bSawShader && bSawFinish);
  XII_TEST_BOOL(icon.toolTip() == QStringLiteral("XII Editor"));
}

XII_CREATE_SIMPLE_TEST(EditorSystemTray, RecentProjects)
{
  auto&      editor          = *xiiQtEditorApp::GetSingleton();
  const auto previousHistory = editor.GetRecentProjectsList();
  XII_SCOPE_EXIT(editor.GetRecentProjectsList() = previousHistory);
  auto& recent = editor.GetRecentProjectsList();
  recent.Clear();
  QTemporaryDir directory;
  const QString first  = MakeProject(directory, QString::fromUtf8("Lumière 日本語 & Space"));
  const QString second = MakeProject(directory, QStringLiteral("Other Project"));
  recent.Insert(second.toUtf8().constData(), 0);
  recent.Insert(first.toUtf8().constData(), 0);
  recent.Insert(directory.path().toUtf8().constData(), 0);
  recent.Insert((directory.path() + QStringLiteral("/missing/xiiProject")).toUtf8().constData(), 0);
  QWidget               window;
  xiiProgress           progress;
  xiiQtEditorSystemTray tray(editor, window, progress);
  auto&                 menu = xiiEditorSystemTrayTest::Recent(tray);
  XII_TEST_INT(menu.actions().size(), 2);
  XII_TEST_BOOL(menu.actions()[0]->data().toString() == first);
  XII_TEST_BOOL(menu.actions()[0]->text().contains(QStringLiteral("&&")));
  XII_TEST_BOOL(menu.actions()[1]->data().toString() == second);
  XII_TEST_BOOL(menu.isEnabled());
  recent.Remove(first.toUtf8().constData());
  Q_EMIT xiiEditorSystemTrayTest::Menu(tray).aboutToShow();
  XII_TEST_INT(menu.actions().size(), 1);
  XII_TEST_BOOL(menu.actions()[0]->data().toString() == second);
  recent.Clear();
  Q_EMIT xiiEditorSystemTrayTest::Menu(tray).aboutToShow();
  XII_TEST_BOOL(!menu.isEnabled());
}

XII_CREATE_SIMPLE_TEST(EditorSystemTray, ActivationAndClose)
{
  const bool            bQuitOnLastWindow = QApplication::quitOnLastWindowClosed();
  TrayTestWindow        window;
  xiiProgress           progress;
  xiiQtEditorSystemTray tray(*xiiQtEditorApp::GetSingleton(), window, progress);
  auto&                 icon = xiiEditorSystemTrayTest::Icon(tray);
  XII_TEST_BOOL(icon.isVisible());
  XII_TEST_BOOL(!icon.icon().isNull());
  XII_TEST_BOOL(QApplication::quitOnLastWindowClosed() == bQuitOnLastWindow);
  Q_EMIT icon.activated(QSystemTrayIcon::Context);
  XII_TEST_BOOL(!window.isVisible());
  window.showMaximized();
  window.showMinimized();
  Q_EMIT icon.activated(QSystemTrayIcon::Trigger);
  XII_TEST_BOOL(window.isVisible());
  XII_TEST_BOOL(!window.isMinimized());
  XII_TEST_BOOL(window.isMaximized());
  window.hide();
  Q_EMIT icon.activated(QSystemTrayIcon::DoubleClick);
  XII_TEST_BOOL(window.isVisible());
  xiiEditorSystemTrayTest::Exit(tray).trigger();
  QApplication::processEvents();
  XII_TEST_INT(window.m_uiCloseRequests, 1);
  XII_TEST_BOOL(window.isVisible());
  XII_TEST_BOOL(icon.isVisible());
  {
    QDialog modal(&window);
    modal.setWindowModality(Qt::ApplicationModal);
    modal.show();
    QApplication::processEvents();
    xiiEditorSystemTrayTest::Refresh(tray);
    XII_TEST_BOOL(!xiiEditorSystemTrayTest::Open(tray).isEnabled());
    XII_TEST_BOOL(!xiiEditorSystemTrayTest::Exit(tray).isEnabled());
    modal.hide();
  }
  xiiEditorSystemTrayTest::Refresh(tray);
  XII_TEST_BOOL(xiiEditorSystemTrayTest::Exit(tray).isEnabled());
  // Recheck when the queued command runs, even if it was enabled at selection time.
  xiiEditorSystemTrayTest::Exit(tray).trigger();
  {
    xiiProgressRange range("Import in progress", false, &progress);
    QApplication::processEvents();
    XII_TEST_INT(window.m_uiCloseRequests, 1);
  }
  window.m_bAcceptClose = true;
  xiiEditorSystemTrayTest::Refresh(tray);
  xiiEditorSystemTrayTest::Exit(tray).trigger();
  QApplication::processEvents();
  XII_TEST_INT(window.m_uiCloseRequests, 2);
  XII_TEST_BOOL(!window.isVisible());
}

XII_CREATE_SIMPLE_TEST(EditorSystemTray, BackgroundAndQueuedShutdown)
{
  auto&                     editor     = *xiiQtEditorApp::GetSingleton();
  auto*                     pProcessor = xiiAssetProcessor::GetSingleton();
  QWidget                   window;
  xiiProgress               progress;
  QPointer<QSystemTrayIcon> icon;
  QPointer<QMenu>           menu;
  std::atomic<bool>         keepBroadcasting = true;
  std::thread               broadcaster;
  {
    xiiQtEditorSystemTray tray(editor, window, progress);
    icon = &xiiEditorSystemTrayTest::Icon(tray);
    menu = &xiiEditorSystemTrayTest::Menu(tray);
    QApplication::processEvents();
    std::thread worker([&]() {
      Q_EMIT editor.ToolExecutionStarted(QStringLiteral("Worker tool"));
      xiiProgressRange range("Worker import", false, &progress);
      range.SetCompletion(0.5);
    });
    worker.join();
    XII_TEST_BOOL(icon->toolTip() == QStringLiteral("XII Editor"));
    QApplication::processEvents();
    XII_TEST_BOOL(icon->toolTip().contains(QStringLiteral("Worker tool")));
    std::thread finish([&]() { Q_EMIT editor.ToolExecutionFinished(true, QStringLiteral("Worker tool")); });
    finish.join();
    QApplication::processEvents();
    XII_TEST_BOOL(icon->toolTip() == QStringLiteral("XII Editor"));
    pProcessor->StartProcessTask();
    QApplication::processEvents();
    XII_TEST_BOOL(icon->toolTip().contains(QStringLiteral("Background asset processing enabled")));
    pProcessor->StopProcessTask(false, true);
    QApplication::processEvents();
    XII_TEST_BOOL(icon->toolTip() == QStringLiteral("XII Editor"));
    broadcaster = std::thread([&]() {
      xiiAssetProcessorEvent event;
      event.m_Type = xiiAssetProcessorEvent::Type::ProcessTaskStateChanged;
      for (xiiUInt32 i = 0; i < 1000 && keepBroadcasting; ++i)
        pProcessor->m_Events.Broadcast(event);
    });
    xiiEditorSystemTrayTest::Exit(tray).trigger();
    std::thread queued([&]() {
      Q_EMIT editor.ToolExecutionStarted();
      Q_EMIT editor.ToolExecutionFinished(false);
    });
    queued.join();
  }
  keepBroadcasting = false;
  broadcaster.join();
  XII_TEST_BOOL(icon.isNull());
  XII_TEST_BOOL(menu.isNull());
  QApplication::processEvents();
  XII_TEST_BOOL(!window.isVisible());
  {
    xiiProgressRange range("No stale subscription", false, &progress);
    range.SetCompletion(0.5);
  }
}

XII_CREATE_SIMPLE_TEST(EditorSystemTray, IconAndWindowLifetime)
{
  if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
    XII_TEST_BOOL(!QSystemTrayIcon::isSystemTrayAvailable());
  auto*                 pWindow = new QWidget;
  xiiProgress           progress;
  xiiQtEditorSystemTray tray(*xiiQtEditorApp::GetSingleton(), *pWindow, progress);
  auto&                 icon = xiiEditorSystemTrayTest::Icon(tray);
  QIcon                 changed(QStringLiteral(":/GuiFoundation/Icons/Warning.svg"));
  pWindow->setWindowIcon(changed);
  XII_TEST_BOOL(icon.icon().cacheKey() == changed.cacheKey());
  QEvent themeChange(QEvent::ThemeChange);
  QCoreApplication::sendEvent(qApp, &themeChange);
  XII_TEST_BOOL(icon.icon().cacheKey() == changed.cacheKey());
  delete pWindow;
  XII_TEST_BOOL(!icon.isVisible());
  Q_EMIT icon.activated(QSystemTrayIcon::DoubleClick);
  XII_TEST_BOOL(!xiiEditorSystemTrayTest::Menu(tray).isEnabled());
}

XII_CREATE_SIMPLE_TEST(EditorSystemTray, ExitEventLoop)
{
  TrayTestWindow window;
  window.m_bAcceptClose = true;
  xiiProgress           progress;
  xiiQtEditorSystemTray tray(*xiiQtEditorApp::GetSingleton(), window, progress);
  window.show();
  XII_TEST_BOOL(QApplication::quitOnLastWindowClosed());
  bool   bTimedOut = false;
  QTimer watchdog;
  watchdog.setSingleShot(true);
  QObject::connect(&watchdog, &QTimer::timeout, &tray, [&]() {
    bTimedOut = true;
    qApp->quit();
  });
  watchdog.start(2000);
  QTimer::singleShot(0, &tray, [&]() { xiiEditorSystemTrayTest::Exit(tray).trigger(); });
  QApplication::exec();
  XII_TEST_BOOL(!bTimedOut);
  XII_TEST_INT(window.m_uiCloseRequests, 1);
  XII_TEST_BOOL(!window.isVisible());
  XII_TEST_BOOL(!xiiEditorSystemTrayTest::Icon(tray).isVisible());
}

XII_CREATE_SIMPLE_TEST(EditorSystemTray, EditorShutdown)
{
  auto*           pApplication  = xiiApplication::GetApplicationInstance();
  const xiiString sPreviousName = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(sPreviousName));
  auto&            editor = *xiiQtEditorApp::GetSingleton();
  xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/TrayShutdownPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  // Settle the initial document/tab layout before exercising an interactive-session shutdown.
  QApplication::processEvents();
  auto&                     tray = xiiEditorSystemTrayTest::Attach(editor);
  QPointer<QSystemTrayIcon> icon = &xiiEditorSystemTrayTest::Icon(tray);
  QPointer<QMenu>           menu = &xiiEditorSystemTrayTest::Menu(tray);
  Q_EMIT editor.ToolExecutionStarted(QStringLiteral("Queued shutdown tool"));
  editor.ShutdownEditor();
  XII_TEST_BOOL(icon.isNull());
  XII_TEST_BOOL(menu.isNull());
  QApplication::processEvents();
}

#endif
