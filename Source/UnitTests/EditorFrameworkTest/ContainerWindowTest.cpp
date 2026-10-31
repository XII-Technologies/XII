/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <GuiFoundation/Widgets/WindowTitleBar.h>
#include <QApplication>
#include <QCloseEvent>
#include <QEnterEvent>
#include <QHoverEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QScreen>
#include <QStyleOptionToolButton>
#include <QToolBar>
#include <QToolButton>
#include <QWindow>
#include <ads/DockManager.h>
#include <ads/DockWidget.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#endif

namespace
{
  class ChromeDocument : public xiiQtDocumentWindow
  {
  public:
    explicit ChromeDocument(xiiStringView sName) : xiiQtDocumentWindow(sName)
    {
      auto* pEdit = new QLineEdit(this);
      setCentralWidget(pEdit);
      auto* pMenu = qobject_cast<QMenuBar*>(menuWidget());
      pMenu->addMenu(QStringLiteral("File"))->addAction(QStringLiteral("Fixture action"));
      pMenu->addMenu(QStringLiteral("Edit"));
    }

    xiiStringView GetWindowLayoutGroupName() const override { return "WindowChromeFixture"; }
  };

  class MenuChangeObserver : public QObject
  {
  public:
    int m_iAdded   = 0;
    int m_iRemoved = 0;

    bool eventFilter(QObject*, QEvent* pEvent) override
    {
      m_iAdded += pEvent->type() == QEvent::ActionAdded;
      m_iRemoved += pEvent->type() == QEvent::ActionRemoved;
      return false;
    }
  };
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorWindowChrome);

XII_CREATE_SIMPLE_TEST(EditorWindowChrome, CaptionRegionsAndNativeControls)
{
  xiiQtContainerWindow window;
  window.SetWindowPresentationEnabled(false);
  window.resize(1000, 650);
  window.SetWindowPresentationEnabled(true);
  window.show();
  QApplication::processEvents();
  auto*      pShell    = static_cast<xiiQtWindowTitleBar*>(window.menuWidget());
  auto*      pCaption  = pShell->findChild<QWidget*>(QStringLiteral("EditorCaptionSpacer"));
  auto*      pCommands = pShell->findChild<QToolButton*>(QStringLiteral("EditorFindCommand"));
  auto*      pMenu     = pShell->findChild<QMenuBar*>(QStringLiteral("EditorDocumentMenu"));
  const auto caption   = pCaption->mapTo(pShell, pCaption->rect().center());
  XII_TEST_BOOL(pShell->IsCaptionPosition(caption));
  XII_TEST_BOOL(!pShell->IsCaptionPosition(QPoint(-1, -1)));
  XII_TEST_BOOL(!pShell->IsCaptionPosition(pCommands->mapTo(pShell, pCommands->rect().center())));
  pMenu->addMenu(QStringLiteral("Menu"));
  QApplication::processEvents();
  XII_TEST_BOOL(!pShell->IsCaptionPosition(pMenu->mapTo(pShell, pMenu->rect().center())));
  pCommands->setEnabled(false);
  XII_TEST_BOOL(!pShell->IsCaptionPosition(pCommands->mapTo(pShell, pCommands->rect().center())));

  auto DoubleClick = [pShell](QPoint position) {
    QMouseEvent event(QEvent::MouseButtonDblClick, position, pShell->mapToGlobal(position), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(pShell, &event);
    QApplication::processEvents();
  };
  DoubleClick(pCommands->mapTo(pShell, pCommands->rect().center()));
  XII_TEST_BOOL(!window.isMaximized());
  DoubleClick(pCaption->mapTo(pShell, pCaption->rect().center()));
  XII_TEST_BOOL(window.isMaximized());
  DoubleClick(pCaption->mapTo(pShell, pCaption->rect().center()));
  XII_TEST_BOOL(!window.isMaximized());
  window.showFullScreen();
  QApplication::processEvents();
  DoubleClick(pCaption->mapTo(pShell, pCaption->rect().center()));
  XII_TEST_BOOL(window.isFullScreen());
  window.showNormal();
  QApplication::processEvents();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  const HWND hWindow = reinterpret_cast<HWND>(window.winId());
  const auto flags   = GetWindowLongPtrW(hWindow, GWL_STYLE);
  XII_TEST_BOOL((flags & (WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) ==
                (WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX));
  XII_TEST_BOOL(!window.windowFlags().testFlag(Qt::ExpandedClientAreaHint));
  auto* pMaximize = pShell->findChild<QToolButton*>(QStringLiteral("EditorMaximize"));
  XII_TEST_BOOL(pMaximize != nullptr && !pMaximize->accessibleName().isEmpty());
  const auto buttonSize = pMaximize->sizeHint();
  pMaximize->click();
  QApplication::processEvents();
  XII_TEST_BOOL(window.isMaximized());
  XII_TEST_BOOL(pMaximize->accessibleName() == QStringLiteral("Restore"));
  XII_TEST_BOOL(pMaximize->sizeHint() == buttonSize);
  pMaximize->click();
  QApplication::processEvents();
  XII_TEST_BOOL(!window.isMaximized());
  auto HitTest = [hWindow](LONG x, LONG y) {
    return SendMessageW(hWindow, WM_NCHITTEST, 0, MAKELPARAM(x, y));
  };
  auto WidgetHitTest = [&window, hWindow, HitTest](QWidget* pWidget) {
    const auto position       = pWidget->mapTo(&window, pWidget->rect().center());
    const auto scale          = window.windowHandle()->devicePixelRatio();
    POINT      nativePosition = {qRound(position.x() * scale), qRound(position.y() * scale)};
    XII_TEST_BOOL(ClientToScreen(hWindow, &nativePosition) != FALSE);
    return HitTest(nativePosition.x, nativePosition.y);
  };
  for (auto* pScreen : QApplication::screens())
  {
    window.move(pScreen->availableGeometry().topLeft() + QPoint(32, 32));
    QApplication::processEvents();
    xiiTestFramework::Output(xiiTestOutput::Message, "Native frame on screen at (%d, %d), Qt scale %.2f", pScreen->geometry().x(), pScreen->geometry().y(), window.devicePixelRatioF());
    RECT frame;
    XII_TEST_BOOL(GetWindowRect(hWindow, &frame) != FALSE);
    XII_TEST_INT(HitTest(frame.left + 1, frame.top + 1), HTTOPLEFT);
    XII_TEST_INT(HitTest(frame.right - 2, frame.top + 1), HTTOPRIGHT);
    XII_TEST_INT(HitTest(frame.left + 1, frame.bottom - 2), HTBOTTOMLEFT);
    XII_TEST_INT(HitTest(frame.right - 2, frame.bottom - 2), HTBOTTOMRIGHT);
    XII_TEST_INT(HitTest(frame.left + 1, (frame.top + frame.bottom) / 2), HTLEFT);
    XII_TEST_INT(HitTest(frame.right - 2, (frame.top + frame.bottom) / 2), HTRIGHT);
    XII_TEST_INT(HitTest((frame.left + frame.right) / 2, frame.top + 1), HTTOP);
    XII_TEST_INT(HitTest((frame.left + frame.right) / 2, frame.bottom - 2), HTBOTTOM);
    XII_TEST_INT(WidgetHitTest(pCaption), HTCAPTION);
    XII_TEST_INT(WidgetHitTest(pCommands), HTCLIENT);
    XII_TEST_INT(WidgetHitTest(pMenu), HTCLIENT);
    XII_TEST_INT(WidgetHitTest(pMaximize), HTCLIENT);
  }
  SendMessageW(hWindow, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
  QApplication::processEvents();
  XII_TEST_BOOL(window.isMaximized() && IsZoomed(hWindow));
  XII_TEST_INT(WidgetHitTest(pCaption), HTCAPTION);
  XII_TEST_INT(WidgetHitTest(pCommands), HTCLIENT);
  SendMessageW(hWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
  QApplication::processEvents();
  XII_TEST_BOOL(!window.isMaximized() && !IsZoomed(hWindow));
#endif
}

XII_CREATE_SIMPLE_TEST(EditorWindowChrome, FocusAndMenuGeometry)
{
  xiiQtContainerWindow window;
  window.SetWindowPresentationEnabled(false);
  auto* pFirst  = new ChromeDocument("ChromeFirst");
  auto* pSecond = new ChromeDocument("ChromeSecond");
  XII_SCOPE_EXIT(window.SetWindowPresentationEnabled(false); window.close(); QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete));
  window.resize(1200, 700);
  window.SetWindowPresentationEnabled(true);
  window.show();
  window.activateWindow();
  pSecond->EnsureVisible();
  pSecond->centralWidget()->setFocus();
  QApplication::processEvents();
  auto* pSource   = qobject_cast<QMenuBar*>(pSecond->menuWidget());
  auto* pShell    = static_cast<xiiQtWindowTitleBar*>(window.menuWidget());
  auto* pMenu     = pShell->findChild<QMenuBar*>(QStringLiteral("EditorDocumentMenu"));
  auto* pCommands = pShell->findChild<QToolButton*>(QStringLiteral("EditorFindCommand"));
  XII_TEST_BOOL(pMenu->actions() == pSource->actions());
  MenuChangeObserver observer;
  pMenu->installEventFilter(&observer);
  const auto menuGeometry    = pMenu->geometry();
  const auto commandGeometry = pCommands->geometry();
  const auto actions         = pMenu->actions();
  pMenu->setActiveAction(actions.front());
  for (int i = 0; i < 12; ++i)
  {
    pCommands->setFocus();
    Q_EMIT qApp->focusChanged(pSecond->centralWidget(), nullptr);
    Q_EMIT qApp->focusChanged(nullptr, pCommands);
    pSecond->centralWidget()->setFocus();
    QApplication::processEvents();
    XII_TEST_BOOL(pMenu->actions() == actions);
    XII_TEST_BOOL(pMenu->geometry() == menuGeometry);
    XII_TEST_BOOL(pCommands->geometry() == commandGeometry);
  }
  XII_TEST_INT(observer.m_iAdded, 0);
  XII_TEST_INT(observer.m_iRemoved, 0);
  QMenu plugin(QStringLiteral("Plugin"));
  pSource->insertMenu(pSource->actions().back(), &plugin);
  XII_TEST_BOOL(pMenu->actions() == pSource->actions());
  XII_TEST_INT(observer.m_iAdded, 1);
  XII_TEST_INT(observer.m_iRemoved, 0);
  pSource->removeAction(plugin.menuAction());
  XII_TEST_BOOL(pMenu->actions() == actions);
  XII_TEST_INT(observer.m_iRemoved, 1);
  pFirst->EnsureVisible();
  pFirst->centralWidget()->setFocus();
  QApplication::processEvents();
  XII_TEST_BOOL(pMenu->actions() == qobject_cast<QMenuBar*>(pFirst->menuWidget())->actions());
  pSecond->EnsureVisible();
  pSecond->centralWidget()->setFocus();
  QApplication::processEvents();
  XII_TEST_BOOL(pMenu->actions() == actions);
  // Closing the active document must update the menu even when no new widget gets focus.
  pSecond->CloseDocumentWindow();
  QApplication::sendPostedEvents(pSecond, QEvent::MetaCall);
  QApplication::processEvents();
  XII_TEST_BOOL(pMenu->actions() == qobject_cast<QMenuBar*>(pFirst->menuWidget())->actions());
}

XII_CREATE_SIMPLE_TEST(EditorWindowChrome, HoverAndResponsiveGeometry)
{
  xiiQtContainerWindow window;
  window.show();
  auto* pShell = static_cast<xiiQtWindowTitleBar*>(window.menuWidget());
  auto* pMenu  = pShell->findChild<QMenuBar*>(QStringLiteral("EditorDocumentMenu"));
  pMenu->addMenu(QStringLiteral("Project"))->addAction(QStringLiteral("Fixture action"));
  pMenu->addMenu(QStringLiteral("Edit"));
  auto* pToolbar = new QToolBar(QStringLiteral("Document toolbar"), &window);
  window.addToolBar(pToolbar);
  pToolbar->addAction(window.windowIcon(), QStringLiteral("Action"));
  auto* pButton = new QToolButton(pToolbar);
  pButton->setText(QStringLiteral("Menu"));
  pButton->setIcon(window.windowIcon());
  pButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  auto* pPopup = new QMenu(pButton);
  pPopup->addAction(QStringLiteral("Popup action"));
  pButton->setMenu(pPopup);
  pButton->setPopupMode(QToolButton::InstantPopup);
  pToolbar->addWidget(pButton);

  for (int iWidth : {800, 960, 1099, 1100, 1101, 1280})
  {
    window.resize(iWidth, 650);
    QApplication::processEvents();
    QList<QWidget*> widgets = {pShell, pMenu, pToolbar};
    for (auto* pControl : window.findChildren<QToolButton*>())
      if (pControl->isVisible())
        widgets.push_back(pControl);
    QList<QRect> geometries;
    QList<QSize> hints;
    for (auto* pWidget : widgets)
    {
      geometries.push_back(pWidget->geometry());
      hints.push_back(pWidget->sizeHint());
    }
    for (auto* pWidget : widgets)
    {
      QEnterEvent enter(QPointF(5, 5), QPointF(5, 5), pWidget->mapToGlobal(QPoint(5, 5)));
      QApplication::sendEvent(pWidget, &enter);
      QHoverEvent hover(QEvent::HoverMove, QPointF(5, 5), QPointF(5, 5), QPointF(4, 4));
      QApplication::sendEvent(pWidget, &hover);
      pWidget->setFocus();
      QApplication::processEvents();
      QEvent leave(QEvent::Leave);
      QApplication::sendEvent(pWidget, &leave);
    }
    pPopup->popup(pButton->mapToGlobal(QPoint(0, pButton->height())));
    QApplication::processEvents();
    pPopup->hide();
    QApplication::processEvents();
    for (qsizetype i = 0; i < widgets.size(); ++i)
    {
      XII_TEST_BOOL(widgets[i]->geometry() == geometries[i]);
      XII_TEST_BOOL(widgets[i]->sizeHint() == hints[i]);
    }
    for (const auto* szName : {"EditorFindCommand", "EditorWorkspaces"})
    {
      const auto* pControl = pShell->findChild<QToolButton*>(QString::fromUtf8(szName));
      XII_TEST_INT(pControl->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
      XII_TEST_BOOL(!pControl->accessibleName().isEmpty());
    }
  }

  // The production proxy style must not change dimensions with hover, focus or press.
  xiiQtDarkEditorStyle   style;
  QStyleOptionToolButton option;
  option.initFrom(pButton);
  option.iconSize        = QSize(16, 16);
  option.text            = pButton->text();
  option.toolButtonStyle = pButton->toolButtonStyle();
  const auto size        = style.sizeFromContents(QStyle::CT_ToolButton, &option, QSize(70, 16), pButton);
  option.state |= QStyle::State_MouseOver | QStyle::State_HasFocus | QStyle::State_Sunken;
  XII_TEST_BOOL(style.sizeFromContents(QStyle::CT_ToolButton, &option, QSize(70, 16), pButton) == size);
}

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
