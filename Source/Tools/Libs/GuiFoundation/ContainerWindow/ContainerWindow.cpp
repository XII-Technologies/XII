#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <QCloseEvent>
#include <QLabel>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTimer>
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
  setMinimumSize(QSize(800, 600));
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

  ads::CDockManager::ConfigFlags flags =
    ads::CDockManager::DefaultDockAreaButtons |
    ads::CDockManager::ActiveTabHasCloseButton |
    ads::CDockManager::XmlCompressionEnabled |
    ads::CDockManager::FloatingContainerHasWidgetTitle |
    ads::CDockManager::DragPreviewShowsContentPixmap |
    ads::CDockManager::FocusHighlighting |
    ads::CDockManager::AlwaysShowTabs |
    ads::CDockManager::DockAreaHasCloseButton |
    ads::CDockManager::DockAreaCloseButtonClosesTab |
    ads::CDockManager::MiddleMouseButtonClosesTab |
    ads::CDockManager::DockAreaHasTabsMenuButton |
    ads::CDockManager::FloatingContainerHasWidgetIcon |
    ads::CDockManager::AllTabsHaveCloseButton |
    ads::CDockManager::OpaqueSplitterResize;
  ads::CDockManager::setConfigFlags(flags);

  m_pDockManager = new ads::CDockManager(this);

  connect(m_pDockManager, &ads::CDockManager::floatingWidgetCreated, this, &xiiQtContainerWindow::SlotFloatingWidgetOpened);
}

xiiQtContainerWindow::~xiiQtContainerWindow()
{
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

  setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void xiiQtContainerWindow::ScheduleRestoreWindowLayout()
{
  m_iWindowLayoutRestoreScheduled++;
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void xiiQtContainerWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void xiiQtContainerWindow::closeEvent(QCloseEvent* e)
{
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

  // We need to destroy the dock manager here, doing it in the constructor leads to an access violation.
  m_pDockManager->deleteLater();
  m_pDockManager = nullptr;
  QMainWindow::closeEvent(e);
}

void xiiQtContainerWindow::SaveWindowLayout()
{
  if (!m_pDockManager)
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
  for (xiiUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->SaveWindowLayout();
}

void xiiQtContainerWindow::RestoreWindowLayout()
{
  --m_iWindowLayoutRestoreScheduled;
  if (m_iWindowLayoutRestoreScheduled > 0)
    return;

  show();

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
      restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
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
            dock->toggleView();
          }
        }
      }
    }
    Settings.endGroup();
  }

  for (xiiUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->RestoreWindowLayout();

  m_bWindowLayoutRestored = true;
}

void xiiQtContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<xiiQtDocumentWindow*>(pDocWindow));
}

void xiiQtContainerWindow::SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget)
{
  FloatingWidget->installEventFilter(this);
}

void xiiQtContainerWindow::SlotDockWidgetFloatingChanged(bool bFloating)
{
  if (!bFloating)
    return;

  for (auto pDoc : m_DocumentWindows)
  {
    UpdateWindowDecoration(pDoc);
  }
}

void xiiQtContainerWindow::UpdateWindowDecoration(xiiQtDocumentWindow* pDocWindow)
{
  const xiiUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == xiiInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  dock->setTabToolTip(QString::fromUtf8(pDocWindow->GetDisplayName().GetData()));
  dock->setIcon(xiiQtUiServices::GetCachedIconResource(pDocWindow->GetWindowIcon().GetData()));
  dock->setWindowTitle(QString::fromUtf8(pDocWindow->GetDisplayNameShort().GetData()));

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

  // NOTE: This function is called by the xiiQtDocumentWindow constructor
  // that means any derived classes are not yet constructed!
  // therefore calling virtual functions here, like GetDisplayNameShort() will still call
  // the base class implementation, NOT the derived one !
  // therefore, we do some stuff in xiiQtContainerWindow::UpdateWindowDecoration() instead

  m_DocumentWindows.PushBack(pDocWindow);
  xiiString         displayName = pDocWindow->GetDisplayNameShort();
  ads::CDockWidget* dock        = new ads::CDockWidget(QString::fromUtf8(displayName.GetData(), displayName.GetElementCount()));
  dock->installEventFilter(pDocWindow);

  xiiStringBuilder tmp;
  dock->setObjectName(pDocWindow->GetUniqueName().GetData(tmp));
  XII_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  XII_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
  dock->setWidget(pDocWindow);
  dock->tabWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
  if (!m_DocumentDocks.IsEmpty())
  {
    ads::CDockAreaWidget* dockArea = m_DocumentDocks.PeekBack()->dockAreaWidget();
    m_pDockManager->addDockWidgetTabToArea(dock, dockArea);
  }
  else
  {
    m_pDockManager->addDockWidgetTab(ads::TopDockWidgetArea, dock);
  }
  m_DocumentDocks.PushBack(dock);
  connect(dock, &ads::CDockWidget::closed, this, &xiiQtContainerWindow::SlotDocumentTabCloseRequested);
  connect(dock->tabWidget(), &QWidget::customContextMenuRequested, this, &xiiQtContainerWindow::SlotTabsContextMenuRequested);
  connect(dock, &ads::CDockWidget::topLevelChanged, this, &xiiQtContainerWindow::SlotDockWidgetFloatingChanged);


  pDocWindow->m_pContainerWindow = this;

  // we cannot call virtual functions on pDocWindow here, because the object might still be under construction
  // so we delay it until later
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

  xiiStringBuilder tmp;
  dock->setObjectName(pDocWindow->GetUniqueName().GetData(tmp));
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

  if (pPanel->isClosed())
  {
    pPanel->toggleView();
  }
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

      // closing a non-main window should close all documents as well
      // this will remove them from the recently-open documents list and not restore them next time
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
    // TODO: There is no CloseRequested event so we just reopen on a timer.
    QTimer::singleShot(1, [dock]() { dock->toggleView(); });
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
      m_pStatusBarLabel->setText(QString::fromUtf8(e.m_sText.GetData()));
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
