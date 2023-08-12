#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>

xiiQtColorGradientEditorWidget::xiiQtColorGradientEditorWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  GradientWidget->setColorGradientData(&m_Gradient);
  GradientWidget->setEditMode(true);
  GradientWidget->FrameExtents();

  GradientWidget->setShowColorCPs(true);
  GradientWidget->setShowAlphaCPs(true);
  GradientWidget->setShowIntensityCPs(true);
  GradientWidget->setShowCoords(true, true);

  on_GradientWidget_selectionChanged(-1, -1, -1);

  connect(GradientWidget, &xiiQtColorGradientWidget::addColorCp, this, [this](double x, const xiiColorGammaUB& color) { Q_EMIT ColorCpAdded(x, color); });
  connect(GradientWidget, &xiiQtColorGradientWidget::moveColorCpToPos, this, [this](xiiInt32 iIdx, double x) { Q_EMIT ColorCpMoved(iIdx, x); });
  connect(GradientWidget, &xiiQtColorGradientWidget::deleteColorCp, this, [this](xiiInt32 iIdx) { Q_EMIT ColorCpDeleted(iIdx); });

  connect(GradientWidget, &xiiQtColorGradientWidget::addAlphaCp, this, [this](double x, xiiUInt8 uiAlpha) { Q_EMIT AlphaCpAdded(x, uiAlpha); });
  connect(GradientWidget, &xiiQtColorGradientWidget::moveAlphaCpToPos, this, [this](xiiInt32 iIdx, double x) { Q_EMIT AlphaCpMoved(iIdx, x); });
  connect(GradientWidget, &xiiQtColorGradientWidget::deleteAlphaCp, this, [this](xiiInt32 iIdx) { Q_EMIT AlphaCpDeleted(iIdx); });

  connect(GradientWidget, &xiiQtColorGradientWidget::addIntensityCp, this, [this](double x, float fIntensity) { Q_EMIT IntensityCpAdded(x, fIntensity); });
  connect(GradientWidget, &xiiQtColorGradientWidget::moveIntensityCpToPos, this, [this](xiiInt32 iIdx, double x) { Q_EMIT IntensityCpMoved(iIdx, x); });
  connect(GradientWidget, &xiiQtColorGradientWidget::deleteIntensityCp, this, [this](xiiInt32 iIdx) { Q_EMIT IntensityCpDeleted(iIdx); });

  connect(GradientWidget, &xiiQtColorGradientWidget::beginOperation, this, [this]() { Q_EMIT BeginOperation(); });
  connect(GradientWidget, &xiiQtColorGradientWidget::endOperation, this, [this](bool bCommit) { Q_EMIT EndOperation(bCommit); });

  connect(GradientWidget, &xiiQtColorGradientWidget::triggerPickColor, this, [this]() { on_ButtonColor_clicked(); });
}


xiiQtColorGradientEditorWidget::~xiiQtColorGradientEditorWidget() = default;


void xiiQtColorGradientEditorWidget::SetColorGradient(const xiiColorGradient& gradient)
{
  bool clearSelection = false;

  // clear selection if the number of control points has changed
  {
    xiiUInt32 numRgb = 0xFFFFFFFF, numRgb2 = 0xFFFFFFFF;
    xiiUInt32 numAlpha = 0xFFFFFFFF, numAlpha2 = 0xFFFFFFFF;
    xiiUInt32 numIntensity = 0xFFFFFFFF, numIntensity2 = 0xFFFFFFFF;

    gradient.GetNumControlPoints(numRgb, numAlpha, numIntensity);
    m_Gradient.GetNumControlPoints(numRgb2, numAlpha2, numIntensity2);

    if (numRgb != numRgb2 || numAlpha != numAlpha2 || numIntensity != numIntensity2)
      clearSelection = true;
  }

  // const bool wasEmpty = m_Gradient.IsEmpty();

  m_Gradient = gradient;

  {
    xiiQtScopedUpdatesDisabled ud(this);

    // if (wasEmpty)
    //  GradientWidget->FrameExtents();

    if (clearSelection)
      GradientWidget->ClearSelectedCP();
  }

  UpdateCpUi();

  GradientWidget->update();
}

void xiiQtColorGradientEditorWidget::SetScrubberPosition(xiiUInt64 uiTick)
{
  GradientWidget->SetScrubberPosition(uiTick / 4800.0);
}

void xiiQtColorGradientEditorWidget::FrameGradient()
{
  GradientWidget->FrameExtents();
  GradientWidget->update();
}

void xiiQtColorGradientEditorWidget::on_ButtonFrame_clicked()
{
  FrameGradient();
}

void xiiQtColorGradientEditorWidget::on_GradientWidget_selectionChanged(xiiInt32 colorCP, xiiInt32 alphaCP, xiiInt32 intensityCP)
{
  m_iSelectedColorCP     = colorCP;
  m_iSelectedAlphaCP     = alphaCP;
  m_iSelectedIntensityCP = intensityCP;

  SpinPosition->setEnabled((m_iSelectedColorCP != -1) || (m_iSelectedAlphaCP != -1) || (m_iSelectedIntensityCP != -1));

  LabelColor->setVisible(m_iSelectedColorCP != -1);
  ButtonColor->setVisible(m_iSelectedColorCP != -1);

  LabelAlpha->setVisible(m_iSelectedAlphaCP != -1);
  SpinAlpha->setVisible(m_iSelectedAlphaCP != -1);
  SliderAlpha->setVisible(m_iSelectedAlphaCP != -1);

  LabelIntensity->setVisible(m_iSelectedIntensityCP != -1);
  SpinIntensity->setVisible(m_iSelectedIntensityCP != -1);

  UpdateCpUi();
}


void xiiQtColorGradientEditorWidget::on_SpinPosition_valueChanged(double value)
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpMoved(m_iSelectedColorCP, value);
  }
  else if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpMoved(m_iSelectedAlphaCP, value);
  }
  else if (m_iSelectedIntensityCP != -1)
  {
    Q_EMIT IntensityCpMoved(m_iSelectedIntensityCP, value);
  }
}


void xiiQtColorGradientEditorWidget::on_SpinAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}

void xiiQtColorGradientEditorWidget::on_SliderAlpha_valueChanged(int value)
{
  if (m_iSelectedAlphaCP != -1)
  {
    Q_EMIT AlphaCpChanged(m_iSelectedAlphaCP, value);
  }
}


void xiiQtColorGradientEditorWidget::on_SliderAlpha_sliderPressed()
{
  Q_EMIT BeginOperation();
}


void xiiQtColorGradientEditorWidget::on_SliderAlpha_sliderReleased()
{
  Q_EMIT EndOperation(true);
}

void xiiQtColorGradientEditorWidget::on_SpinIntensity_valueChanged(double value)
{
  if (m_iSelectedIntensityCP != -1)
  {
    Q_EMIT IntensityCpChanged(m_iSelectedIntensityCP, value);
  }
}


void xiiQtColorGradientEditorWidget::on_ButtonColor_clicked()
{
  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    m_PickColorStart   = xiiColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
    m_PickColorCurrent = m_PickColorStart;

    Q_EMIT BeginOperation();

    xiiQtUiServices::GetSingleton()->ShowColorDialog(
      m_PickColorStart, false, false, this, SLOT(onCurrentColorChanged(const xiiColor&)), SLOT(onColorAccepted()), SLOT(onColorReset()));
  }
}

void xiiQtColorGradientEditorWidget::onCurrentColorChanged(const xiiColor& col)
{
  if (m_iSelectedColorCP != -1)
  {
    m_PickColorCurrent = col;

    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);
  }
}

void xiiQtColorGradientEditorWidget::onColorAccepted()
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorCurrent);

    Q_EMIT EndOperation(true);
  }
}

void xiiQtColorGradientEditorWidget::onColorReset()
{
  if (m_iSelectedColorCP != -1)
  {
    Q_EMIT ColorCpChanged(m_iSelectedColorCP, m_PickColorStart);

    Q_EMIT EndOperation(false);
  }
}


void xiiQtColorGradientEditorWidget::on_ButtonNormalize_clicked()
{
  Q_EMIT NormalizeRange();
}

void xiiQtColorGradientEditorWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  ButtonColor->setPalette(m_Pal);
  QWidget::showEvent(event);
}

void xiiQtColorGradientEditorWidget::UpdateCpUi()
{
  xiiQtScopedBlockSignals    bs(this);
  xiiQtScopedUpdatesDisabled ud(this);

  if (m_iSelectedColorCP != -1)
  {
    const auto& cp = m_Gradient.GetColorControlPoint(m_iSelectedColorCP);

    SpinPosition->setValue(cp.m_PosX);

    QColor col;
    col.setRgb(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);

    ButtonColor->setAutoFillBackground(true);
    m_Pal.setColor(QPalette::Button, col);
    ButtonColor->setPalette(m_Pal);
  }

  if (m_iSelectedAlphaCP != -1)
  {
    SpinPosition->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_PosX);
    SpinAlpha->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_Alpha);
    SliderAlpha->setValue(m_Gradient.GetAlphaControlPoint(m_iSelectedAlphaCP).m_Alpha);
  }

  if (m_iSelectedIntensityCP != -1)
  {
    SpinPosition->setValue(m_Gradient.GetIntensityControlPoint(m_iSelectedIntensityCP).m_PosX);
    SpinIntensity->setValue(m_Gradient.GetIntensityControlPoint(m_iSelectedIntensityCP).m_Intensity);
  }
}
