#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Math.h>
#include <GuiFoundation/Widgets/ImageWidget.moc.h>
#include <QGraphicsPixmapItem>
#include <QScrollArea>
#include <QScrollBar>

xiiQtImageScene::xiiQtImageScene(QObject* pParent) :
  QGraphicsScene(pParent)
{
  m_pImageItem = nullptr;
  setItemIndexMethod(QGraphicsScene::NoIndex);
}

void xiiQtImageScene::SetImage(QPixmap pixmap)
{
  if (m_pImageItem)
    delete m_pImageItem;

  m_Pixmap     = pixmap;
  m_pImageItem = addPixmap(m_Pixmap);
  setSceneRect(0, 0, m_Pixmap.width(), m_Pixmap.height());
}



xiiQtImageWidget::xiiQtImageWidget(QWidget* pParent, bool bShowButtons) :
  QWidget(pParent)
{
  setupUi(this);
  m_pScene = new xiiQtImageScene(GraphicsView);
  GraphicsView->setScene(m_pScene);

  m_fCurrentScale = 1.0f;

  if (!bShowButtons)
    ButtonBar->setVisible(false);
}

xiiQtImageWidget::~xiiQtImageWidget() = default;

void xiiQtImageWidget::SetImageSize(float fScale)
{
  if (m_fCurrentScale == fScale)
    return;

  m_fCurrentScale = fScale;
  ImageApplyScale();
}

void xiiQtImageWidget::ScaleImage(float fFactor)
{
  float fPrevScale = m_fCurrentScale;
  m_fCurrentScale  = xiiMath::Clamp(m_fCurrentScale * fFactor, 0.2f, 5.0f);

  fFactor = m_fCurrentScale / fPrevScale;
  ImageApplyScale();
}

void xiiQtImageWidget::ImageApplyScale()
{
  QTransform scale = QTransform::fromScale(m_fCurrentScale, m_fCurrentScale);
  GraphicsView->setTransform(scale);
}

void xiiQtImageWidget::SetImage(QPixmap pixmap)
{
  m_pScene->SetImage(pixmap);
  ImageApplyScale();
}

void xiiQtImageWidget::on_ButtonZoomIn_clicked()
{
  ScaleImage(1.25f);
}

void xiiQtImageWidget::on_ButtonZoomOut_clicked()
{
  ScaleImage(0.75f);
}

void xiiQtImageWidget::on_ButtonResetZoom_clicked()
{
  SetImageSize(1.0f);
}
