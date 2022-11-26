#pragma once

#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsView>

class QWheelEvent;
class QMouseEvent;
class QKeyEvent;
class xiiQGridBarWidget;

class XII_GUIFOUNDATION_DLL xiiQtGraphicsView : public QGraphicsView
{
  Q_OBJECT

public:
  xiiQtGraphicsView(QWidget* parent = nullptr);

  virtual void wheelEvent(QWheelEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void keyPressEvent(QKeyEvent* e) override;

  void  SetZoom(float zoom);
  float GetZoom() const { return m_fZoom; }

  void SetZoomLimits(float minZoom, float maxZoom);

Q_SIGNALS:
  void BeginDrag();
  void EndDrag();
  void DeleteCPs();

protected:
  void UpdateTransform();

  bool m_bPanning;
  bool m_bForwardMouseEvents;
  bool m_bDragging;

  float  m_fMinZoom, m_fMaxZoom;
  float  m_fZoom;
  QPoint m_LastGlobalMouseMovePos;
};
