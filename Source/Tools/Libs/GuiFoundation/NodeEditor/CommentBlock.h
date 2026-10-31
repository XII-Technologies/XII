/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <QGraphicsPathItem>
#include <ToolsFoundation/NodeObject/GraphCommentBlock.h>

class QGraphicsTextItem;
class xiiQtNodeScene;

/// A resizable Editor annotation, drawn behind domain nodes.
class XII_GUIFOUNDATION_DLL xiiQtGraphCommentBlock : public QGraphicsPathItem
{
public:
  explicit xiiQtGraphCommentBlock(xiiQtNodeScene& scene);
  int                         type() const override;
  void                        SetBlock(const xiiGraphCommentBlock& block);
  const xiiGraphCommentBlock& GetBlock() const { return m_Block; }

protected:
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

private:
  xiiQtNodeScene&      m_Scene;
  xiiGraphCommentBlock m_Block;
  xiiGraphCommentBlock m_Start;
  QPointF              m_StartMouse;
  QGraphicsTextItem*   m_pText     = nullptr;
  bool                 m_bResizing = false;
};
