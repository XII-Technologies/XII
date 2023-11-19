#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

xiiDynamicArray<xiiQtDocumentPanel*> xiiQtDocumentPanel::s_AllDocumentPanels;

xiiQtDocumentPanel::xiiQtDocumentPanel(QWidget* pParent, xiiDocument* pDocument) :
  QDockWidget(pParent)
{
  m_pDocument = pDocument;
  s_AllDocumentPanels.PushBack(this);

  setBackgroundRole(QPalette::ColorRole::Highlight);

  setFeatures(DockWidgetFeature::DockWidgetFloatable | DockWidgetFeature::DockWidgetMovable);
}

xiiQtDocumentPanel::~xiiQtDocumentPanel()
{
  s_AllDocumentPanels.RemoveAndSwap(this);
}

void xiiQtDocumentPanel::closeEvent(QCloseEvent* e)
{
  e->ignore();
}

bool xiiQtDocumentPanel::event(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::ShortcutOverride || pEvent->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
    if (xiiQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, pEvent->type() == QEvent::ShortcutOverride))
      return true;
  }
  return QDockWidget::event(pEvent);
}
