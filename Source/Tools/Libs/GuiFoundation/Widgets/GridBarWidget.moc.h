#pragma once

#include <Foundation/Types/Delegate.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class QPaintEvent;

class XII_GUIFOUNDATION_DLL xiiQGridBarWidget : public QWidget
{
  Q_OBJECT

public:
  xiiQGridBarWidget(QWidget* parent);

  void SetConfig(const QRectF& viewportSceneRect, double fTextGridStops, double fFineGridStops, xiiDelegate<QPointF(const QPointF&)> mapFromSceneFunc);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  QRectF                               m_ViewportSceneRect;
  double                               m_fTextGridStops;
  double                               m_fFineGridStops;
  xiiDelegate<QPointF(const QPointF&)> m_MapFromSceneFunc;
};
