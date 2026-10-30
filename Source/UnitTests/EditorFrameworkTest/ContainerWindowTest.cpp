/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <QApplication>
#include <QCloseEvent>
#include <QLineEdit>
#include <QMenuBar>
#include <ads/DockManager.h>
#include <ads/DockWidget.h>

XII_CREATE_SIMPLE_TEST_GROUP(EditorContainerShutdown);

XII_CREATE_SIMPLE_TEST(EditorContainerShutdown, QueuedCloseAndFocusChanges)
{
  const bool bQuitOnClose = QApplication::quitOnLastWindowClosed();
  QApplication::setQuitOnLastWindowClosed(false);
  XII_SCOPE_EXIT(QApplication::setQuitOnLastWindowClosed(bQuitOnClose));

  xiiQtContainerWindow        window;
  QPointer<ads::CDockManager> manager = window.GetDockManager();
  auto*                       pDock   = new ads::CDockWidget(manager, QStringLiteral("Shutdown focus fixture"));
  pDock->setObjectName(QStringLiteral("ShutdownFocusFixture"));
  auto* pEdit = new QLineEdit(pDock);
  pDock->setWidget(pEdit);
  manager->addDockWidget(ads::CenterDockWidgetArea, pDock);
  window.show();
  window.activateWindow();
  pEdit->setFocus();
  QApplication::processEvents();

  // Exercise QWidget's queued close/hide path, without reading or writing user layouts.
  window.SetWindowPresentationEnabled(false);
  XII_TEST_BOOL(QMetaObject::invokeMethod(&window, "close", Qt::QueuedConnection));
  QApplication::sendPostedEvents(&window, QEvent::MetaCall);
  XII_TEST_BOOL(!window.isVisible());
  XII_TEST_BOOL(window.GetDockManager() == nullptr);
  XII_TEST_BOOL(window.GetFocusedDocumentWindow() == nullptr);

  // Focus notifications and layout work can still arrive before deleteLater is processed.
  Q_EMIT qApp->focusChanged(nullptr, nullptr);
  const auto geometry = window.geometry();
  window.RestoreWindowLayout();
  XII_TEST_BOOL(window.geometry() == geometry);
  window.SetWindowPresentationEnabled(true);
  XII_TEST_BOOL(QMetaObject::invokeMethod(&window, "SlotRestoreLayout", Qt::QueuedConnection));
  QApplication::sendPostedEvents(&window, QEvent::MetaCall);
  XII_TEST_BOOL(!window.isVisible());

  // A second close must not reschedule deletion through the cleared dock-manager pointer.
  QCloseEvent repeatedClose;
  QApplication::sendEvent(&window, &repeatedClose);
  XII_TEST_BOOL(repeatedClose.isAccepted());
  QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  XII_TEST_BOOL(manager == nullptr);
  Q_EMIT qApp->focusChanged(nullptr, nullptr);
  XII_TEST_BOOL(window.GetFocusedDocumentWindow() == nullptr);
  auto* pMenu = window.findChild<QMenuBar*>(QStringLiteral("EditorDocumentMenu"));
  XII_TEST_BOOL(pMenu != nullptr && pMenu->actions().isEmpty());
}

XII_CREATE_SIMPLE_TEST(EditorContainerShutdown, AcceptedCloseBeforeApplicationShutdown)
{
  auto*           pApplication = xiiApplication::GetApplicationInstance();
  const xiiString previousName = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(previousName));
  const bool bQuitOnClose = QApplication::quitOnLastWindowClosed();
  QApplication::setQuitOnLastWindowClosed(false);
  XII_SCOPE_EXIT(QApplication::setQuitOnLastWindowClosed(bQuitOnClose));

  auto*                  pEditor = xiiQtEditorApp::GetSingleton();
  const xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/EditorClosePreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  pEditor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(pEditor->ShutdownEditor());

  auto* pWindow = xiiQtContainerWindow::GetContainerWindow();
  if (!XII_TEST_BOOL(pWindow != nullptr))
    return;
  xiiHybridArray<xiiQtDocumentWindow*, 16> documents;
  pWindow->GetDocumentWindows(documents);
  XII_TEST_BOOL(!documents.IsEmpty());
  XII_TEST_BOOL(!xiiQtApplicationPanel::GetAllApplicationPanels().IsEmpty());
  QPointer<ads::CDockManager> manager = pWindow->GetDockManager();
  XII_TEST_BOOL(QMetaObject::invokeMethod(pWindow, "close", Qt::QueuedConnection));
  QApplication::sendPostedEvents(pWindow, QEvent::MetaCall);
  XII_TEST_BOOL(pWindow->GetDockManager() == nullptr);
  Q_EMIT qApp->focusChanged(nullptr, nullptr);
  QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
  XII_TEST_BOOL(manager == nullptr);
  XII_TEST_BOOL(xiiQtApplicationPanel::GetAllApplicationPanels().IsEmpty());
  pWindow->GetDocumentWindows(documents);
  XII_TEST_BOOL(documents.IsEmpty());
  XII_TEST_BOOL(pWindow->GetFocusedDocumentWindow() == nullptr);
}
