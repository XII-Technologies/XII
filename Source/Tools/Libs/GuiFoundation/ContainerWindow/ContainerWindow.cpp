/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/Style/DarkEditorStyle.moc.h>
#include <GuiFoundation/Widgets/CommandPalette.moc.h>
#include <GuiFoundation/Widgets/WindowTitleBar.h>
#include <QApplication>
#include <QCloseEvent>
#include <QInputDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QShortcut>
#include <QStatusBar>
#include <QStyleHints>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ads/DockAreaWidget.h>
#include <ads/DockManager.h>
#include <ads/DockWidgetTab.h>
#include <ads/FloatingDockContainer.h>

xiiQtContainerWindow* xiiQtContainerWindow::s_pContainerWindow = nullptr;
bool                  xiiQtContainerWindow::s_bForceClose      = false;

namespace
{
  bool GetProjectLayoutPath(xiiStringBuilder& out_sFile, bool bWrite)
  {
    if (!xiiToolsProject::IsProjectOpen())
    {
      out_sFile.Clear();
      return false;
    }
    out_sFile = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    out_sFile.AppendPath("layout.settings");
    if (!bWrite && !QFile::exists(out_sFile.GetData()))
    {
      out_sFile.Clear();
      return false;
    }
    return true;
  }

  bool GetApplicationLayoutPath(xiiStringBuilder& out_sFile, bool bWrite)
  {
    out_sFile = xiiApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
    out_sFile.AppendPath("layout.settings");
    if (!bWrite && !QFile::exists(out_sFile.GetData()))
    {
      out_sFile.Clear();
      return false;
    }
    return true;
  }
} // namespace

xiiQtContainerWindow::xiiQtContainerWindow()
{
  setMinimumSize(QSize(800, 450));
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  // Qt retains the native resize frame and system move/menu handling. The title
  // controls are Qt widgets so they remain accessible in the expanded client area.
  setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::ExpandedClientAreaHint | Qt::NoTitleBarBackgroundHint | Qt::WindowSystemMenuHint);
  setAttribute(Qt::WA_ContentsMarginsRespectsSafeArea, false);
  QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif

  m_bWindowLayoutRestored         = false;
  m_pStatusBarLabel               = nullptr;
  m_iWindowLayoutRestoreScheduled = 0;

  s_pContainerWindow = this;

  setObjectName("xiiEditor");
  setWindowIcon(QIcon(QStringLiteral(":/GuiFoundation/XII-Logo.svg")));

  xiiQtDocumentWindow::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtContainerWindow::DocumentWindowEventHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtContainerWindow::ProjectEventHandler, this));
  xiiQtUiServices::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtContainerWindow::UIServicesEventHandler, this));

  UpdateWindowTitle();

  ads::CDockManager::ConfigFlags flags = ads::CDockManager::DefaultDockAreaButtons;
  flags |= ads::CDockManager::ActiveTabHasCloseButton;
  flags |= ads::CDockManager::XmlCompressionEnabled;
  flags |= ads::CDockManager::FloatingContainerHasWidgetTitle;
  flags |= ads::CDockManager::FloatingContainerHasWidgetIcon;
  flags |= ads::CDockManager::HideSingleCentralWidgetTitleBar;
  flags |= ads::CDockManager::DragPreviewShowsContentPixmap;
  flags |= ads::CDockManager::FocusHighlighting;
  // flags |= ads::CDockManager::AlwaysShowTabs;
  // flags |= ads::CDockManager::DockAreaHasCloseButton;
  flags |= ads::CDockManager::DockAreaCloseButtonClosesTab;
  flags |= ads::CDockManager::MiddleMouseButtonClosesTab;
  flags |= ads::CDockManager::DockAreaHasTabsMenuButton;
  flags |= ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility;
  // flags |= ads::CDockManager::AllTabsHaveCloseButton;
  flags |= ads::CDockManager::RetainTabSizeWhenCloseButtonHidden;
  flags |= ads::CDockManager::DockAreaHideDisabledButtons;
  flags |= ads::CDockManager::DockAreaHasUndockButton;
  // flags |= ads::CDockManager::DoubleClickUndocksWidget; // This is not ideal.
  flags |= ads::CDockManager::OpaqueSplitterResize;
  ads::CDockManager::setConfigFlags(flags);

  ads::CDockManager::AutoHideFlags autoHideFlags = ads::CDockManager::AutoHideFeatureEnabled;
  autoHideFlags |= ads::CDockManager::DockAreaHasAutoHideButton;
  autoHideFlags |= ads::CDockManager::AutoHideHasMinimizeButton;
  autoHideFlags |= ads::CDockManager::AutoHideHasCloseButton;
  autoHideFlags |= ads::CDockManager::AutoHideShowOnMouseOver;
  autoHideFlags |= ads::CDockManager::AutoHideCloseOnOutsideMouseClick;
  ads::CDockManager::setAutoHideConfigFlags(autoHideFlags);

  m_pDockManager = new ads::CDockManager(this);
  xiiQtDarkEditorStyle::ApplyDockStyle(m_pDockManager);

  auto* pShell = new xiiQtWindowTitleBar(this);
  pShell->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  setMenuWidget(pShell);
  auto* pBrand    = new QLabel(QStringLiteral("  XII  "), pShell);
  QFont brandFont = pBrand->font();
  brandFont.setBold(true);
  pBrand->setFont(brandFont);
  pBrand->setAttribute(Qt::WA_TransparentForMouseEvents);
  pShell->addWidget(pBrand);
  pShell->addSeparator();
  m_pDocumentMenu = new QMenuBar(pShell);
  m_pDocumentMenu->setNativeMenuBar(false);
  m_pDocumentMenu->setObjectName(QStringLiteral("EditorDocumentMenu"));
  m_pDocumentMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  m_pDocumentMenu->setMinimumWidth(160);
  pShell->addWidget(m_pDocumentMenu);
  pShell->addSeparator();
  m_pProjectLabel = new QLabel(pShell);
  m_pProjectLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pProjectLabel->setMinimumWidth(80);
  m_pProjectLabel->setMaximumWidth(200);
  m_pProjectLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  pShell->addWidget(m_pProjectLabel);
  auto* pSpacer = new QWidget(pShell);
  pSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  pSpacer->setMinimumWidth(32);
  pSpacer->setAttribute(Qt::WA_TransparentForMouseEvents);
  pShell->addWidget(pSpacer);

  auto* pCommands = new QToolButton(pShell);
  pCommands->setObjectName(QStringLiteral("EditorFindCommand"));
  pCommands->setFocusPolicy(Qt::NoFocus);
  pCommands->setText(tr("Commands"));
  pCommands->setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Search.svg")));
  pCommands->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  pCommands->setToolTip(tr("Search commands for the focused document (Ctrl+Shift+P)"));
  pShell->addWidget(pCommands);
  connect(pCommands, &QToolButton::clicked, this, &xiiQtContainerWindow::ShowCommandPalette);
  auto* pShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+P")), this);
  pShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  connect(pShortcut, &QShortcut::activated, this, &xiiQtContainerWindow::ShowCommandPalette);

  auto* pWorkspaces = new QToolButton(pShell);
  pWorkspaces->setObjectName(QStringLiteral("EditorWorkspaces"));
  pWorkspaces->setFocusPolicy(Qt::NoFocus);
  pWorkspaces->setText(tr("Workspaces"));
  pWorkspaces->setIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/Window.svg")));
  pWorkspaces->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  pWorkspaces->setToolTip(tr("Workspaces and application panels"));
  pWorkspaces->setPopupMode(QToolButton::InstantPopup);
  auto* pWorkspaceMenu = new QMenu(pWorkspaces);
  pWorkspaces->setMenu(pWorkspaceMenu);
  pShell->addWidget(pWorkspaces);
  connect(pWorkspaceMenu, &QMenu::aboutToShow, this, [this, pWorkspaceMenu]() {
    pWorkspaceMenu->clear();
    auto* pPanels = pWorkspaceMenu->addMenu(tr("Application Panels"));
    for (auto* pPanel : m_ApplicationPanels)
    {
      // ADS omits icons from checkable toggle actions; retain both in the workspace menu.
      auto* pAction = pPanel->toggleViewAction();
      pAction->setIcon(pPanel->icon());
      pAction->setIconVisibleInMenu(true);
      pPanels->addAction(pAction);
    }
    pWorkspaceMenu->addSeparator();
    auto* pSave = pWorkspaceMenu->addAction(tr("Save Workspace As..."));
    pSave->setEnabled(xiiToolsProject::IsProjectOpen());
    connect(pSave, &QAction::triggered, this, [this]() {
      bool       bAccepted = false;
      const auto sName     = QInputDialog::getText(this, tr("Save Workspace"), tr("Workspace name"), QLineEdit::Normal, QString(), &bAccepted).trimmed();
      if (bAccepted && !sName.isEmpty())
      {
        if (SaveWorkspace(sName).Failed())
          xiiQtUiServices::MessageBoxWarning("The workspace could not be saved.");
        else
          statusBar()->showMessage(tr("Saved workspace: %1").arg(sName), 4000);
      }
    });
    const auto names = GetWorkspaceNames();
    if (!names.isEmpty())
      pWorkspaceMenu->addSeparator();
    for (const auto& sName : names)
    {
      auto* pRestore = pWorkspaceMenu->addAction(sName);
      pRestore->setEnabled(xiiQtDocumentWindow::s_bAllowRestoreWindowLayout);
      connect(pRestore, &QAction::triggered, this, [this, sName]() {
        if (RestoreWorkspace(sName).Failed())
          xiiQtUiServices::MessageBoxWarning("The workspace could not be restored.");
      });
    }
    if (!names.isEmpty())
    {
      pWorkspaceMenu->addSeparator();
      connect(pWorkspaceMenu->addAction(tr("Delete Saved Workspace...")), &QAction::triggered, this, [this, names]() {
        bool       bAccepted = false;
        const auto sName     = QInputDialog::getItem(this, tr("Delete Saved Workspace"), tr("Saved workspace to delete"), names, 0, false, &bAccepted);
        if (bAccepted && RemoveWorkspace(sName).Failed())
          xiiQtUiServices::MessageBoxWarning("The saved workspace could not be deleted.");
      });
    }
  });
  UpdateWindowTitle();

  pShell->AddWindowControls();
  connect(qApp, &QApplication::focusChanged, this, [this]() { UpdateDocumentMenu(); });
  connect(m_pDockManager, &ads::CDockManager::floatingWidgetCreated, this, &xiiQtContainerWindow::SlotFloatingWidgetOpened);
}

xiiQtContainerWindow::~xiiQtContainerWindow()
{
  // Child-widget destruction changes focus after C++ members have been torn
  // down. Stop menu discovery before QMainWindow destroys those children.
  disconnect(qApp, nullptr, this, nullptr);
  for (auto* pWindow : m_DocumentWindows)
    if (pWindow->menuWidget())
      pWindow->menuWidget()->removeEventFilter(this);
  m_pMenuSource.clear();
  m_pDocumentMenu->clear();
  m_pDocumentMenu    = nullptr;
  s_pContainerWindow = nullptr;

  xiiQtDocumentWindow::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtContainerWindow::DocumentWindowEventHandler, this));
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtContainerWindow::ProjectEventHandler, this));
  xiiQtUiServices::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtContainerWindow::UIServicesEventHandler, this));
}

void xiiQtContainerWindow::UpdateWindowTitle()
{
  xiiStringBuilder sTitle;

  if (xiiToolsProject::IsProjectOpen())
  {
    sTitle = xiiToolsProject::GetSingleton()->GetProjectName(false);
    sTitle.Append(" - ");
  }

  sTitle.Append(xiiApplication::GetApplicationInstance()->GetApplicationName().GetView());

  setWindowTitle(xiiMakeQString(sTitle.GetView()));
  if (m_pProjectLabel)
  {
    m_pProjectLabel->setText(xiiToolsProject::IsProjectOpen() ? xiiMakeQString(xiiToolsProject::GetSingleton()->GetProjectName(false)) : tr("No project open"));
    m_pProjectLabel->setToolTip(windowTitle());
  }
}

xiiQtDocumentWindow* xiiQtContainerWindow::GetFocusedDocumentWindow() const
{
  // An accepted close clears the manager before hiding/deferred deletion can change focus.
  if (!m_pDockManager)
    return nullptr;

  xiiQtDocumentWindow* pDocumentWindow = nullptr;
  for (auto* pCurrent = QApplication::focusWidget(); pCurrent; pCurrent = pCurrent->parentWidget())
    if ((pDocumentWindow = qobject_cast<xiiQtDocumentWindow*>(pCurrent)) != nullptr)
      return pDocumentWindow;
  const auto uiIndex = m_DocumentDocks.IndexOf(m_pDockManager->focusedDockWidget());
  if (uiIndex != xiiInvalidIndex)
    return m_DocumentWindows[uiIndex];
  for (xiiUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    if (m_DocumentDocks[i]->dockAreaWidget() && m_DocumentDocks[i]->isCurrentTab() && m_DocumentWindows[i]->isVisible() && !m_DocumentDocks[i]->isFloating())
      return m_DocumentWindows[i];
  return nullptr;
}

void xiiQtContainerWindow::UpdateDocumentMenu()
{
  if (!m_pDocumentMenu)
    return;
  auto*      pWindow = GetFocusedDocumentWindow();
  auto*      pSource = pWindow ? qobject_cast<QMenuBar*>(pWindow->menuWidget()) : nullptr;
  const auto actions = pSource ? pSource->actions() : QList<QAction*>();
  if (m_pMenuSource == pSource && m_pDocumentMenu->actions() == actions)
    return;
  m_pDocumentMenu->clear();
  m_pMenuSource = pSource;
  m_pDocumentMenu->addActions(actions);
}

void xiiQtContainerWindow::ShowCommandPalette()
{
  auto*    pDocumentWindow = GetFocusedDocumentWindow();
  QWidget* pContext        = QApplication::focusWidget();
  if (!pContext || (pDocumentWindow && !pDocumentWindow->isAncestorOf(pContext)))
    pContext = pDocumentWindow;
  xiiQtCommandPalette palette(this, xiiQtProxy::GetContextActions(pDocumentWindow ? pDocumentWindow->GetDocument() : nullptr, pContext));
  palette.exec();
}

void xiiQtContainerWindow::ScheduleRestoreWindowLayout()
{
  if (!m_bWindowPresentationEnabled)
  {
    m_iWindowLayoutRestoreScheduled = 1;
    return;
  }
  m_iWindowLayoutRestoreScheduled++;
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

QStringList xiiQtContainerWindow::GetWorkspaceNames() const
{
  if (!xiiToolsProject::IsProjectOpen())
    return {};
  xiiStringBuilder file = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  file.AppendPath("workspaces.settings");
  QSettings   settings(xiiMakeQString(file), QSettings::IniFormat);
  QStringList names;
  for (const auto& group : settings.childGroups())
  {
    const auto name = settings.value(group + QStringLiteral("/Name")).toString();
    if (!name.isEmpty())
      names.push_back(name);
  }
  names.sort(Qt::CaseInsensitive);
  return names;
}

xiiResult xiiQtContainerWindow::SaveWorkspace(const QString& sName)
{
  if (!xiiToolsProject::IsProjectOpen() || !m_bWindowPresentationEnabled || sName.trimmed().isEmpty() || sName.size() > 80)
    return XII_FAILURE;
  xiiStringBuilder file = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  file.AppendPath("workspaces.settings");
  QSettings settings(xiiMakeQString(file), QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(sName.trimmed().toUtf8().toHex()));
  settings.remove(QString());
  settings.setValue(QStringLiteral("Name"), sName.trimmed());
  settings.setValue(QStringLiteral("Shell"), m_pDockManager->saveState(1));
  for (auto* pWindow : m_DocumentWindows)
  {
    const auto group = xiiMakeQString(pWindow->GetWindowLayoutGroupName()).toUtf8().toHex();
    settings.setValue(QStringLiteral("Documents/") + QString::fromLatin1(group), pWindow->m_pDockManager->saveState());
  }
  settings.endGroup();
  settings.sync();
  return settings.status() == QSettings::NoError ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiQtContainerWindow::RestoreWorkspace(const QString& sName)
{
  if (!xiiToolsProject::IsProjectOpen() || !m_bWindowPresentationEnabled || !xiiQtDocumentWindow::s_bAllowRestoreWindowLayout)
    return XII_FAILURE;
  xiiStringBuilder file = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  file.AppendPath("workspaces.settings");
  QSettings settings(xiiMakeQString(file), QSettings::IniFormat);
  settings.beginGroup(QString::fromLatin1(sName.trimmed().toUtf8().toHex()));
  const auto state = settings.value(QStringLiteral("Shell")).toByteArray();
  if (state.isEmpty())
    return XII_FAILURE;
  const auto        previous = m_pDockManager->saveState(1);
  QList<QByteArray> previousDocuments;
  for (auto* pWindow : m_DocumentWindows)
    previousDocuments.push_back(pWindow->m_pDockManager->saveState());
  if (!m_pDockManager->restoreState(state, 1))
  {
    m_pDockManager->restoreState(previous, 1);
    return XII_FAILURE;
  }
  // A saved workspace must never close documents that were opened after it was saved.
  for (auto* pDock : m_DocumentDocks)
  {
    if (!pDock->dockContainer())
      m_pDockManager->addDockWidgetTab(ads::TopDockWidgetArea, pDock);
    if (pDock->isClosed())
      pDock->toggleView(true);
  }
  layout()->activate();
  bool bSucceeded = true;
  for (auto* pWindow : m_DocumentWindows)
  {
    const auto group         = xiiMakeQString(pWindow->GetWindowLayoutGroupName()).toUtf8().toHex();
    const auto documentState = settings.value(QStringLiteral("Documents/") + QString::fromLatin1(group)).toByteArray();
    if (!documentState.isEmpty())
    {
      if (!pWindow->m_pDockManager->restoreState(documentState))
      {
        bSucceeded = false;
        break;
      }
    }
  }
  if (!bSucceeded)
  {
    m_pDockManager->restoreState(previous, 1);
    for (xiiUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
      m_DocumentWindows[i]->m_pDockManager->restoreState(previousDocuments[i]);
  }
  return bSucceeded ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiQtContainerWindow::RemoveWorkspace(const QString& sName)
{
  if (!xiiToolsProject::IsProjectOpen() || sName.trimmed().isEmpty())
    return XII_FAILURE;
  xiiStringBuilder file = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  file.AppendPath("workspaces.settings");
  QSettings settings(xiiMakeQString(file), QSettings::IniFormat);
  settings.remove(QString::fromLatin1(sName.trimmed().toUtf8().toHex()));
  settings.sync();
  return settings.status() == QSettings::NoError ? XII_SUCCESS : XII_FAILURE;
}

void xiiQtContainerWindow::SlotRestoreLayout()
{
  XII_LOG_BLOCK("DocumentSlotRestoreLayout");

  if (!m_bWindowPresentationEnabled)
  {
    m_iWindowLayoutRestoreScheduled = 1;
    return;
  }
  RestoreWindowLayout();
}

void xiiQtContainerWindow::setVisible(bool bVisible)
{
  // Reject indirect QWidget/queued show requests before any native mapping can occur.
  if (bVisible && !m_bWindowPresentationEnabled)
    return;
  QMainWindow::setVisible(bVisible);
}

void xiiQtContainerWindow::closeEvent(QCloseEvent* e)
{
  if (!m_pDockManager)
  {
    QMainWindow::closeEvent(e);
    return;
  }

  SaveWindowLayout();
  SaveDocumentLayouts();

  if (s_bForceClose)
    return;

  s_bForceClose = true;
  XII_SCOPE_EXIT(s_bForceClose = false);

  e->setAccepted(true);

  if (!xiiToolsProject::CanCloseProject())
  {
    e->setAccepted(false);
    return;
  }

  xiiToolsProject::SaveProjectState();

  // do not close the documents in the main container window here,
  // as that would remove them from the recently-open documents list and not restore them when opening the editor again
  xiiDynamicArray<xiiQtDocumentWindow*> windows = m_DocumentWindows;
  for (xiiQtDocumentWindow* pWindow : windows)
  {
    pWindow->DisableWindowLayoutSaving();
    pWindow->ShutdownDocumentWindow();
  }

  // Shut down document windows before deferring destruction of their Qt-owned dock manager.
  m_pDockManager->deleteLater();
  m_pDockManager = nullptr;
  QMainWindow::closeEvent(e);
}

void xiiQtContainerWindow::SaveWindowLayout()
{
  if (!m_pDockManager || !m_bWindowPresentationEnabled)
    return;

  xiiStringBuilder sFile;
  GetApplicationLayoutPath(sFile, true);

  xiiStringBuilder sProjectFile;
  GetProjectLayoutPath(sProjectFile, true);

  QSettings Settings(xiiToolsProject::IsProjectOpen() ? sProjectFile.GetData() : sFile.GetData(), QSettings::IniFormat);
  Settings.beginGroup(QString::fromUtf8("ContainerWnd_xiiEditor"));
  {
    Settings.setValue("DockManagerState", m_pDockManager->saveState(1));
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowState", saveState());
  }
  Settings.endGroup();

  if (xiiToolsProject::IsProjectOpen())
  {
    // The last open project always serves as the default layout in case
    // a new project is created or a project without layout data is opened.
    QFile::remove(sFile.GetData());
    QFile::copy(sProjectFile.GetData(), sFile.GetData());
  }
}

void xiiQtContainerWindow::SaveDocumentLayouts()
{
  if (!m_bWindowPresentationEnabled)
    return;
  for (xiiUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->SaveWindowLayout();
}

void xiiQtContainerWindow::RestoreWindowLayout()
{
  // A queued layout restore may arrive after closeEvent has released the manager.
  if (!m_pDockManager)
    return;

  if (m_iWindowLayoutRestoreScheduled > 0)
    --m_iWindowLayoutRestoreScheduled;
  if (m_iWindowLayoutRestoreScheduled > 0)
    return;

  bool bCreteDefaultLayout = true;
  // Restoring geometry must never imply presentation. In particular, startup restores the
  // complete workspace before the application deliberately shows the container once.
  XII_SCOPE_EXIT(if (bCreteDefaultLayout) setWindowState(Qt::WindowMaximized));

  xiiStringBuilder sFile;
  if (!GetProjectLayoutPath(sFile, false))
  {
    if (!GetApplicationLayoutPath(sFile, false))
    {
      // No project or app settings file found, exiting.
      return;
    }
  }

  {
    QSettings Settings(sFile.GetData(), QSettings::IniFormat);
    Settings.beginGroup(QString::fromUtf8("ContainerWnd_xiiEditor"));
    {
      QByteArray geom = Settings.value("WindowGeometry", QByteArray()).toByteArray();
      if (!geom.isEmpty())
      {
        bCreteDefaultLayout = false;
        restoreGeometry(geom);
        restoreState(Settings.value("WindowState", saveState()).toByteArray());
        auto dockState = Settings.value("DockManagerState");
        if (dockState.isValid() && dockState.typeId() == QMetaType::QByteArray)
        {
          m_pDockManager->restoreState(dockState.toByteArray(), 1);
          // As document windows can't be in a closed state (as pressing x destroys them),
          // we need to fix any document window that was accidentally saved in its closed state.
          for (ads::CDockWidget* dock : m_DocumentDocks)
          {
            if (dock->isClosed())
            {
              if (dock->dockContainer() == nullptr)
              {
                if (m_DocumentDocks.GetCount() >= 2)
                {
                  // If we can (we are not the only dock window), we are going to attach to a window that isn't us, ideally the settings window.
                  xiiUInt32 uiBestIndex = 0;
                  for (xiiUInt32 i = 0; i < m_DocumentDocks.GetCount(); i++)
                  {
                    if (m_DocumentWindows[i]->GetUniqueName() == "Settings")
                    {
                      uiBestIndex = i;
                      break;
                    }
                    else if (m_DocumentDocks[i] != dock)
                    {
                      uiBestIndex = i;
                    }
                  }

                  ads::CDockAreaWidget* dockArea = m_DocumentDocks[uiBestIndex]->dockAreaWidget();
                  m_pDockManager->addDockWidgetTabToArea(dock, dockArea);
                }
                else
                {
                  m_pDockManager->addDockWidgetTab(ads::LeftDockWidgetArea, dock);
                }
              }
              dock->toggleView();
            }
          }
        }
      }
    }
    Settings.endGroup();
  }

  // Do NOT restore the layouts of the document windows here.
  // The window may be too small at this time, and the layout restoration may thus resize the document widgets to the bare minimum and destroy the layout.

  m_bWindowLayoutRestored = true;
}

void xiiQtContainerWindow::RestoreDocumentLayouts()
{
  // Size the hidden workspace before document layouts use their available dock geometry.
  layout()->activate();
  for (auto* pWindow : m_DocumentWindows)
  {
    pWindow->layout()->activate();
    pWindow->RestoreWindowLayout(false);
  }
}

void xiiQtContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<xiiQtDocumentWindow*>(pDocWindow));
}

void xiiQtContainerWindow::SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget)
{
  FloatingWidget->installEventFilter(this);
}

void xiiQtContainerWindow::SlotDockWidgetFloatingChanged(bool)
{
  for (auto pDoc : m_DocumentWindows)
  {
#if XII_DISABLED(XII_PLATFORM_OSX)
    pDoc->menuWidget()->setVisible(pDoc->window() != this);
#endif
    UpdateWindowDecoration(pDoc);
  }
  UpdateDocumentMenu();
}

void xiiQtContainerWindow::UpdateWindowDecoration(xiiQtDocumentWindow* pDocWindow)
{
  const xiiUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == xiiInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  dock->setTabToolTip(xiiMakeQString(pDocWindow->GetDisplayName()));
  dock->setIcon(xiiQtUiServices::GetCachedIconResource(pDocWindow->GetWindowIcon()));
  dock->setWindowTitle(xiiMakeQString(pDocWindow->GetDisplayNameShort()));

  // this is a hacky way to detect the xiiQtSettingsTab
  if (pDocWindow->GetDisplayNameShort().IsEmpty())
  {
    dock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    dock->setFeature(ads::CDockWidget::DockWidgetMovable, false);
    dock->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    dock->setFeature(ads::CDockWidget::NoTab, true);
  }

  if (dock->isFloating())
  {
    dock->dockContainer()->floatingWidget()->setWindowTitle(dock->windowTitle());
    dock->dockContainer()->floatingWidget()->setWindowIcon(dock->icon());
  }
}

void xiiQtContainerWindow::RemoveDocumentWindow(xiiQtDocumentWindow* pDocWindow)
{
  const xiiUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == xiiInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  int iCurIdx = -1;

  const bool            bIsTabbed = dock->isTabbed();
  ads::CDockAreaWidget* pDockArea = dock->dockAreaWidget();

  iCurIdx = pDockArea->currentIndex();

  m_pDockManager->removeDockWidget(dock);

  m_DocumentWindows.RemoveAtAndSwap(uiListIndex);
  m_DocumentDocks.RemoveAtAndSwap(uiListIndex);
  XII_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());
  dock->hide();
  dock->deleteLater();
  pDocWindow->m_pContainerWindow = nullptr;

  if (bIsTabbed)
  {
    iCurIdx = xiiMath::Min(iCurIdx, pDockArea->openDockWidgetsCount() - 1);
    pDockArea->setCurrentIndex(iCurIdx);
    pDockArea->currentDockWidget()->update();
  }

  if (pDockArea && pDockArea->openDockWidgetsCount() == 1)
  {
    for (auto pDocWindow2 : m_DocumentWindows)
    {
      UpdateWindowDecoration(pDocWindow);
    }
  }
}

void xiiQtContainerWindow::RemoveApplicationPanel(xiiQtApplicationPanel* pPanel)
{
  const auto uiListIndex = m_ApplicationPanels.IndexOf(pPanel);

  if (uiListIndex == xiiInvalidIndex)
    return;

  m_pDockManager->removeDockWidget(pPanel);
  m_ApplicationPanels.RemoveAtAndSwap(uiListIndex);

  pPanel->m_pContainerWindow = nullptr;
}

void xiiQtContainerWindow::AddDocumentWindow(xiiQtDocumentWindow* pDocWindow)
{
  XII_ASSERT_DEV(!pDocWindow->objectName().isEmpty(), "Panel name must be unique and not empty.");

  if (m_DocumentWindows.IndexOf(pDocWindow) != xiiInvalidIndex)
    return;

  XII_ASSERT_DEV(pDocWindow->m_pContainerWindow == nullptr, "Implementation error");

  // NOTE: This function is called by the xiiQtDocumentWindow constructor that means any derived classes are not yet constructed!
  // Therefore, calling virtual functions here, like GetDisplayNameShort() will still call the base class implementation, NOT the derived one!
  // Thus, we do some stuff in xiiQtContainerWindow::UpdateWindowDecoration() instead.

  pDocWindow->m_pContainerWindow = this;

  xiiString         sDisplayName = pDocWindow->GetDisplayNameShort();
  ads::CDockWidget* dock         = new ads::CDockWidget(m_pDockManager, xiiMakeQString(sDisplayName));

  dock->installEventFilter(pDocWindow);
  dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
  dock->setObjectName(xiiMakeQString(pDocWindow->GetUniqueName()));

  XII_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  XII_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");

  m_DockNames.insert(dock->objectName());

  // Register the pair before docking can synchronously dispatch focus/show
  // events. Menu discovery must always observe matching document/dock arrays.
  m_DocumentWindows.PushBack(pDocWindow);
  m_DocumentDocks.PushBack(dock);
  dock->setWidget(pDocWindow);
  dock->tabWidget()->setContextMenuPolicy(Qt::CustomContextMenu);

  if (m_DocumentDocks.GetCount() > 1)
  {
    ads::CDockAreaWidget* dockArea = m_DocumentDocks[m_DocumentDocks.GetCount() - 2]->dockAreaWidget();
    m_pDockManager->addDockWidgetTabToArea(dock, dockArea);
  }
  else
  {
    m_pDockManager->addDockWidgetTab(ads::TopDockWidgetArea, dock);
  }
  pDocWindow->menuWidget()->installEventFilter(this);

  connect(dock, &ads::CDockWidget::closeRequested, this, &xiiQtContainerWindow::SlotDocumentTabCloseRequested);
  connect(dock->tabWidget(), &QWidget::customContextMenuRequested, this, &xiiQtContainerWindow::SlotTabsContextMenuRequested);
  connect(dock, &ads::CDockWidget::topLevelChanged, this, &xiiQtContainerWindow::SlotDockWidgetFloatingChanged);

  pDocWindow->m_pContainerWindow = this;

  // We cannot call virtual functions on pDocWindow here, because the object might still be under construction so we delay it until later.
  QMetaObject::invokeMethod(this, "SlotUpdateWindowDecoration", Qt::ConnectionType::QueuedConnection, Q_ARG(void*, pDocWindow));
}

void xiiQtContainerWindow::DocumentWindowRenamed(xiiQtDocumentWindow* pDocWindow)
{
  const xiiUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == xiiInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];
  XII_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());

  dock->setObjectName(xiiMakeQString(pDocWindow->GetUniqueName()));
  XII_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  XII_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
}

void xiiQtContainerWindow::AddApplicationPanel(xiiQtApplicationPanel* pPanel)
{
  // panel already in container window ?
  if (m_ApplicationPanels.IndexOf(pPanel) != xiiInvalidIndex)
    return;

  XII_ASSERT_DEV(!pPanel->objectName().isEmpty(), "Dock name must not be empty.");
  XII_ASSERT_DEV(!m_DockNames.contains(pPanel->objectName()), "Dock name must be unique.");
  m_DockNames.insert(pPanel->objectName());
  XII_ASSERT_DEV(pPanel->m_pContainerWindow == nullptr, "Implementation error");

  m_ApplicationPanels.PushBack(pPanel);
  pPanel->m_pContainerWindow = this;
  m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
}

xiiResult xiiQtContainerWindow::EnsureVisible(xiiQtDocumentWindow* pDocWindow)
{
  const auto uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (uiListIndex == xiiInvalidIndex)
    return XII_FAILURE;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  if (!m_bWindowPresentationEnabled)
  {
    if (auto* pArea = dock->dockAreaWidget())
      pArea->setCurrentDockWidget(dock);
    return XII_SUCCESS;
  }
  dock->toggleView(true);
  return XII_SUCCESS;
}

xiiResult xiiQtContainerWindow::EnsureVisible(xiiDocument* pDocument)
{
  for (auto doc : m_DocumentWindows)
  {
    if (doc->GetDocument() == pDocument)
      return EnsureVisible(doc);
  }
  return XII_FAILURE;
}

xiiResult xiiQtContainerWindow::EnsureVisible(xiiQtApplicationPanel* pPanel)
{
  if (m_ApplicationPanels.IndexOf(pPanel) == xiiInvalidIndex)
    return XII_FAILURE;

  if (!m_bWindowPresentationEnabled)
  {
    if (auto* pArea = pPanel->dockAreaWidget())
      pArea->setCurrentDockWidget(pPanel);
    return XII_SUCCESS;
  }

  // ADS also expands an open but collapsed auto-hide drawer through toggleView(true).
  pPanel->toggleView(true);
  pPanel->raise();
  return XII_SUCCESS;
}

xiiResult xiiQtContainerWindow::EnsureVisibleAnyContainer(xiiDocument* pDocument)
{
  // make sure there is a window to make visible in the first place
  pDocument->GetDocumentManager()->EnsureWindowRequested(pDocument);

  if (s_pContainerWindow->EnsureVisible(pDocument).Succeeded())
    return XII_SUCCESS;

  return XII_FAILURE;
}

void xiiQtContainerWindow::GetDocumentWindows(xiiHybridArray<xiiQtDocumentWindow*, 16>& ref_windows)
{
  ref_windows = m_DocumentWindows;
}

bool xiiQtContainerWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (obj == m_pMenuSource && (e->type() == QEvent::ActionAdded || e->type() == QEvent::ActionRemoved))
    UpdateDocumentMenu();
  if (e->type() == QEvent::Type::Close)
  {
    if (auto* pFloatingWidget = qobject_cast<ads::CFloatingDockContainer*>(obj))
    {
      xiiHybridArray<xiiDocument*, 32> docs;
      docs.Reserve(m_DocumentWindows.GetCount());
      xiiHybridArray<xiiQtDocumentWindow*, 32> windows;
      windows.Reserve(m_DocumentWindows.GetCount());

      QList<ads::CDockWidget*> floatingDocks = pFloatingWidget->dockWidgets();
      for (xiiUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
      {
        if (floatingDocks.contains(m_DocumentDocks[i]))
        {
          docs.PushBack(m_DocumentWindows[i]->GetDocument());
          windows.PushBack(m_DocumentWindows[i]);
        }
      }

      if (!xiiToolsProject::CanCloseDocuments(docs))
      {
        e->setAccepted(false);
        return true;
      }

      // Closing a non-main window should close all documents as well this will remove them from the recently-open documents list and not restore them next time.
      for (xiiQtDocumentWindow* pWindow : windows)
      {
        pWindow->CloseDocumentWindow();
      }
      // This is necessary to clean up some 'delete later' Qt objects before the document is closed as they need to remove their references to the doc.
      qApp->processEvents();
    }
  }
  return false;
}

void xiiQtContainerWindow::SlotDocumentTabCloseRequested()
{
  auto       dock        = qobject_cast<ads::CDockWidget*>(sender());
  const auto uiListIndex = m_DocumentDocks.IndexOf(dock);
  XII_ASSERT_DEV(uiListIndex != xiiInvalidIndex, "Can't close non-existing document.");

  xiiQtDocumentWindow* pDocWindow = m_DocumentWindows[uiListIndex];

  if (!pDocWindow->CanCloseWindow())
  {
    return;
  }

  pDocWindow->CloseDocumentWindow();
}

void xiiQtContainerWindow::DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e)
{
  switch (e.m_Type)
  {
    case xiiQtDocumentWindowEvent::Type::WindowClosing:
      RemoveDocumentWindow(e.m_pWindow);
      break;
    case xiiQtDocumentWindowEvent::Type::WindowDecorationChanged:
      UpdateWindowDecoration(e.m_pWindow);
      break;

    default:
      break;
  }
}

void xiiQtContainerWindow::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectOpened:
    case xiiToolsProjectEvent::Type::ProjectClosed:
      UpdateWindowTitle();
      break;

    default:
      break;
  }
}

void xiiQtContainerWindow::UIServicesEventHandler(const xiiQtUiServices::Event& e)
{
  switch (e.m_Type)
  {
    case xiiQtUiServices::Event::Type::ShowGlobalStatusBarText:
    {
      if (statusBar() == nullptr)
        setStatusBar(new QStatusBar());

      if (m_pStatusBarLabel == nullptr)
      {
        m_pStatusBarLabel = new QLabel();
        statusBar()->addWidget(m_pStatusBarLabel);

        QPalette pal = m_pStatusBarLabel->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::red));
        m_pStatusBarLabel->setPalette(pal);
      }

      statusBar()->setHidden(e.m_sText.IsEmpty());

      m_pStatusBarLabel->setText(xiiMakeQString(e.m_sText));
    }
    break;

    default:
      break;
  }
}

void xiiQtContainerWindow::SlotTabsContextMenuRequested(const QPoint& pos)
{
  auto              tab         = qobject_cast<ads::CDockWidgetTab*>(sender());
  ads::CDockWidget* dock        = tab->dockWidget();
  const auto        uiListIndex = m_DocumentDocks.IndexOf(dock);
  XII_ASSERT_DEV(uiListIndex != xiiInvalidIndex, "Can't close non-existing document.");

  xiiQtDocumentWindow* pDoc = m_DocumentWindows[uiListIndex];
  pDoc->RequestWindowTabContextMenu(tab->mapToGlobal(pos));
}
