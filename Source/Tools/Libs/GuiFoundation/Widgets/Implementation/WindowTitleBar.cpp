/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/WindowTitleBar.h>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QToolButton>
#include <QWindow>

xiiQtWindowTitleBar::xiiQtWindowTitleBar(QWidget* pWindow) :
  QWidget(pWindow)
{
  setObjectName(QStringLiteral("EditorTitleBar"));
  setAccessibleName(tr("Workspace title bar"));
  setMinimumHeight(qMax(32, fontMetrics().height() + 16));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setStyleSheet(QStringLiteral(R"(
QWidget#EditorTitleBar { background: palette(window); border: 0; border-bottom: 1px solid #2a2e36; }
QWidget#EditorTitleBar QLabel { color: #c2c7d0; background: transparent; border: 0; }
QWidget#EditorTitleBar QLabel#EditorCaptionSpacer { color: #e0e4ec; }
QWidget#EditorTitleBar QMenuBar { color: #dce0e8; background: palette(window); border: 0; spacing: 2px; }
QWidget#EditorTitleBar QMenuBar::item { background: transparent; padding: 2px 8px; border: 0; border-radius: 4px; }
QWidget#EditorTitleBar QMenuBar::item:selected { background: #2b303a; }
QWidget#EditorTitleBar QMenuBar::item:pressed { background: #353d49; }
QWidget#EditorTitleBar QToolButton { color: #dce0e8; background: transparent; border: 0; border-radius: 4px; padding: 2px 8px; }
QWidget#EditorTitleBar QToolButton:hover { background: #2b303a; }
QWidget#EditorTitleBar QToolButton:pressed, QWidget#EditorTitleBar QToolButton:checked { background: #353d49; }
QWidget#EditorTitleBar QToolButton:focus { outline: 1px solid palette(highlight); }
QWidget#EditorTitleBar QToolButton[captionControl="true"] { border-radius: 0; padding: 0; }
QWidget#EditorTitleBar QToolButton#EditorClose:hover { background: #b42c38; }
QWidget#EditorTitleBar QToolButton#EditorClose:pressed { background: #94232d; }
)"));
  auto* pLayout = new QHBoxLayout(this);
  // Leave the upper strip clear for native resizing, including at high DPI.
  pLayout->setContentsMargins(12, 8, 0, 0);
  pLayout->setSpacing(6);
}

void xiiQtWindowTitleBar::AddWindowControls()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  auto* pControls       = new QWidget(this);
  auto* pControlsLayout = new QHBoxLayout(pControls);
  pControlsLayout->setContentsMargins(0, 0, 0, 0);
  pControlsLayout->setSpacing(0);
  static_cast<QHBoxLayout*>(layout())->addWidget(pControls);
  auto AddButton = [this, pControls, pControlsLayout](const QString& sName, const QString& sIcon, const QString& sTitle) {
    auto* pButton = new QToolButton(pControls);
    pButton->setObjectName(sName);
    pButton->setProperty("captionControl", true);
    pButton->setIcon(QIcon(sIcon));
    pButton->setIconSize(QSize(16, 16));
    pButton->setToolTip(sTitle);
    pButton->setAccessibleName(sTitle);
    pButton->setFixedSize(46, qMax(24, fontMetrics().height() + 8));
    pButton->setAutoRaise(true);
    pControlsLayout->addWidget(pButton);
    return pButton;
  };
  auto* pMinimize = AddButton(QStringLiteral("EditorMinimize"), QStringLiteral(":/GuiFoundation/Icons/WindowMinimize.svg"), tr("Minimize"));
  connect(pMinimize, &QToolButton::clicked, window(), &QWidget::showMinimized);
  m_pMaximizeButton = AddButton(QStringLiteral("EditorMaximize"), QStringLiteral(":/GuiFoundation/Icons/WindowMaximize.svg"), tr("Maximize"));
  connect(m_pMaximizeButton, &QToolButton::clicked, this, [this]() { window()->isMaximized() ? window()->showNormal() : window()->showMaximized(); });
  auto* pClose = AddButton(QStringLiteral("EditorClose"), QStringLiteral(":/GuiFoundation/Icons/WindowClose.svg"), tr("Close editor"));
  connect(pClose, &QToolButton::clicked, window(), &QWidget::close);
  window()->installEventFilter(this);
#endif
}

bool xiiQtWindowTitleBar::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == window() && pEvent->type() == QEvent::WindowStateChange && m_pMaximizeButton)
  {
    const auto sTitle = window()->isMaximized() ? tr("Restore") : tr("Maximize");
    m_pMaximizeButton->setIcon(QIcon(window()->isMaximized() ? QStringLiteral(":/GuiFoundation/Icons/WindowRestore.svg") : QStringLiteral(":/GuiFoundation/Icons/WindowMaximize.svg")));
    m_pMaximizeButton->setToolTip(sTitle);
    m_pMaximizeButton->setAccessibleName(sTitle);
  }
  return QWidget::eventFilter(pObject, pEvent);
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
  {
    setMinimumHeight(qMax(32, fontMetrics().height() + 16));
    for (auto* pButton : findChildren<QToolButton*>())
      if (pButton->objectName() == QStringLiteral("EditorMinimize") || pButton->objectName() == QStringLiteral("EditorMaximize") || pButton->objectName() == QStringLiteral("EditorClose"))
        pButton->setFixedHeight(qMax(24, fontMetrics().height() + 8));
  }
  QWidget::changeEvent(pEvent);
}

void xiiQtWindowTitleBar::mousePressEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::LeftButton && IsCaptionPosition(pEvent->position().toPoint()) && !window()->isFullScreen() &&
      window()->windowHandle() && window()->windowHandle()->startSystemMove())
  {
    pEvent->accept();
    return;
  }
  QWidget::mousePressEvent(pEvent);
}

void xiiQtWindowTitleBar::mouseDoubleClickEvent(QMouseEvent* pEvent)
{
  if (pEvent->button() == Qt::LeftButton && IsCaptionPosition(pEvent->position().toPoint()) && !window()->isFullScreen())
  {
    window()->isMaximized() ? window()->showNormal() : window()->showMaximized();
    pEvent->accept();
    return;
  }
  QWidget::mouseDoubleClickEvent(pEvent);
}
