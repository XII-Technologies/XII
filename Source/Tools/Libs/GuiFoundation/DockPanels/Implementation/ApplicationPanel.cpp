#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <ads/DockAreaWidget.h>
#include <ads/DockContainerWidget.h>
#include <ads/DockWidgetTab.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiQtApplicationPanel, xiiNoBase, 1, xiiRTTINoAllocator)
XII_END_STATIC_REFLECTED_TYPE;

xiiDynamicArray<xiiQtApplicationPanel*> xiiQtApplicationPanel::s_AllApplicationPanels;

xiiQtApplicationPanel::xiiQtApplicationPanel(ads::CDockManager* pDockManager, xiiStringView sPanelName) :
  ads::CDockWidget(pDockManager, xiiMakeQString(sPanelName), xiiQtContainerWindow::GetContainerWindow())
{
  xiiStringBuilder sPanel("AppPanel_", sPanelName);

  setObjectName(xiiMakeQString(sPanel.GetView()));
  setWindowTitle(xiiMakeQString(xiiTranslate(sPanelName)));

  s_AllApplicationPanels.PushBack(this);

  m_pContainerWindow = nullptr;

  xiiQtContainerWindow::GetContainerWindow()->AddApplicationPanel(this);

  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtApplicationPanel::ToolsProjectEventHandler, this));
}

xiiQtApplicationPanel::~xiiQtApplicationPanel()
{
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtApplicationPanel::ToolsProjectEventHandler, this));

  s_AllApplicationPanels.RemoveAndSwap(this);
}

void xiiQtApplicationPanel::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();

  QWidget* pThis = this;

  if (dockAreaWidget())
  {
    dockAreaWidget()->setCurrentDockWidget(this);
  }

  while (pThis)
  {
    pThis->raise();
    pThis = qobject_cast<QWidget*>(pThis->parent());
  }
}


void xiiQtApplicationPanel::ToolsProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectClosing:
      setEnabled(false);
      break;
    case xiiToolsProjectEvent::Type::ProjectOpened:
      setEnabled(true);
      break;

    default:
      break;
  }
}

bool xiiQtApplicationPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (xiiQtProxy::TriggerDocumentAction(nullptr, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }
  return ads::CDockWidget::event(pEvent);
}
