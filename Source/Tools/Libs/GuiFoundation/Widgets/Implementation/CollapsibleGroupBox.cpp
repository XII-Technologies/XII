/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QStyleOptionFocusRect>

xiiQtCollapsibleGroupBox::xiiQtCollapsibleGroupBox(QWidget* pParent) :
  xiiQtGroupBoxBase(pParent, true)
{
  setupUi(this);

  Header->installEventFilter(this);
  Header->setMinimumHeight(fontMetrics().height() + 10);
  Header->setFocusPolicy(Qt::StrongFocus);
  Header->setToolTip(tr("Click or press Space to expand or collapse this section"));
}

void xiiQtCollapsibleGroupBox::SetTitle(xiiStringView sTitle)
{
  xiiQtGroupBoxBase::SetTitle(sTitle);
  Header->setAccessibleName(GetTitle());
  update();
}

void xiiQtCollapsibleGroupBox::SetIcon(const QIcon& icon)
{
  xiiQtGroupBoxBase::SetIcon(icon);
  update();
}

void xiiQtCollapsibleGroupBox::SetFillColor(const QColor& color)
{
  xiiQtGroupBoxBase::SetFillColor(color);
  update();
}

void xiiQtCollapsibleGroupBox::SetCollapseState(bool bCollapsed)
{
  if (bCollapsed == m_bCollapsed)
    return;

  xiiQtScopedUpdatesDisabled sud(this);

  m_bCollapsed = bCollapsed;
  Content->setVisible(!bCollapsed);

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = this;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }

  Q_EMIT CollapseStateChanged(bCollapsed);
}

bool xiiQtCollapsibleGroupBox::GetCollapseState() const
{
  return m_bCollapsed;
}

QWidget* xiiQtCollapsibleGroupBox::GetContent()
{
  return Content;
}

QWidget* xiiQtCollapsibleGroupBox::GetHeader()
{
  return Header;
}

bool xiiQtCollapsibleGroupBox::eventFilter(QObject* object, QEvent* event)
{
  switch (event->type())
  {
    case QEvent::Type::KeyPress:
    {
      const auto* pKey = static_cast<QKeyEvent*>(event);
      if (pKey->key() == Qt::Key_Space || pKey->key() == Qt::Key_Return || pKey->key() == Qt::Key_Enter)
      {
        SetCollapseState(!GetCollapseState());
        return true;
      }
      break;
    }
    case QEvent::Type::FocusIn:
    case QEvent::Type::FocusOut:
      update();
      break;
    case QEvent::Type::FontChange:
      Header->setMinimumHeight(Header->fontMetrics().height() + 10);
      break;
    case QEvent::Type::MouseButtonPress:
      HeaderMousePress(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::Type::MouseMove:
      HeaderMouseMove(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::Type::MouseButtonRelease:
      HeaderMouseRelease(static_cast<QMouseEvent*>(event));
      return true;
    default:
      break;
  }
  return false;
}

void xiiQtCollapsibleGroupBox::paintEvent(QPaintEvent* event)
{
  const QPalette& pal = palette();
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  QRect wr = contentsRect();
  QRect hr = Header->contentsRect();
  hr.moveTopLeft(Header->pos());

  if (m_FillColor.isValid())
  {
    p.fillRect(wr, pal.base());
    p.fillRect(hr, pal.alternateBase());
    // Retain component/category identity without tinting every property control.
    p.fillRect(QRect(hr.left(), hr.top(), 3, hr.height()), m_FillColor);
  }

  if (!Header->isHidden())
  {
    DrawHeader(p, hr);
    if (Header->hasFocus())
    {
      QStyleOptionFocusRect option;
      option.initFrom(Header);
      option.rect = hr.adjusted(1, 1, -1, -1);
      style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p, Header);
    }
  }
}
