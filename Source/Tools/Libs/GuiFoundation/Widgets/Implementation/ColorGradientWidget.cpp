#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Color8UNorm.h>
#include <GuiFoundation/Widgets/ColorGradientWidget.moc.h>
#include <QPainter>
#include <qevent.h>

static const xiiUInt32 CpAreaHeight      = 20;
static const xiiUInt32 CpRadius          = 5;
static const xiiUInt32 MaxCpPickDistance = 5;
static const xiiUInt32 CpRoundedCorner   = 3;

xiiQtColorGradientWidget::xiiQtColorGradientWidget(QWidget* pParent) :
  QWidget(pParent)
{
  m_pColorGradientData = nullptr;

  m_fDisplayExtentMinX = 0;
  m_fDisplayExtentMaxX = 1;

  m_bShowColorCPs     = false;
  m_bShowAlphaCPs     = false;
  m_bShowIntensityCPs = false;
  m_bEditMode         = false;
  m_bDraggingCP       = false;
  m_bTempMode         = false;
  m_bShowCoordsTop    = false;
  m_bShowCoordsBottom = false;

  m_iSelectedColorCP     = -1;
  m_iSelectedAlphaCP     = -1;
  m_iSelectedIntensityCP = -1;

  // needed to get keyPress events
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);

  if (!m_bEditMode)
  {
    setCursor(Qt::PointingHandCursor);
  }

  {
    // Build grid pattern.
    const int iSize = 16;
    QImage    img(iSize, iSize, QImage::Format::Format_RGBA8888);
    img.fill(Qt::white);
    QRgb halfGrayColor = qRgb(191, 191, 191);
    for (int i = 0; i < iSize / 2; ++i)
    {
      for (int j = 0; j < iSize / 2; ++j)
      {
        img.setPixel(i, j, halfGrayColor);
        img.setPixel(i + iSize / 2, j + iSize / 2, halfGrayColor);
      }
    }
    m_AlphaPattern = QPixmap::fromImage(img);
  }
}


xiiQtColorGradientWidget::~xiiQtColorGradientWidget() = default;

void xiiQtColorGradientWidget::SetScrubberPosition(double fPosition)
{
  m_bShowScrubber     = true;
  m_fScrubberPosition = fPosition;

  update();
}

void xiiQtColorGradientWidget::setColorGradientData(const xiiColorGradient* pGradient)
{
  m_pColorGradientData = pGradient;
  FrameExtents();

  update();
}


void xiiQtColorGradientWidget::setEditMode(bool bEdit)
{
  m_bEditMode = bEdit;

  setMouseTracking(m_bEditMode);
  setCursor(m_bEditMode ? Qt::ArrowCursor : Qt::PointingHandCursor);
}

void xiiQtColorGradientWidget::setShowColorCPs(bool bShow)
{
  m_bShowColorCPs = bShow;
}


void xiiQtColorGradientWidget::setShowAlphaCPs(bool bShow)
{
  m_bShowAlphaCPs = bShow;
}


void xiiQtColorGradientWidget::setShowIntensityCPs(bool bShow)
{
  m_bShowIntensityCPs = bShow;
}


void xiiQtColorGradientWidget::setShowCoords(bool bTop, bool bBottom)
{
  m_bShowCoordsTop    = bTop;
  m_bShowCoordsBottom = bBottom;
}

void xiiQtColorGradientWidget::ClearSelectedCP()
{
  SelectCP(-1, -1, -1);
}

void xiiQtColorGradientWidget::SelectCP(xiiInt32 iColorCP, xiiInt32 iAlphaCP, xiiInt32 iIntensityCP)
{
  m_bDraggingCP = false;

  if (m_bTempMode)
  {
    m_bTempMode = false;
    Q_EMIT endOperation(true);
  }

  bool changed = false;

  if (iColorCP != m_iSelectedColorCP)
  {
    m_iSelectedColorCP = iColorCP;
    changed            = true;
  }

  if (iAlphaCP != m_iSelectedAlphaCP)
  {
    m_iSelectedAlphaCP = iAlphaCP;
    changed            = true;
  }

  if (iIntensityCP != m_iSelectedIntensityCP)
  {
    m_iSelectedIntensityCP = iIntensityCP;
    changed                = true;
  }

  if (changed)
  {
    Q_EMIT selectionChanged(m_iSelectedColorCP, m_iSelectedAlphaCP, m_iSelectedIntensityCP);
  }
}

void xiiQtColorGradientWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);

  ClampDisplayExtents();

  if (m_fDisplayExtentMinX >= m_fDisplayExtentMaxX)
  {
    m_fDisplayExtentMinX = 0.0f;
    m_fDisplayExtentMaxX = 1.0f;
  }

  QPainter p(this);

  PaintColorGradient(p);
  PaintCoordinateLines(p);
  PaintCoordinateStrips(p);
  PaintColorCpArea(p);
  PaintAlphaCpArea(p);
  PaintIntensityCpArea(p);

  PaintColorCPs(p);
  PaintAlphaCPs(p);
  PaintIntensityCPs(p);

  PaintScrubber(p);
}

void xiiQtColorGradientWidget::PaintColorGradient(QPainter& p) const
{
  const QRect GradientArea = GetGradientArea();

  if (m_pColorGradientData == nullptr)
  {
    QBrush whiteBrush;
    whiteBrush.setStyle(Qt::BrushStyle::Dense4Pattern);
    whiteBrush.setColor(QColor(255, 255, 255, 255));
    p.fillRect(GradientArea, whiteBrush);
    return;
  }

  xiiColorGradient GradientFinal;
  GradientFinal = *m_pColorGradientData;
  GradientFinal.SortControlPoints();

  p.drawTiledPixmap(GradientArea, m_AlphaPattern);

  if (m_fDisplayExtentMinX <= m_fDisplayExtentMaxX)
  {
    const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

    const xiiInt32 width = GradientArea.width();

    const xiiInt32 yTop         = GradientArea.top();
    const xiiInt32 yOnlyAlpha   = yTop + GradientArea.height() / 4;
    const xiiInt32 yColorDark   = yOnlyAlpha + GradientArea.height() / 4;
    const xiiInt32 yColorTransp = yColorDark + GradientArea.height() / 4;
    const xiiInt32 yOnlyColorHeight =
      GradientArea.bottom() - yColorTransp; // GradientArea.height() / 4 has rounding errors, so last segment has to fill the rest

    QImage qiOnlyAlpha(width, 1, QImage::Format::Format_RGB32);
    QImage qiColorDark(width, 1, QImage::Format::Format_RGB32);
    QImage qiColorTransp(width, 1, QImage::Format::Format_ARGB32);
    QImage qiOnlyColor(width, 1, QImage::Format::Format_RGB32);

    for (xiiInt32 posX = 0; posX < width; ++posX)
    {
      xiiColorGammaUB rgba;
      float           intensity;

      const double lerp = (double)posX / (double)width;
      GradientFinal.Evaluate(m_fDisplayExtentMinX + lerp * range, rgba, intensity);

      const xiiColor         linearCol     = rgba;
      const xiiColor         linearColDark = linearCol * linearCol.a;
      const xiiColorGammaUB  rgbaColDar    = linearColDark;
      const xiiColorLinearUB linearAlpha(rgba.a, rgba.a, rgba.a, 255);
      const xiiColorGammaUB  srgbAlpha = xiiColor(linearAlpha);

      qiOnlyAlpha.setPixel(posX, 0, qRgb(srgbAlpha.r, srgbAlpha.g, srgbAlpha.b));
      qiColorDark.setPixel(posX, 0, qRgb(rgbaColDar.r, rgbaColDar.g, rgbaColDar.b));
      qiColorTransp.setPixel(posX, 0, qRgba(rgba.r, rgba.g, rgba.b, rgba.a));
      qiOnlyColor.setPixel(posX, 0, qRgb(rgba.r, rgba.g, rgba.b));
    }

    p.drawTiledPixmap(QRect(0, yTop, width, GradientArea.height() / 4), QPixmap::fromImage(qiOnlyAlpha));
    p.drawTiledPixmap(QRect(0, yOnlyAlpha, width, GradientArea.height() / 4), QPixmap::fromImage(qiColorDark));
    p.drawTiledPixmap(QRect(0, yColorDark, width, GradientArea.height() / 4), QPixmap::fromImage(qiColorTransp));
    p.drawTiledPixmap(QRect(0, yColorTransp, width, yOnlyColorHeight), QPixmap::fromImage(qiOnlyColor));

    // Paint Lines indicating the extremes
    {
      p.save();

      double fExtentMin, fExtentMax;
      GradientFinal.GetExtents(fExtentMin, fExtentMax);

      QPen endLines;
      endLines.setColor(Qt::white);
      endLines.setStyle(Qt::PenStyle::DashLine);

      p.setCompositionMode(QPainter::CompositionMode_Difference);

      const xiiInt32 minPos = GradientToWindowCoord(fExtentMin);
      const xiiInt32 maxPos = GradientToWindowCoord(fExtentMax);

      p.setPen(endLines);
      p.drawLine(QPoint(minPos, GradientArea.top()), QPoint(minPos, GradientArea.bottom()));
      p.drawLine(QPoint(maxPos, GradientArea.top()), QPoint(maxPos, GradientArea.bottom()));

      p.restore();
    }
  }
  else
  {
    p.setPen(QColor(255, 255, 255, 255));
    p.drawRect(GradientArea);
  }
}

void xiiQtColorGradientWidget::PaintCpBackground(QPainter& p, const QRect& area) const
{
  QBrush bg;
  bg.setStyle(Qt::BrushStyle::SolidPattern);
  bg.setColor(QColor(150, 150, 150));

  p.fillRect(area, bg);
}

void xiiQtColorGradientWidget::PaintColorCpArea(QPainter& p)
{
  if (!m_bShowColorCPs)
    return;

  PaintCpBackground(p, GetColorCpArea());
}

void xiiQtColorGradientWidget::PaintAlphaCpArea(QPainter& p)
{
  if (!m_bShowAlphaCPs)
    return;

  PaintCpBackground(p, GetAlphaCpArea());
}

void xiiQtColorGradientWidget::PaintIntensityCpArea(QPainter& p)
{
  if (!m_bShowIntensityCPs)
    return;

  PaintCpBackground(p, GetIntensityCpArea());
}


void xiiQtColorGradientWidget::PaintCoordinateStrips(QPainter& p) const
{
  if (m_bShowCoordsTop)
    PaintCoordinateStrip(p, GetCoordAreaTop());

  if (m_bShowCoordsBottom)
    PaintCoordinateStrip(p, GetCoordAreaBottom());
}


void xiiQtColorGradientWidget::PaintCoordinateStrip(QPainter& p, const QRect& area) const
{
  QBrush bg;
  bg.setStyle(Qt::BrushStyle::SolidPattern);
  bg.setColor(QColor(180, 180, 180));

  p.fillRect(area, bg);

  const double fStep = ComputeCoordinateDisplayStep();

  const double fFirstStop = xiiMath::RoundToMultiple(m_fDisplayExtentMinX, fStep);

  QString text;
  p.setPen(QColor(0, 85, 127));

  for (double fCurStop = fFirstStop; fCurStop < m_fDisplayExtentMaxX; fCurStop += fStep)
  {
    const xiiInt32 xPos = GradientToWindowCoord(fCurStop);

    text.asprintf("%.2f", fCurStop);
    QRectF r(xPos - 50, area.top(), 100, area.height());
    p.drawText(r, text, QTextOption(Qt::AlignCenter));
  }
}


void xiiQtColorGradientWidget::PaintCoordinateLines(QPainter& p)
{
  if (!m_bEditMode)
    return;

  const double fStep = ComputeCoordinateDisplayStep();

  const QRect area = GetGradientArea();

  p.save();

  QPen endLines;
  endLines.setColor(Qt::white);
  endLines.setStyle(Qt::PenStyle::SolidLine);

  p.setCompositionMode(QPainter::CompositionMode_Difference);
  p.setPen(endLines);

  const double fFirstStop = xiiMath::RoundToMultiple(m_fDisplayExtentMinX, fStep);

  const xiiInt32 iLineHeight = area.height() / 8;

  QVarLengthArray<QLine, 100> lines;
  for (double fCurStop = fFirstStop; fCurStop < m_fDisplayExtentMaxX; fCurStop += fStep)
  {
    const xiiInt32 xPos = GradientToWindowCoord(fCurStop);

    lines.push_back(QLine(QPoint(xPos, area.top()), QPoint(xPos, area.top() + iLineHeight)));
    lines.push_back(QLine(QPoint(xPos, area.bottom()), QPoint(xPos, area.bottom() - iLineHeight)));
  }
  p.drawLines(lines.data(), static_cast<xiiInt32>(lines.size()));
  p.restore();
}

void xiiQtColorGradientWidget::PaintControlPoint(QPainter& p, const QRect& area, double posX, const xiiColorGammaUB& outlineColor, const xiiColorGammaUB& fillColor, bool selected) const
{
  const xiiInt32 iPosX = GradientToWindowCoord(posX);

  if (iPosX < area.left() - (xiiInt32)CpRadius)
    return;
  if (iPosX > area.right() + (xiiInt32)CpRadius)
    return;

  QColor penColor;
  penColor.setRgb(outlineColor.r, outlineColor.g, outlineColor.b);

  QColor brushColor;
  brushColor.setRgb(fillColor.r, fillColor.g, fillColor.b);

  QBrush brush;
  brush.setStyle(Qt::BrushStyle::SolidPattern);
  brush.setColor(brushColor);

  const xiiInt32 iPosY = area.center().y();

  p.setPen(penColor);
  p.setBrush(brush);

  if (!selected)
  {
    p.drawEllipse(QPoint(iPosX, iPosY), CpRadius, CpRadius);
  }
  else
  {
    p.drawEllipse(QPoint(iPosX, iPosY), CpRadius, CpRadius);
    // p.drawRoundRect(QRect(iPosX - CpRadius, iPosY - CpRadius, 2 * CpRadius, 2 * CpRadius), CpRoundedCorner);
  }
}

void xiiQtColorGradientWidget::PaintColorCPs(QPainter& p) const
{
  if (!m_bShowColorCPs)
    return;

  const QRect area = GetColorCpArea();

  xiiUInt32 numRgb;
  xiiUInt32 numAlpha;
  xiiUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  for (xiiUInt32 i = 0; i < numRgb; ++i)
  {
    const auto& cp = m_pColorGradientData->GetColorControlPoint(i);

    const bool selected = (i == m_iSelectedColorCP);

    PaintControlPoint(p, area, cp.m_PosX, selected ? xiiColor::White : xiiColor::Black, xiiColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue), selected);
  }
}


void xiiQtColorGradientWidget::PaintAlphaCPs(QPainter& p) const
{
  if (!m_bShowAlphaCPs)
    return;

  const QRect area = GetAlphaCpArea();

  xiiUInt32 numRgb;
  xiiUInt32 numAlpha;
  xiiUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  for (xiiUInt32 i = 0; i < numAlpha; ++i)
  {
    const auto& cp = m_pColorGradientData->GetAlphaControlPoint(i);

    const bool selected = i == m_iSelectedAlphaCP;

    PaintControlPoint(p, area, cp.m_PosX, selected ? xiiColor::White : xiiColor::Black, xiiColorGammaUB(cp.m_Alpha, cp.m_Alpha, cp.m_Alpha), selected);
  }
}

void xiiQtColorGradientWidget::PaintIntensityCPs(QPainter& p) const
{
  if (!m_bShowIntensityCPs)
    return;

  const QRect area = GetIntensityCpArea();

  xiiUInt32 numRgb;
  xiiUInt32 numAlpha;
  xiiUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  float fMaxIntensity = 0.0f;
  for (xiiUInt32 i = 0; i < numIntensity; ++i)
  {
    const auto& cp = m_pColorGradientData->GetIntensityControlPoint(i);
    fMaxIntensity  = xiiMath::Max(cp.m_Intensity, fMaxIntensity);
  }

  const float fInvMaxIntensity = 1.0f / fMaxIntensity;

  for (xiiUInt32 i = 0; i < numIntensity; ++i)
  {
    const auto& cp = m_pColorGradientData->GetIntensityControlPoint(i);

    const bool selected = i == m_iSelectedIntensityCP;

    float fIntensity = cp.m_Intensity * fInvMaxIntensity;
    PaintControlPoint(p, area, cp.m_PosX, selected ? xiiColor::White : xiiColor::Black, xiiColor(fIntensity, fIntensity, fIntensity), selected);
  }
}

void xiiQtColorGradientWidget::PaintScrubber(QPainter& p) const
{
  if (!m_bShowScrubber)
    return;

  const QRect area = rect();

  const xiiInt32 xPos = GradientToWindowCoord(m_fScrubberPosition);
  if (xPos < 0 || xPos > area.width())
    return;

  p.save();

  QPen pen;
  pen.setCosmetic(true);
  pen.setColor(palette().highlight().color());
  pen.setWidth(1);

  p.setPen(pen);
  p.drawLine(QLine(xPos, area.top(), xPos, area.bottom()));

  p.restore();
}

void xiiQtColorGradientWidget::mousePressEvent(QMouseEvent* event)
{
  if (!m_bEditMode)
  {
    // in non-edit mode, allow to react to this click (only)

    if (event->button() == Qt::MouseButton::LeftButton)
    {
      Q_EMIT GradientClicked();
    }
  }
  else
  {
    if (event->button() == Qt::MouseButton::RightButton)
    {
      m_LastMousePosition = event->globalPosition();
    }

    if (event->buttons() == Qt::MouseButton::LeftButton)
    {
      // left click and nothing else

      xiiInt32 iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp;
      if (HoversControlPoint(event->pos(), iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp))
      {
        SelectCP(iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp);

        m_bDraggingCP = true;

        update();
      }
    }
  }

  QWidget::mousePressEvent(event);
}


void xiiQtColorGradientWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::MouseButton::LeftButton)
  {
    m_bDraggingCP = false;

    if (m_bTempMode)
    {
      m_bTempMode = false;
      Q_EMIT endOperation(true);
    }
  }

  QWidget::mouseReleaseEvent(event);
}

void xiiQtColorGradientWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (m_bEditMode)
  {
    const Area area = HoversInteractiveArea(event->pos());

    // left click and nothing else
    if (event->buttons() == Qt::MouseButton::LeftButton)
    {
      const double posX = WindowToGradientCoord(event->pos().x());

      const bool hovers = HoversControlPoint(event->pos());

      // in an interactive area, not over any control point
      if (area > Area::Gradient)
      {
        if (!hovers)
        {
          xiiColorGammaUB rgba;
          float           intensity;
          EvaluateAt(event->pos().x(), rgba, intensity);

          if (area == Area::ColorCPs)
          {
            Q_EMIT addColorCp(posX, rgba);
          }
          else if (area == Area::AlphaCPs)
          {
            Q_EMIT addAlphaCp(posX, rgba.a);
          }
          else if (area == Area::IntensityCPs)
          {
            Q_EMIT addIntensityCp(posX, intensity);
          }

          setCursor(Qt::SizeHorCursor);
        }
        else if (m_iSelectedColorCP != -1)
        {
          if (m_bTempMode)
          {
            m_bDraggingCP = false;
            m_bTempMode   = false;
            Q_EMIT endOperation(true);
          }

          Q_EMIT triggerPickColor();
        }
      }
    }
  }

  QWidget::mouseDoubleClickEvent(event);
}

void xiiQtColorGradientWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (m_bEditMode)
  {
    // reset mouse dragging flag if necessary
    if (!event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
      m_bDraggingCP = false;

      if (m_bTempMode)
      {
        m_bTempMode = false;
        Q_EMIT endOperation(true);
      }
    }

    if (m_bDraggingCP)
    {
      if (!m_bTempMode)
      {
        m_bTempMode = true;
        Q_EMIT beginOperation();
      }

      const double newPosX = WindowToGradientCoord(event->pos().x());

      if (m_iSelectedColorCP != -1)
      {
        Q_EMIT moveColorCpToPos(m_iSelectedColorCP, newPosX);
      }
      else if (m_iSelectedAlphaCP != -1)
      {
        Q_EMIT moveAlphaCpToPos(m_iSelectedAlphaCP, newPosX);
      }
      else if (m_iSelectedIntensityCP != -1)
      {
        Q_EMIT moveIntensityCpToPos(m_iSelectedIntensityCP, newPosX);
      }
    }
    else
    {
      if (event->buttons() == Qt::MouseButton::RightButton)
      {
        // scroll displayed area
        if (m_fDisplayExtentMinX < m_fDisplayExtentMaxX)
        {
          const QPointF mouseMove = event->globalPosition() - m_LastMousePosition;
          m_LastMousePosition     = event->globalPosition();

          const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

          const double scrolled = (double)mouseMove.x() / (double)GetGradientArea().width();

          /// \todo Why not += ?
          m_fDisplayExtentMinX -= scrolled * range;
          m_fDisplayExtentMaxX -= scrolled * range;

          update();
        }
      }
    }
  }

  UpdateMouseCursor(event);
  QWidget::mouseMoveEvent(event);
}


void xiiQtColorGradientWidget::UpdateMouseCursor(QMouseEvent* event)
{
  setCursor(Qt::ArrowCursor);

  if (m_bDraggingCP)
  {
    setCursor(Qt::SizeHorCursor);
    return;
  }

  if (event->buttons() == Qt::MouseButton::RightButton)
  {
    setCursor(Qt::ClosedHandCursor);
    return;
  }

  xiiInt32 iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp;

  if (HoversInteractiveArea(event->pos()) > Area::Gradient)
  {
    if (HoversControlPoint(event->pos(), iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp))
      setCursor(Qt::SizeHorCursor);
    else
      setCursor(Qt::PointingHandCursor);
  }
}

void xiiQtColorGradientWidget::wheelEvent(QWheelEvent* event)
{
  if (m_bEditMode)
  {
    // zoom displayed area
    if (m_fDisplayExtentMinX < m_fDisplayExtentMaxX)
    {
      const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      const double zoomCenter = WindowToGradientCoord(event->position().x());
#else
      const double zoomCenter = WindowToGradientCoord(event->pos().x());
#endif
      const double zoomNorm = (zoomCenter - m_fDisplayExtentMinX) / range;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      const double changePerc = (event->angleDelta().y() > 0) ? 0.1 : -0.1;
#else
      const double changePerc = (event->delta() > 0) ? 0.1 : -0.1;
#endif
      const double change = changePerc * range;

      m_fDisplayExtentMinX += change * zoomNorm;
      m_fDisplayExtentMaxX -= change * (1.0 - zoomNorm);

      ClampDisplayExtents(zoomNorm);

      update();
    }
  }

  QWidget::wheelEvent(event);
}

void xiiQtColorGradientWidget::ClampDisplayExtents(double zoomCenter)
{
  const double newRange     = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;
  const double clampedRange = xiiMath::Clamp(newRange, 0.05, 100.0);
  const double center       = xiiMath::Lerp(m_fDisplayExtentMinX, m_fDisplayExtentMaxX, zoomCenter);

  m_fDisplayExtentMinX = center - clampedRange * zoomCenter;
  m_fDisplayExtentMaxX = center + clampedRange * (1.0 - zoomCenter);
}

void xiiQtColorGradientWidget::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Delete)
  {
    if (m_iSelectedColorCP != -1)
    {
      Q_EMIT deleteColorCp(m_iSelectedColorCP);
      ClearSelectedCP();
    }
    else if (m_iSelectedAlphaCP != -1)
    {
      Q_EMIT deleteAlphaCp(m_iSelectedAlphaCP);
      ClearSelectedCP();
    }
    else if (m_iSelectedIntensityCP != -1)
    {
      Q_EMIT deleteIntensityCp(m_iSelectedIntensityCP);
      ClearSelectedCP();
    }
  }

  QWidget::keyPressEvent(event);
}

QRect xiiQtColorGradientWidget::GetColorCpArea() const
{
  QRect r = rect();
  r.setTop(r.bottom() - CpAreaHeight);

  if (m_bShowCoordsBottom)
    r.setTop(r.top() - CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

QRect xiiQtColorGradientWidget::GetAlphaCpArea() const
{
  QRect r = rect();

  if (m_bShowIntensityCPs)
  {
    // below intensity curve
    r.setTop(r.top() + CpAreaHeight);
  }

  if (m_bShowCoordsTop)
    r.setTop(r.top() + CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

QRect xiiQtColorGradientWidget::GetIntensityCpArea() const
{
  QRect r = rect();

  if (m_bShowCoordsTop)
    r.setTop(r.top() + CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

QRect xiiQtColorGradientWidget::GetGradientArea() const
{
  QRect r = rect();

  if (m_bShowIntensityCPs)
    r.setTop(r.top() + CpAreaHeight);

  if (m_bShowAlphaCPs)
    r.setTop(r.top() + CpAreaHeight);

  if (m_bShowCoordsTop)
    r.setTop(r.top() + CpAreaHeight);


  if (m_bShowColorCPs)
    r.setBottom(r.bottom() - CpAreaHeight);

  if (m_bShowCoordsBottom)
    r.setBottom(r.bottom() - CpAreaHeight);

  return r;
}


QRect xiiQtColorGradientWidget::GetCoordAreaTop() const
{
  QRect r = rect();

  r.setHeight(CpAreaHeight);
  return r;
}


QRect xiiQtColorGradientWidget::GetCoordAreaBottom() const
{
  QRect r = rect();
  r.setTop(r.bottom() - CpAreaHeight);

  r.setHeight(CpAreaHeight);
  return r;
}

double xiiQtColorGradientWidget::WindowToGradientCoord(xiiInt32 mouseWindowPosX) const
{
  QRect        area = GetGradientArea();
  const double norm = (double)(mouseWindowPosX - area.left()) / (double)area.width();
  return m_fDisplayExtentMinX + norm * (m_fDisplayExtentMaxX - m_fDisplayExtentMinX);
}

xiiInt32 xiiQtColorGradientWidget::GradientToWindowCoord(double gradientPosX) const
{
  QRect        area = GetGradientArea();
  const double norm = (gradientPosX - m_fDisplayExtentMinX) / (m_fDisplayExtentMaxX - m_fDisplayExtentMinX);
  return area.left() + norm * (area.right() - area.left());
}

xiiInt32 xiiQtColorGradientWidget::FindClosestColorCp(xiiInt32 iWindowPosX) const
{
  xiiUInt32 numRgb;
  xiiUInt32 numAlpha;
  xiiUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  xiiInt32 iClosest      = -1;
  xiiInt32 iBestDistance = MaxCpPickDistance + 1;

  for (xiiUInt32 i = 0; i < numRgb; ++i)
  {
    const auto& cp = m_pColorGradientData->GetColorControlPoint(i);

    const xiiInt32 iCpPos = GradientToWindowCoord(cp.m_PosX);
    const xiiInt32 iDist  = xiiMath::Abs(iCpPos - iWindowPosX);

    if (iDist < iBestDistance)
    {
      iClosest      = i;
      iBestDistance = iDist;
    }
  }

  return iClosest;
}

xiiInt32 xiiQtColorGradientWidget::FindClosestAlphaCp(xiiInt32 iWindowPosX) const
{
  xiiUInt32 numRgb;
  xiiUInt32 numAlpha;
  xiiUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  xiiInt32 iClosest      = -1;
  xiiInt32 iBestDistance = MaxCpPickDistance + 1;

  for (xiiUInt32 i = 0; i < numAlpha; ++i)
  {
    const auto& cp = m_pColorGradientData->GetAlphaControlPoint(i);

    const xiiInt32 iCpPos = GradientToWindowCoord(cp.m_PosX);
    const xiiInt32 iDist  = xiiMath::Abs(iCpPos - iWindowPosX);

    if (iDist < iBestDistance)
    {
      iClosest      = i;
      iBestDistance = iDist;
    }
  }

  return iClosest;
}

xiiInt32 xiiQtColorGradientWidget::FindClosestIntensityCp(xiiInt32 iWindowPosX) const
{
  xiiUInt32 numRgb;
  xiiUInt32 numAlpha;
  xiiUInt32 numIntensity;
  m_pColorGradientData->GetNumControlPoints(numRgb, numAlpha, numIntensity);

  xiiInt32 iClosest      = -1;
  xiiInt32 iBestDistance = MaxCpPickDistance + 1;

  for (xiiUInt32 i = 0; i < numIntensity; ++i)
  {
    const auto& cp = m_pColorGradientData->GetIntensityControlPoint(i);

    const xiiInt32 iCpPos = GradientToWindowCoord(cp.m_PosX);
    const xiiInt32 iDist  = xiiMath::Abs(iCpPos - iWindowPosX);

    if (iDist < iBestDistance)
    {
      iClosest      = i;
      iBestDistance = iDist;
    }
  }

  return iClosest;
}

bool xiiQtColorGradientWidget::HoversControlPoint(const QPoint& windowPos) const
{
  xiiInt32 iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp;
  return HoversControlPoint(windowPos, iHoverColorCp, iHoverAlphaCp, iHoverIntensityCp);
}

bool xiiQtColorGradientWidget::HoversControlPoint(const QPoint& windowPos, xiiInt32& iHoverColorCp, xiiInt32& iHoverAlphaCp, xiiInt32& iHoverIntensityCp) const
{
  iHoverColorCp     = -1;
  iHoverAlphaCp     = -1;
  iHoverIntensityCp = -1;

  if (m_bShowColorCPs)
  {
    if (GetColorCpArea().contains(windowPos))
    {
      iHoverColorCp = FindClosestColorCp(windowPos.x());
    }
  }

  if (m_bShowAlphaCPs)
  {
    if (GetAlphaCpArea().contains(windowPos))
    {
      iHoverAlphaCp = FindClosestAlphaCp(windowPos.x());
    }
  }

  if (m_bShowIntensityCPs)
  {
    if (GetIntensityCpArea().contains(windowPos))
    {
      iHoverIntensityCp = FindClosestIntensityCp(windowPos.x());
    }
  }

  return (iHoverColorCp != -1) || (iHoverAlphaCp != -1) || (iHoverIntensityCp != -1);
}


xiiQtColorGradientWidget::Area xiiQtColorGradientWidget::HoversInteractiveArea(const QPoint& windowPos) const
{
  if (m_bShowColorCPs)
  {
    if (GetColorCpArea().contains(windowPos))
      return Area::ColorCPs;
  }

  if (m_bShowAlphaCPs)
  {
    if (GetAlphaCpArea().contains(windowPos))
      return Area::AlphaCPs;
  }

  if (m_bShowIntensityCPs)
  {
    if (GetIntensityCpArea().contains(windowPos))
      return Area::IntensityCPs;
  }

  if (GetGradientArea().contains(windowPos))
    return Area::Gradient;

  return Area::None;
}


void xiiQtColorGradientWidget::EvaluateAt(xiiInt32 windowPos, xiiColorGammaUB& rgba, float& intensity) const
{
  xiiColorGradient GradientFinal;
  GradientFinal = *m_pColorGradientData;
  GradientFinal.SortControlPoints();

  const double range = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;

  const double lerp = (double)windowPos / (double)rect().width();
  GradientFinal.Evaluate(m_fDisplayExtentMinX + lerp * range, rgba, intensity);
}


double xiiQtColorGradientWidget::ComputeCoordinateDisplayStep() const
{
  const xiiInt32 iPixelsNeeded = 50;
  const double   fFitInWindow  = xiiMath::Max<double>(2, rect().width() / (double)iPixelsNeeded);

  const double fGradientRange = (m_fDisplayExtentMaxX - m_fDisplayExtentMinX);
  const double fSubRange      = fGradientRange / fFitInWindow;

  const double   fExp = xiiMath::Log10(fSubRange);
  const xiiInt32 iExp = xiiMath::Ceil(fExp);

  const double step = xiiMath::Pow(10.0, (double)iExp);

  return step;
}

void xiiQtColorGradientWidget::FrameExtents()
{
  if (m_pColorGradientData)
  {
    m_pColorGradientData->GetExtents(m_fDisplayExtentMinX, m_fDisplayExtentMaxX);
  }

  if (!m_pColorGradientData || m_fDisplayExtentMinX > m_fDisplayExtentMaxX)
  {
    m_fDisplayExtentMinX = 0;
    m_fDisplayExtentMaxX = 1;
  }

  if (m_fDisplayExtentMinX == m_fDisplayExtentMaxX)
  {
    m_fDisplayExtentMinX = xiiMath::Floor(m_fDisplayExtentMinX - 0.1);
    m_fDisplayExtentMaxX = xiiMath::Ceil(m_fDisplayExtentMaxX + 0.1);
  }

  if (m_bEditMode)
  {
    // round up/down to next multiple of 1
    m_fDisplayExtentMinX = xiiMath::Floor(m_fDisplayExtentMinX);
    m_fDisplayExtentMaxX = xiiMath::Ceil(m_fDisplayExtentMaxX);

    const double range  = m_fDisplayExtentMaxX - m_fDisplayExtentMinX;
    const double border = range * 0.05;

    m_fDisplayExtentMinX -= border;
    m_fDisplayExtentMaxX += border;
  }
}
