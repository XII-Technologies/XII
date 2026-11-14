/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Vec2.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class xiiQtNodeScene;

class XII_GUIFOUNDATION_DLL xiiQtNodeView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit xiiQtNodeView(QWidget* pParent = nullptr);
  ~xiiQtNodeView() override;

  void            SetScene(xiiQtNodeScene* pScene);
  xiiQtNodeScene* GetScene();

protected:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void resizeEvent(QResizeEvent*) override;
  void drawBackground(QPainter* painter, const QRectF& r) override;

private:
  void UpdateView();

  void DrawGrid(QPainter* painter, double gridStep);

private:
  xiiQtNodeScene* m_pScene      = nullptr;
  bool            m_bPanning    = false;
  xiiInt32        m_iPanCounter = 0;

  QPointF m_ViewPos;
  QPointF m_ViewScale;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF m_StartDragView;
#else
  QPoint m_vStartDragView;
#endif

  QPointF m_StartDragScene;
};
