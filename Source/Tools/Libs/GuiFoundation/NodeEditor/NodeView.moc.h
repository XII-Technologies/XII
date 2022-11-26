#pragma once

#include <Foundation/Math/Vec2.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class xiiQtNodeScene;

class XII_GUIFOUNDATION_DLL xiiQtNodeView : public QGraphicsView
{
  Q_OBJECT
public:
  explicit xiiQtNodeView(QWidget* parent = nullptr);
  ~xiiQtNodeView();

  void            SetScene(xiiQtNodeScene* pScene);
  xiiQtNodeScene* GetScene();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void wheelEvent(QWheelEvent* event) override;
  virtual void contextMenuEvent(QContextMenuEvent* event) override;
  virtual void resizeEvent(QResizeEvent*) override;

private:
  void UpdateView();

private:
  xiiQtNodeScene* m_pScene;
  bool            m_bPanning;
  xiiInt32        m_iPanCounter;

  QPointF m_ViewPos;
  QPointF m_ViewScale;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  QPointF m_StartDragView;
#else
  QPoint m_vStartDragView;
#endif

  QPointF m_StartDragScene;
};
