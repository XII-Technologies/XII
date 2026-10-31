/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/WindowTitleBar.h>
#include <QMouseEvent>
#include <QWindow>

xiiQtWindowTitleBar::xiiQtWindowTitleBar(QWidget* pWindow) :
  QToolBar(tr("Workspace title bar"), pWindow)
{
  setObjectName(QStringLiteral("EditorTitleBar"));
  setMovable(false);
  setFloatable(false);
  setMinimumHeight(qMax(36, fontMetrics().height() + 14));
  setStyleSheet(QStringLiteral("QToolBar#EditorTitleBar { background: palette(window); border: 0; border-bottom: 1px solid palette(mid); }"));
  setContentsMargins(4, 0, 4, 0);
}

bool xiiQtWindowTitleBar::IsCaptionPosition(const QPoint& position) const
{
  // childAt honors WA_TransparentForMouseEvents on the brand, project label and
  // reserved caption spacer. Never turn an ignored control event into a drag.
  return rect().contains(position) && childAt(position) == nullptr;
}

void xiiQtWindowTitleBar::changeEvent(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::FontChange)
    setMinimumHeight(qMax(36, fontMetrics().height() + 14));
  QToolBar::changeEvent(pEvent);
}

void xiiQtWindowTitleBar::mousePressEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::LeftButton && IsCaptionPosition(pEvent->position().toPoint()) && !window()->isFullScreen() &&
      window()->windowHandle() && window()->windowHandle()->startSystemMove())
  {
    pEvent->accept();
    return;
  }
  QToolBar::mousePressEvent(pEvent);
}

void xiiQtWindowTitleBar::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::LeftButton && IsCaptionPosition(pEvent->position().toPoint()) && !window()->isFullScreen() &&
      window()->windowFlags().testFlag(Qt::WindowMaximizeButtonHint))
  {
    window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
    pEvent->accept();
    return;
  }
  QToolBar::mouseDoubleClickEvent(pEvent);
}
