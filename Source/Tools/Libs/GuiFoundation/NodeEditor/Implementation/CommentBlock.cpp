/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/NodeEditor/CommentBlock.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QPainter>

xiiQtGraphCommentBlock::xiiQtGraphCommentBlock(xiiQtNodeScene& scene) : m_Scene(scene)
{
  setZValue(-10);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemClipsChildrenToShape);
  setAcceptHoverEvents(true);
  m_pText = new QGraphicsTextItem(this);
  m_pText->setAcceptedMouseButtons(Qt::NoButton);
  m_pText->setDefaultTextColor(Qt::white);
  m_pText->setPos(8, 4);
}

int xiiQtGraphCommentBlock::type() const { return xiiQtNodeScene::CommentBlock; }

void xiiQtGraphCommentBlock::SetBlock(const xiiGraphCommentBlock& block)
{
  m_Block = block;
  setPos(block.m_vPosition.x, block.m_vPosition.y);
  QPainterPath shape;
  shape.addRoundedRect(0, 0, block.m_vSize.x, block.m_vSize.y, 6, 6);
  setPath(shape);
  m_pText->setTextWidth(block.m_vSize.x - 16);
  m_pText->setPlainText(xiiMakeQString(block.m_sText));
  update();
}

void xiiQtGraphCommentBlock::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  const xiiColorGammaUB color(m_Block.m_Color);
  const QColor          opaque(color.r, color.g, color.b);
  painter->setPen(QPen(isSelected() ? Qt::white : opaque, isSelected() ? 2 : 1));
  auto body = opaque;
  body.setAlpha(40);
  painter->setBrush(body);
  painter->drawPath(path());
  painter->save();
  painter->setClipPath(path());
  painter->fillRect(QRectF(0, 0, m_Block.m_vSize.x, m_pText->boundingRect().height() + 8), opaque);
  painter->restore();
  painter->drawLine(QPointF(m_Block.m_vSize.x - 14, m_Block.m_vSize.y - 3), QPointF(m_Block.m_vSize.x - 3, m_Block.m_vSize.y - 14));
}

void xiiQtGraphCommentBlock::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  if (event->button() != Qt::LeftButton)
  {
    event->ignore();
    return;
  }
  m_Start      = m_Block;
  m_StartMouse = event->scenePos();
  m_bResizing  = event->pos().x() >= m_Block.m_vSize.x - 18 && event->pos().y() >= m_Block.m_vSize.y - 18;
  if (!event->modifiers().testFlag(Qt::ControlModifier))
    scene()->clearSelection();
  setSelected(true);
  event->accept();
}

void xiiQtGraphCommentBlock::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  const auto delta = event->scenePos() - m_StartMouse;
  auto       block = m_Start;
  if (m_bResizing)
    block.m_vSize = xiiVec2(xiiMath::Max(160.0f, m_Start.m_vSize.x + static_cast<float>(delta.x())), xiiMath::Max(80.0f, m_Start.m_vSize.y + static_cast<float>(delta.y())));
  else
    block.m_vPosition += xiiVec2(delta.x(), delta.y());
  SetBlock(block);
  event->accept();
}

void xiiQtGraphCommentBlock::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  const auto edited = m_Block;
  SetBlock(m_Start);
  const auto status = m_Scene.SetCommentBlock(edited, !m_bResizing);
  xiiQtUiServices::GetSingleton()->MessageBoxStatus(status, "Could not edit graph comment block.");
  event->accept();
}

void xiiQtGraphCommentBlock::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
  const bool resize = event->pos().x() >= m_Block.m_vSize.x - 18 && event->pos().y() >= m_Block.m_vSize.y - 18;
  setCursor(resize ? Qt::SizeFDiagCursor : Qt::SizeAllCursor);
  event->accept();
}
