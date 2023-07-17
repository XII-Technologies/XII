#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <ToolsFoundation/Document/Document.h>
#include <ads/DockWidget.h>

xiiEvent<const xiiQtDocumentWindowEvent&> xiiQtDocumentWindow::s_Events;
xiiDynamicArray<xiiQtDocumentWindow*>     xiiQtDocumentWindow::s_AllDocumentWindows;
bool                                      xiiQtDocumentWindow::s_bAllowRestoreWindowLayout = true;

void xiiQtDocumentWindow::Constructor()
{
  s_AllDocumentWindows.PushBack(this);

  // status bar
  {
    connect(statusBar(), &QStatusBar::messageChanged, this, &xiiQtDocumentWindow::OnStatusBarMessageChanged);

    m_pPermanentDocumentStatusText = new QLabel();
    statusBar()->addWidget(m_pPermanentDocumentStatusText, 1);

    m_pPermanentGlobalStatusButton = new QToolButton();
    m_pPermanentGlobalStatusButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pPermanentGlobalStatusButton->setVisible(false);
    statusBar()->addPermanentWidget(m_pPermanentGlobalStatusButton, 0);

    XII_VERIFY(connect(m_pPermanentGlobalStatusButton, &QToolButton::clicked, this, &xiiQtDocumentWindow::OnPermanentGlobalStatusClicked), "");
  }

  setDockNestingEnabled(true);

  xiiQtMenuBarActionMapView* pMenuBar = new xiiQtMenuBarActionMapView(this);
  setMenuBar(pMenuBar);

  xiiInt32              iContainerWindowIndex = xiiToolsProject::SuggestContainerWindow(m_pDocument);
  xiiQtContainerWindow* pContainer            = xiiQtContainerWindow::GetContainerWindow();
  pContainer->AddDocumentWindow(this);

  xiiQtUiServices::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::UIServicesEventHandler, this));
  xiiQtUiServices::s_TickEvent.AddEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::UIServicesTickEventHandler, this));
}

xiiQtDocumentWindow::xiiQtDocumentWindow(xiiDocument* pDocument)
{
  m_pDocument   = pDocument;
  m_sUniqueName = m_pDocument->GetDocumentPath();

  xiiStringBuilder tmp;
  setObjectName(GetUniqueName().GetData(tmp));

  xiiDocumentManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::DocumentManagerEventHandler, this));
  pDocument->m_EventsOne.AddEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::DocumentEventHandler, this));

  Constructor();
}

xiiQtDocumentWindow::xiiQtDocumentWindow(xiiStringView sUniqueName)
{
  m_pDocument   = nullptr;
  m_sUniqueName = sUniqueName;

  xiiStringBuilder tmp;
  setObjectName(GetUniqueName().GetData(tmp));

  Constructor();
}


xiiQtDocumentWindow::~xiiQtDocumentWindow()
{
  xiiQtUiServices::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::UIServicesEventHandler, this));
  xiiQtUiServices::s_TickEvent.RemoveEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::UIServicesTickEventHandler, this));

  s_AllDocumentWindows.RemoveAndSwap(this);

  if (m_pDocument)
  {
    m_pDocument->m_EventsOne.RemoveEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::DocumentEventHandler, this));
    xiiDocumentManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtDocumentWindow::DocumentManagerEventHandler, this));
  }
}

void xiiQtDocumentWindow::SetVisibleInContainer(bool bVisible)
{
  if (m_bIsVisibleInContainer == bVisible)
    return;

  m_bIsVisibleInContainer = bVisible;
  InternalVisibleInContainerChanged(bVisible);

  if (m_bIsVisibleInContainer)
  {
    // if the window is now visible, immediately do a redraw and trigger the timers
    SlotRedraw();
    // Make sure the window gains focus as well when it becomes visible so that shortcuts will immediately work.
    setFocus();
  }
}

void xiiQtDocumentWindow::SetTargetFramerate(xiiInt16 iTargetFPS)
{
  if (m_iTargetFramerate == iTargetFPS)
    return;

  m_iTargetFramerate = iTargetFPS;

  if (m_iTargetFramerate != 0)
    SlotRedraw();
}

void xiiQtDocumentWindow::TriggerRedraw()
{
  SlotRedraw();
}

void xiiQtDocumentWindow::UIServicesTickEventHandler(const xiiQtUiServices::TickEvent& e)
{
  if (e.m_Type == xiiQtUiServices::TickEvent::Type::StartFrame && m_bIsVisibleInContainer)
  {
    const xiiInt32 iSystemFramerate = static_cast<xiiInt32>(xiiMath::Round(e.m_fRefreshRate));

    xiiInt32 iTargetFramerate = m_iTargetFramerate;
    if (iTargetFramerate <= 0)
      iTargetFramerate = iSystemFramerate;

    // if the application does not have focus, drastically reduce the update rate to limit CPU draw etc.
    if (QApplication::activeWindow() == nullptr)
      iTargetFramerate = xiiMath::Min(10, iTargetFramerate / 4);

    // We do not hit the requested framerate directly if the system framerate can't be evenly divided. We will chose the next higher framerate.
    if (iTargetFramerate < iSystemFramerate)
    {
      xiiUInt32 mod = xiiMath::Max(1u, (xiiUInt32)xiiMath::Floor(iSystemFramerate / (double)iTargetFramerate));
      if ((e.m_uiFrame % mod) != 0)
        return;
    }

    SlotRedraw();
  }
}


void xiiQtDocumentWindow::SlotRedraw()
{
  xiiStringBuilder sFilename = xiiPathUtils::GetFileName(this->GetUniqueName());
  XII_PROFILE_SCOPE(sFilename.GetData());
  {
    xiiQtDocumentWindowEvent e;
    e.m_Type    = xiiQtDocumentWindowEvent::Type::BeforeRedraw;
    e.m_pWindow = this;
    s_Events.Broadcast(e, 1);
  }

  // if our window is not visible, interrupt the redrawing, and do nothing
  if (!m_bIsVisibleInContainer)
    return;

  m_bIsDrawingATM = true;
  InternalRedraw();
  m_bIsDrawingATM = false;
}

void xiiQtDocumentWindow::DocumentEventHandler(const xiiDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentEvent::Type::ModifiedChanged:
    {
      xiiQtDocumentWindowEvent dwe;
      dwe.m_pWindow = this;
      dwe.m_Type    = xiiQtDocumentWindowEvent::Type::WindowDecorationChanged;
      s_Events.Broadcast(dwe);
    }
    break;

    case xiiDocumentEvent::Type::EnsureVisible:
    {
      EnsureVisible();
    }
    break;

    case xiiDocumentEvent::Type::DocumentStatusMsg:
    {
      ShowTemporaryStatusBarMsg(e.m_sStatusMsg);
    }
    break;

    default:
      break;
  }
}

void xiiQtDocumentWindow::DocumentManagerEventHandler(const xiiDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case xiiDocumentManager::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument == m_pDocument)
      {
        ShutdownDocumentWindow();
        return;
      }
    }
    break;

    default:
      break;
  }
}

void xiiQtDocumentWindow::UIServicesEventHandler(const xiiQtUiServices::Event& e)
{
  switch (e.m_Type)
  {
    case xiiQtUiServices::Event::Type::ShowDocumentTemporaryStatusBarText:
      ShowTemporaryStatusBarMsg(xiiFmt(e.m_sText), e.m_Time);
      break;

    case xiiQtUiServices::Event::Type::ShowDocumentPermanentStatusBarText:
    {
      if (m_pPermanentGlobalStatusButton)
      {
        QPalette pal = palette();

        switch (e.m_TextType)
        {
          case xiiQtUiServices::Event::Info:
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Log.png"));
            break;

          case xiiQtUiServices::Event::Warning:
            pal.setColor(QPalette::WindowText, QColor(255, 100, 0));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Warning16.png"));
            break;

          case xiiQtUiServices::Event::Error:
            pal.setColor(QPalette::WindowText, QColor(Qt::red));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Error16.png"));
            break;
        }

        m_pPermanentGlobalStatusButton->setPalette(pal);
        m_pPermanentGlobalStatusButton->setText(QString::fromUtf8(e.m_sText, e.m_sText.GetElementCount()));
        m_pPermanentGlobalStatusButton->setVisible(!m_pPermanentGlobalStatusButton->text().isEmpty());
      }
    }
    break;

    default:
      break;
  }
}

xiiString xiiQtDocumentWindow::GetDisplayNameShort() const
{
  xiiStringBuilder s = GetDisplayName();
  s                  = s.GetFileName();

  if (m_pDocument && m_pDocument->IsModified())
    s.Append('*');

  return s;
}

void xiiQtDocumentWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);
  SetVisibleInContainer(true);
}

void xiiQtDocumentWindow::hideEvent(QHideEvent* event)
{
  QMainWindow::hideEvent(event);
  SetVisibleInContainer(false);
}

bool xiiQtDocumentWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::ShortcutOverride)
  {
    // This filter is added by xiiQtContainerWindow::AddDocumentWindow as that ones is the ony code path that can connect dock container to their content.
    // This filter is necessary as clicking any action in a menu bar sets the focus to the parent CDockWidget at which point further shortcuts would stop working.
    if (qobject_cast<ads::CDockWidget*>(obj))
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
      if (xiiQtProxy::TriggerDocumentAction(m_pDocument, keyEvent))
        return true;
    }
  }
  return false;
}

bool xiiQtDocumentWindow::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (xiiQtProxy::TriggerDocumentAction(m_pDocument, keyEvent))
      return true;
  }
  return QMainWindow::event(event);
}

void xiiQtDocumentWindow::FinishWindowCreation()
{
  ScheduleRestoreWindowLayout();
}

void xiiQtDocumentWindow::ScheduleRestoreWindowLayout()
{
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void xiiQtDocumentWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void xiiQtDocumentWindow::SaveWindowLayout()
{
  // This is a workaround for newer Qt versions (5.13 or so) that seem to change the state of QDockWidgets to "closed" once the parent
  // QMainWindow gets the closeEvent, even though they still exist and the QMainWindow is not yet deleted. Previously this function was
  // called multiple times, including once after the QMainWindow got its closeEvent, which would then save a corrupted state. Therefore,
  // once the parent xiiQtContainerWindow gets the closeEvent, we now prevent further saving of the window layout.
  if (!m_bAllowSaveWindowLayout)
    return;

  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  xiiStringBuilder sGroup;
  sGroup.Format("DocumentWnd_{0}", GetWindowLayoutGroupName());

  QSettings Settings;
  Settings.beginGroup(QString::fromUtf8(sGroup, sGroup.GetElementCount()));
  {
    // All other properties are defined by the outer container window.
    Settings.setValue("WindowState", saveState());
  }
  Settings.endGroup();
}

void xiiQtDocumentWindow::RestoreWindowLayout()
{
  if (!s_bAllowRestoreWindowLayout)
    return;

  xiiQtScopedUpdatesDisabled _(this);

  xiiStringBuilder sGroup;
  sGroup.Format("DocumentWnd_{0}", GetWindowLayoutGroupName());

  {
    QSettings Settings;
    Settings.beginGroup(QString::fromUtf8(sGroup, sGroup.GetElementCount()));
    {
      restoreState(Settings.value("WindowState", saveState()).toByteArray());
    }
    Settings.endGroup();

    // with certain Qt versions the window state could be saved corrupted
    // if that is the case, make sure that non-closable widgets get restored to be visible
    // otherwise the user would need to delete the serialized state from the registry
    {
      for (QDockWidget* dockWidget : findChildren<QDockWidget*>())
      {
        // not closable means the user can generally not change the visible state -> make sure it is visible
        if (!dockWidget->features().testFlag(QDockWidget::DockWidgetClosable) && dockWidget->isHidden())
        {
          dockWidget->show();
        }
      }
    }
  }

  statusBar()->clearMessage();
}

void xiiQtDocumentWindow::DisableWindowLayoutSaving()
{
  m_bAllowSaveWindowLayout = false;
}

xiiStatus xiiQtDocumentWindow::SaveDocument()
{
  if (m_pDocument)
  {
    {
      if (m_pDocument->GetUnknownObjectTypeInstances() > 0)
      {
        if (xiiQtUiServices::MessageBoxQuestion("Warning! This document contained unknown object types that could not be loaded. Saving the "
                                                "document means those objects will get lost permanently.\n\nDo you really want to save this "
                                                "document?",
                                                QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
          return xiiStatus(XII_SUCCESS); // failed successfully
      }
    }

    xiiStatus res = m_pDocument->SaveDocument();

    xiiStringBuilder s, s2;
    s.Format("Failed to save document:\n'{0}'", m_pDocument->GetDocumentPath());
    s2.Format("Successfully saved document:\n'{0}'", m_pDocument->GetDocumentPath());

    xiiQtUiServices::MessageBoxStatus(res, s, s2);

    if (res.m_Result.Failed())
    {
      ShowTemporaryStatusBarMsg("Failed to save document");
      return res;
    }

    ShowTemporaryStatusBarMsg("Document saved");
  }

  return xiiStatus(XII_SUCCESS);
}

void xiiQtDocumentWindow::ShowTemporaryStatusBarMsg(const xiiFormatString& msg, xiiTime duration)
{
  xiiStringBuilder tmp;
  statusBar()->showMessage(QString::fromUtf8(msg.GetTextCStr(tmp)), (int)duration.GetMilliseconds());
}


void xiiQtDocumentWindow::SetPermanentStatusBarMsg(const xiiFormatString& text)
{
  if (!text.IsEmpty())
  {
    // clear temporary message
    statusBar()->clearMessage();
  }

  xiiStringBuilder tmp;
  m_pPermanentDocumentStatusText->setText(QString::fromUtf8(text.GetTextCStr(tmp)));
}

void xiiQtDocumentWindow::CreateImageCapture(xiiStringView sOutputPath)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

bool xiiQtDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool xiiQtDocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  if (m_pDocument && m_pDocument->IsModified())
  {
    QMessageBox::StandardButton res = xiiQtUiServices::MessageBoxQuestion("Save before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      xiiStatus err = SaveDocument();

      if (err.Failed())
      {
        xiiQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
    }
  }

  return true;
}

void xiiQtDocumentWindow::CloseDocumentWindow()
{
  QMetaObject::invokeMethod(this, "SlotQueuedDelete", Qt::ConnectionType::QueuedConnection);
}

void xiiQtDocumentWindow::SlotQueuedDelete()
{
  setFocus();
  clearFocus();

  if (m_pDocument)
  {
    m_pDocument->GetDocumentManager()->CloseDocument(m_pDocument);
    return;
  }
  else
  {
    ShutdownDocumentWindow();
  }
}

void xiiQtDocumentWindow::OnPermanentGlobalStatusClicked(bool)
{
  xiiQtUiServices::Event e;
  e.m_Type = xiiQtUiServices::Event::ClickedDocumentPermanentStatusBarText;

  xiiQtUiServices::GetSingleton()->s_Events.Broadcast(e);
}

void xiiQtDocumentWindow::OnStatusBarMessageChanged(const QString& sNewText)
{
  QPalette pal = palette();

  if (sNewText.startsWith("Error:"))
  {
    pal.setColor(QPalette::WindowText, xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Red)));
  }
  else if (sNewText.startsWith("Warning:"))
  {
    pal.setColor(QPalette::WindowText, xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Yellow)));
  }
  else if (sNewText.startsWith("Note:"))
  {
    pal.setColor(QPalette::WindowText, xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Blue)));
  }

  statusBar()->setPalette(pal);
}

void xiiQtDocumentWindow::ShutdownDocumentWindow()
{
  SaveWindowLayout();

  InternalCloseDocumentWindow();

  xiiQtDocumentWindowEvent e;
  e.m_pWindow = this;
  e.m_Type    = xiiQtDocumentWindowEvent::Type::WindowClosing;
  s_Events.Broadcast(e);

  InternalDeleteThis();

  e.m_Type = xiiQtDocumentWindowEvent::Type::WindowClosed;
  s_Events.Broadcast(e);
}

void xiiQtDocumentWindow::InternalCloseDocumentWindow() {}

void xiiQtDocumentWindow::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();
}

void xiiQtDocumentWindow::RequestWindowTabContextMenu(const QPoint& globalPos)
{
  xiiQtMenuActionMapView menu(nullptr);

  xiiActionContext context;
  context.m_sMapping  = "DocumentWindowTabMenu";
  context.m_pDocument = GetDocument();
  context.m_pWindow   = this;
  menu.SetActionContext(context);

  menu.exec(globalPos);
}

xiiQtDocumentWindow* xiiQtDocumentWindow::FindWindowByDocument(const xiiDocument* pDocument)
{
  // Sub-documents never have a window, so go to the main document instead
  pDocument = pDocument->GetMainDocument();

  for (auto pWnd : s_AllDocumentWindows)
  {
    if (pWnd->GetDocument() == pDocument)
      return pWnd;
  }

  return nullptr;
}

xiiQtContainerWindow* xiiQtDocumentWindow::GetContainerWindow() const
{
  return m_pContainerWindow;
}

xiiString xiiQtDocumentWindow::GetWindowIcon() const
{
  if (GetDocument() != nullptr)
    return GetDocument()->GetDocumentTypeDescriptor()->m_sIcon;

  return ":/GuiFoundation/XII-Logo.svg";
}
