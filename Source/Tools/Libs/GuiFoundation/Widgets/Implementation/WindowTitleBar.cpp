/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/WindowTitleBar.h>
#include <QMouseEvent>
#include <QToolButton>
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

void xiiQtWindowTitleBar::AddWindowControls()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  auto AddButton = [this](const QString& sName, const QString& sText, const QString& sToolTip) {
    auto* pButton = new QToolButton(this);
    pButton->setObjectName(sName);
    pButton->setText(sText);
    pButton->setToolTip(sToolTip);
    pButton->setAccessibleName(sToolTip);
    pButton->setFixedWidth(36);
    pButton->setAutoRaise(true);
    addWidget(pButton);
    return pButton;
  };
  addSeparator();
  auto* pMinimize = AddButton(QStringLiteral("EditorMinimize"), QStringLiteral("\u2212"), tr("Minimize"));
  connect(pMinimize, &QToolButton::clicked, window(), &QWidget::showMinimized);
  m_pMaximizeButton = AddButton(QStringLiteral("EditorMaximize"), QStringLiteral("\u25a1"), tr("Maximize"));
  connect(m_pMaximizeButton, &QToolButton::clicked, this, [this]() { window()->isMaximized() ? window()->showNormal() : window()->showMaximized(); });
  auto* pClose = AddButton(QStringLiteral("EditorClose"), QStringLiteral("\u00d7"), tr("Close editor"));
  pClose->setStyleSheet(QStringLiteral("QToolButton:hover { background: #a52b32; color: white; }"));
  connect(pClose, &QToolButton::clicked, window(), &QWidget::close);
  window()->installEventFilter(this);
#endif
}

bool xiiQtWindowTitleBar::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == window() && pEvent->type() == QEvent::WindowStateChange && m_pMaximizeButton)
  {
    const auto sTitle = window()->isMaximized() ? tr("Restore") : tr("Maximize");
    m_pMaximizeButton->setText(window()->isMaximized() ? QStringLiteral("\u2750") : QStringLiteral("\u25a1"));
    m_pMaximizeButton->setToolTip(sTitle);
    m_pMaximizeButton->setAccessibleName(sTitle);
  }
  return QToolBar::eventFilter(pObject, pEvent);
}

void xiiQtWindowTitleBar::resizeEvent(QResizeEvent* pEvent)
{
  for (auto* pButton : findChildren<QToolButton*>())
    if (pButton->objectName() == QStringLiteral("EditorFindCommand") || pButton->objectName() == QStringLiteral("EditorWorkspaces"))
      pButton->setToolButtonStyle(width() < 1100 ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
  QToolBar::resizeEvent(pEvent);
}

void xiiQtWindowTitleBar::changeEvent(QEvent* pEvent)
{
  if (pEvent->type() == QEvent::FontChange)
    setMinimumHeight(qMax(36, fontMetrics().height() + 14));
  QToolBar::changeEvent(pEvent);
}

void xiiQtWindowTitleBar::mousePressEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::LeftButton && window()->windowHandle() && window()->windowHandle()->startSystemMove())
  {
    pEvent->accept();
    return;
  }
  QToolBar::mousePressEvent(pEvent);
}

void xiiQtWindowTitleBar::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::LeftButton && !window()->isFullScreen())
  {
    window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
    pEvent->accept();
    return;
  }
  QToolBar::mouseDoubleClickEvent(pEvent);
}
