#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>

xiiQtCollapsibleGroupBox::xiiQtCollapsibleGroupBox(QWidget* pParent) :
  xiiQtGroupBoxBase(pParent, true)
{
  setupUi(this);

  Header->installEventFilter(this);
}

void xiiQtCollapsibleGroupBox::SetTitle(xiiStringView sTitle)
{
  xiiQtGroupBoxBase::SetTitle(sTitle);
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

  QRect cr = wr;
  cr.setTop(hr.height());
  cr.adjust(Rounding / 2, 0, 0, -Rounding / 2);

  if (m_FillColor.isValid())
  {
    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.5, 0.5, Rounding, -0.5);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, Rounding, Rounding);
    p.fillPath(oPath, pal.alternateBase());

    QRectF crAdjusted = cr;
    crAdjusted.adjust(0.5, 0.5, Rounding, -0.5);
    QPainterPath path;
    path.addRoundedRect(crAdjusted, Rounding, Rounding);
    p.fillPath(path, m_FillColor);
  }

  if (!Header->isHidden())
  {
    DrawHeader(p, hr);
  }
}
