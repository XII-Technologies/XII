#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ImageWidget.h>
#include <QGraphicsScene>

class QGraphicsPixmapItem;

class XII_GUIFOUNDATION_DLL xiiQtImageScene : public QGraphicsScene
{
public:
  xiiQtImageScene(QObject* pParent = nullptr);

  void SetImage(QPixmap pixmap);

private:
  QPixmap              m_Pixmap;
  QGraphicsPixmapItem* m_pImageItem;
};

class XII_GUIFOUNDATION_DLL xiiQtImageWidget : public QWidget, public Ui_ImageWidget
{
  Q_OBJECT

public:
  xiiQtImageWidget(QWidget* pParent, bool bShowButtons = true);
  ~xiiQtImageWidget();

  void SetImage(QPixmap pixmap);

  void SetImageSize(float fScale = 1.0f);
  void ScaleImage(float fFactor);

private Q_SLOTS:

  void on_ButtonZoomIn_clicked();
  void on_ButtonZoomOut_clicked();
  void on_ButtonResetZoom_clicked();

private:
  void ImageApplyScale();

  xiiQtImageScene* m_pScene;
  float            m_fCurrentScale;
};
