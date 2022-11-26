#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCloseEvent>

xiiDynamicArray<xiiQtDocumentPanel*> xiiQtDocumentPanel::s_AllDocumentPanels;

xiiQtDocumentPanel::xiiQtDocumentPanel(QWidget* parent, xiiDocument* pDocument) :
  QDockWidget(parent)
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

bool xiiQtDocumentPanel::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (xiiQtProxy::TriggerDocumentAction(m_pDocument, keyEvent))
      return true;
  }
  return QDockWidget::event(event);
}
