#pragma once

#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>

#include <QWidget>

class QMouseEvent;

class XII_GUIFOUNDATION_DLL xiiQtColorGradientWidget : public QWidget
{
  Q_OBJECT

public:
  explicit xiiQtColorGradientWidget(QWidget* pParent);
  ~xiiQtColorGradientWidget();

  void SetScrubberPosition(double fPosition);

  void setColorGradientData(const xiiColorGradient* gradient);

  void setEditMode(bool edit);
  void setShowColorCPs(bool show);
  void setShowAlphaCPs(bool show);
  void setShowIntensityCPs(bool show);
  void setShowCoords(bool top, bool bottom);

  void FrameExtents();
  void ClearSelectedCP();
  void SelectCP(xiiInt32 colorCP, xiiInt32 alphaCP, xiiInt32 intensityCP);

Q_SIGNALS:
  void GradientClicked();
  void addColorCp(double posX, const xiiColorGammaUB& color);
  void addAlphaCp(double posX, xiiUInt8 value);
  void addIntensityCp(double posX, float intensity);
  void moveColorCpToPos(xiiInt32 index, double newPosX);
  void moveAlphaCpToPos(xiiInt32 index, double newPosX);
  void moveIntensityCpToPos(xiiInt32 index, double newPosX);
  void deleteColorCp(xiiInt32 index);
  void deleteAlphaCp(xiiInt32 index);
  void deleteIntensityCp(xiiInt32 index);
  void selectionChanged(xiiInt32 colorCP, xiiInt32 alphaCP, xiiInt32 intensityCP);
  void beginOperation();
  void endOperation(bool commit);
  void triggerPickColor();

private:
  enum class Area
  {
    None         = 0,
    Gradient     = 1,
    ColorCPs     = 2,
    AlphaCPs     = 3,
    IntensityCPs = 4,
  };


  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  void UpdateMouseCursor(QMouseEvent* event);

  virtual void wheelEvent(QWheelEvent* event) override;

  void ClampDisplayExtents(double zoomCenter = 0.5);

  virtual void keyPressEvent(QKeyEvent* event) override;

  void PaintColorGradient(QPainter& p) const;
  void PaintCpBackground(QPainter& p, const QRect& area) const;
  void PaintColorCpArea(QPainter& p);
  void PaintAlphaCpArea(QPainter& p);
  void PaintIntensityCpArea(QPainter& p);
  void PaintCoordinateStrips(QPainter& p) const;
  void PaintCoordinateStrip(QPainter& p, const QRect& area) const;
  void PaintCoordinateLines(QPainter& p);

  void PaintControlPoint(
    QPainter&              p,
    const QRect&           area,
    double                 posX,
    const xiiColorGammaUB& outlineColor,
    const xiiColorGammaUB& fillColor,
    bool                   selected) const;
  void PaintColorCPs(QPainter& p) const;
  void PaintAlphaCPs(QPainter& p) const;
  void PaintIntensityCPs(QPainter& p) const;
  void PaintScrubber(QPainter& p) const;

  QRect GetColorCpArea() const;
  QRect GetAlphaCpArea() const;
  QRect GetIntensityCpArea() const;
  QRect GetGradientArea() const;
  QRect GetCoordAreaTop() const;
  QRect GetCoordAreaBottom() const;

  double   WindowToGradientCoord(xiiInt32 mouseWindowPosX) const;
  xiiInt32 GradientToWindowCoord(double gradientPosX) const;

  xiiInt32 FindClosestColorCp(xiiInt32 iWindowPosX) const;
  xiiInt32 FindClosestAlphaCp(xiiInt32 iWindowPosX) const;
  xiiInt32 FindClosestIntensityCp(xiiInt32 iWindowPosX) const;

  bool HoversControlPoint(const QPoint& windowPos) const;
  bool HoversControlPoint(const QPoint& windowPos, xiiInt32& iHoverColorCp, xiiInt32& iHoverAlphaCp, xiiInt32& iHoverIntensityCp) const;
  Area HoversInteractiveArea(const QPoint& windowPos) const;

  void EvaluateAt(xiiInt32 windowPos, xiiColorGammaUB& rgba, float& intensity) const;

  double ComputeCoordinateDisplayStep() const;

  const xiiColorGradient* m_pColorGradientData;

  bool m_bEditMode;
  bool m_bShowColorCPs;
  bool m_bShowAlphaCPs;
  bool m_bShowIntensityCPs;
  bool m_bDraggingCP;
  bool m_bTempMode;
  bool m_bShowCoordsTop;
  bool m_bShowCoordsBottom;

  double m_fDisplayExtentMinX;
  double m_fDisplayExtentMaxX;

  xiiInt32 m_iSelectedColorCP;
  xiiInt32 m_iSelectedAlphaCP;
  xiiInt32 m_iSelectedIntensityCP;

  QPoint  m_LastMousePosition;
  QPixmap m_AlphaPattern;

  bool   m_bShowScrubber     = false;
  double m_fScrubberPosition = 0;
};
