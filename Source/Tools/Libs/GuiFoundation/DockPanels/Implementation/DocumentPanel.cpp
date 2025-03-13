#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

xiiQtDocumentPanel::xiiQtDocumentPanel(ads::CDockManager* pDockManager, QWidget* pParent, xiiDocument* pDocument) :
  ads::CDockWidget(pDockManager, "xiiQtDocumentPanel", pParent)
{
  m_pDocument = pDocument;

  setMinimumWidth(300);
  setMinimumHeight(200);

  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetClosable, false);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFloatable, true);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetMovable, true);
  setFeature(ads::CDockWidget::DockWidgetFeature::DockWidgetFocusable, true);
}

xiiQtDocumentPanel::~xiiQtDocumentPanel() = default;

bool xiiQtDocumentPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (xiiQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }

  return CDockWidget::event(pEvent);
}
