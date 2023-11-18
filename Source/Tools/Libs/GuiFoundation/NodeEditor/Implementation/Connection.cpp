#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QApplication>
#include <QPalette>

xiiQtConnection::xiiQtConnection(QGraphicsItem* pParent) :
  QGraphicsPathItem(pParent)
{
  QPen pen(xiiToQtColor(xiiColor::White), 3, Qt::SolidLine);
  setPen(pen);
  setBrush(Qt::NoBrush);

  m_InDir  = QPointF(-1.0f, 0.0f);
  m_OutDir = QPointF(1.0f, 0.0f);
  setZValue(-1);
}

xiiQtConnection::~xiiQtConnection() = default;

void xiiQtConnection::InitConnection(const xiiDocumentObject* pObject, const xiiConnection* pConnection)
{
  m_pObject     = pObject;
  m_pConnection = pConnection;
}

void xiiQtConnection::SetPosIn(const QPointF& point)
{
  m_InPoint = point;
  UpdateGeometry();
}

void xiiQtConnection::SetPosOut(const QPointF& point)
{
  m_OutPoint = point;
  UpdateGeometry();
}

void xiiQtConnection::SetDirIn(const QPointF& dir)
{
  m_InDir = dir;
  UpdateGeometry();
}

void xiiQtConnection::SetDirOut(const QPointF& dir)
{
  m_OutDir = dir;
  UpdateGeometry();
}

void xiiQtConnection::UpdateGeometry()
{
  constexpr float arrowHalfSize = 8.0f;

  prepareGeometryChange();

  QPainterPath p;
  QPointF      dir = m_InPoint - m_OutPoint;

  auto pScene = static_cast<xiiQtNodeScene*>(scene());
  if (pScene->GetConnectionStyle() == xiiQtNodeScene::ConnectionStyle::StraightLine)
  {
    QPointF startPoint = m_OutPoint;
    QPointF endPoint   = m_InPoint;

    if (pScene->GetConnectionDecorationFlags().IsSet(xiiQtNodeScene::ConnectionDecorationFlags::DirectionArrows))
    {
      const float   length    = xiiMath::Sqrt(dir.x() * dir.x() + dir.y() * dir.y());
      const float   invLength = length != 0.0f ? 1.0f / length : 1.0f;
      const QPointF dirNorm   = dir * invLength;
      const QPointF normal    = QPointF(dirNorm.y(), -dirNorm.x());

      // offset start and endpoint
      startPoint -= normal * (arrowHalfSize * 1.3f);
      endPoint -= normal * (arrowHalfSize * 1.3f);

      const QPointF midPoint  = startPoint + dir * 0.5f;
      const QPointF tipPoint  = midPoint + dirNorm * arrowHalfSize;
      const QPointF backPoint = midPoint - dirNorm * arrowHalfSize;

      QPolygonF arrow;
      arrow.append(tipPoint);
      arrow.append(backPoint + normal * arrowHalfSize);
      arrow.append(backPoint - normal * arrowHalfSize);
      arrow.append(tipPoint);

      p.addPolygon(arrow);
    }

    p.moveTo(startPoint);
    p.lineTo(endPoint);
  }
  else
  {
    p.moveTo(m_OutPoint);
    float fDotOut = xiiMath::Abs(QPointF::dotProduct(m_OutDir, dir));
    float fDotIn  = xiiMath::Abs(QPointF::dotProduct(m_InDir, -dir));

    float fMinDistance = xiiMath::Abs(QPointF::dotProduct(m_OutDir.transposed(), dir));
    fMinDistance       = xiiMath::Min(200.0f, fMinDistance);

    fDotOut = xiiMath::Max(fMinDistance, fDotOut);
    fDotIn  = xiiMath::Max(fMinDistance, fDotIn);

    QPointF ctr1 = m_OutPoint + m_OutDir * (fDotOut * 0.5f);
    QPointF ctr2 = m_InPoint + m_InDir * (fDotIn * 0.5f);

    p.cubicTo(ctr1, ctr2, m_InPoint);
  }

  setPath(p);
}

QPen xiiQtConnection::DeterminePen() const
{
  if (m_pConnection == nullptr)
  {
    return pen();
  }

  xiiColor       color;
  const xiiColor sourceColor = m_pConnection->GetSourcePin().GetColor();
  const xiiColor targetColor = m_pConnection->GetTargetPin().GetColor();

  const bool isSourceGrey = (sourceColor.r == sourceColor.g && sourceColor.r == sourceColor.b);
  const bool isTargetGrey = (targetColor.r == targetColor.g && targetColor.r == targetColor.b);

  if (!isSourceGrey)
  {
    color = xiiMath::Lerp(sourceColor, targetColor, 0.2f);
  }
  else if (!isTargetGrey)
  {
    color = xiiMath::Lerp(sourceColor, targetColor, 0.8f);
  }
  else
  {
    color = xiiMath::Lerp(sourceColor, targetColor, 0.5f);
  }

  if (m_bAdjacentNodeSelected)
  {
    color = xiiMath::Lerp(color, xiiColor::White, 0.1f);
    return QPen(QBrush(xiiToQtColor(color)), 3, Qt::DashLine);
  }
  else
  {
    return QPen(QBrush(xiiToQtColor(color)), 2, Qt::SolidLine);
  }
}

void xiiQtConnection::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto palette = QApplication::palette();

  QPen p = DeterminePen();
  if (isSelected())
  {
    p.setColor(palette.highlight().color());
  }
  painter->setPen(p);

  auto decorationFlags = static_cast<xiiQtNodeScene*>(scene())->GetConnectionDecorationFlags();
  if (decorationFlags.IsSet(xiiQtNodeScene::ConnectionDecorationFlags::DirectionArrows))
  {
    painter->setBrush(p.brush());
  }

  painter->drawPath(path());
}
