#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <QApplication>
#include <QPalette>

xiiQtPin::xiiQtPin()
{
  auto palette = QApplication::palette();

  QPen pen(palette.light().color(), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(palette.base());

  setFlag(QGraphicsItem::ItemSendsGeometryChanges);
  setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

  m_pLabel = new QGraphicsTextItem(this);
}

xiiQtPin::~xiiQtPin() = default;

void xiiQtPin::AddConnection(xiiQtConnection* pConnection)
{
  XII_ASSERT_DEBUG(!m_Connections.Contains(pConnection), "Connection already present!");
  m_Connections.PushBack(pConnection);

  ConnectedStateChanged(true);

  UpdateConnections();
}

void xiiQtPin::RemoveConnection(xiiQtConnection* pConnection)
{
  XII_ASSERT_DEBUG(m_Connections.Contains(pConnection), "Connection not present!");
  m_Connections.RemoveAndSwap(pConnection);

  if (m_Connections.IsEmpty())
    ConnectedStateChanged(false);
}

void xiiQtPin::ConnectedStateChanged(bool bConnected)
{
  if (bConnected)
  {
    setBrush(pen().color().darker(125));
  }
  else
  {
    setBrush(QApplication::palette().base());
  }
}

void xiiQtPin::SetPin(const xiiPin& pin)
{
  m_pPin = &pin;

  m_pLabel->setPlainText(pin.GetName());
  auto rectLabel = m_pLabel->boundingRect();

  const int iRadus = rectLabel.height();
  QRectF    bounds;

  if (pin.GetType() == xiiPin::Type::Input)
  {
    m_pLabel->setPos(iRadus, 0);
    bounds = QRectF(0, 0, iRadus, iRadus);
  }
  else
  {
    m_pLabel->setPos(0, 0);
    bounds = QRectF(rectLabel.width(), 0, iRadus, iRadus);
  }

  const int shrink = 3;
  bounds.adjust(shrink, shrink, -shrink, -shrink);
  m_PinCenter = bounds.center();

  {
    QPainterPath p;
    switch (m_pPin->m_Shape)
    {
      case xiiPin::Shape::Circle:
        p.addEllipse(bounds);
        break;
      case xiiPin::Shape::Rect:
        p.addRect(bounds);
        break;
      case xiiPin::Shape::RoundRect:
        p.addRoundedRect(bounds, 2, 2);
        break;
        XII_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    setPath(p);
  }

  QPen p = pen();
  p.setColor(xiiToQtColor(pin.GetColor()));
  setPen(p);
}

QPointF xiiQtPin::GetPinPos() const
{
  return mapToScene(m_PinCenter);
}

QPointF xiiQtPin::GetPinDir() const
{
  if (m_pPin->GetType() == xiiPin::Type::Input)
  {
    return QPointF(-1.0f, 0.0f);
  }
  else
  {
    return QPointF(1.0f, 0.0f);
    ;
  }
}

QRectF xiiQtPin::GetPinRect() const
{
  auto rectLabel = m_pLabel->boundingRect();
  rectLabel.translate(m_pLabel->pos());

  if (m_pPin->GetType() == xiiPin::Type::Input)
  {
    rectLabel.adjust(-5, 0, 0, 0);
  }
  else
  {
    rectLabel.adjust(0, 0, 5, 0);
  }
  return rectLabel;
}

void xiiQtPin::UpdateConnections()
{
  for (xiiQtConnection* pConnection : m_Connections)
  {
    if (m_pPin->GetType() == xiiPin::Type::Input)
    {
      pConnection->SetDirIn(GetPinDir());
      pConnection->SetPosIn(GetPinPos());
    }
    else
    {
      pConnection->SetDirOut(GetPinDir());
      pConnection->SetPosOut(GetPinPos());
    }
  }
}

void xiiQtPin::SetHighlightState(xiiQtPinHighlightState state)
{
  if (m_HighlightState != state)
  {
    m_HighlightState = state;

    if (AdjustRenderingForHighlight(state))
    {
      update();
    }
  }
}

void xiiQtPin::SetActive(bool bActive)
{
  m_bIsActive = bActive;

  if (AdjustRenderingForHighlight(m_HighlightState))
  {
    update();
  }
}

bool xiiQtPin::AdjustRenderingForHighlight(xiiQtPinHighlightState state)
{
  xiiColorGammaUB pinColor = GetPin()->GetColor();
  QColor          base     = QApplication::palette().base().color();

  if (!m_bIsActive)
    pinColor = xiiMath::Lerp<xiiColor>(xiiColorGammaUB(base.red(), base.green(), base.blue()), pinColor, 0.2f);

  switch (state)
  {
    case xiiQtPinHighlightState::None:
    {
      QPen p = pen();
      p.setColor(xiiToQtColor(pinColor));
      setPen(p);

      setBrush(GetConnections().IsEmpty() ? base : pen().color().darker(125));
    }
    break;

    case xiiQtPinHighlightState::CannotConnect:
    case xiiQtPinHighlightState::CannotConnectSameDirection:
    {
      QPen p = pen();
      p.setColor(base.lighter());
      setPen(p);

      setBrush(base);
    }
    break;

    case xiiQtPinHighlightState::CanReplaceConnection:
    case xiiQtPinHighlightState::CanAddConnection:
    {
      QPen p = pen();
      p.setColor(xiiToQtColor(pinColor));
      setPen(p);

      setBrush(base);
    }
    break;
  }

  QColor labelColor = QApplication::palette().buttonText().color();
  if (!m_bIsActive)
    labelColor = labelColor.darker(150);

  m_pLabel->setDefaultTextColor(labelColor);

  return true;
}

QVariant xiiQtPin::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (change == QGraphicsItem::ItemScenePositionHasChanged)
  {
    UpdateConnections();
  }

  return QGraphicsPathItem::itemChange(change, value);
}
