#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/UIServices/ColorDialog.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

QByteArray xiiQtColorDialog::s_LastDialogGeometry;

void xiiQtUiServices::ShowColorDialog(
  const xiiColor& color,
  bool            bAlpha,
  bool            bHDR,
  QWidget*        pParent,
  const char*     szSlotCurColChanged,
  const char*     szSlotAccept,
  const char*     szSlotReject)
{
  m_pColorDlg = new xiiQtColorDialog(color, pParent);
  m_pColorDlg->restoreGeometry(m_ColorDlgGeometry);
  m_pColorDlg->ShowAlpha(bAlpha);
  m_pColorDlg->ShowHDR(bHDR);

  XII_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(CurrentColorChanged(const xiiColor&)), pParent, szSlotCurColChanged) != nullptr,
             "signal/slot connection failed");
  XII_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(accepted()), pParent, szSlotAccept) != nullptr, "signal/slot connection failed");
  XII_VERIFY(QWidget::connect(m_pColorDlg, SIGNAL(rejected()), pParent, szSlotReject) != nullptr, "signal/slot connection failed");

  m_pColorDlg->exec();
  delete m_pColorDlg;
  m_pColorDlg = nullptr;

  m_ColorDlgGeometry = xiiQtColorDialog::GetLastDialogGeometry();
}

xiiQtColorDialog::xiiQtColorDialog(const xiiColor& initial, QWidget* pParent) :
  QDialog(pParent)
{
  setupUi(this);

  m_CurrentColor = initial;
  m_bAlpha       = false;
  m_bHDR         = false;

  m_fExposureValue = initial.ComputeHdrExposureValue();
  ComputeRgbAndHsv(initial);

  {
    SpinRed->SetIntMode(true);
    SpinGreen->SetIntMode(true);
    SpinBlue->SetIntMode(true);
    SpinAlpha->SetIntMode(true);

    SpinHue->SetIntMode(true);
    SpinSaturation->SetIntMode(true);
    SpinValue->SetIntMode(true);
    // SpinValue->setDecimals(1);

    ButtonOk->setAutoDefault(false);
    ButtonCancel->setAutoDefault(false);

    ButtonOk->setDefault(false);
    ButtonCancel->setDefault(false);
  }

  {
    connect(SpinHue, SIGNAL(valueChanged(double)), this, SLOT(ChangedHSV()));
    connect(SpinSaturation, SIGNAL(valueChanged(double)), this, SLOT(ChangedHSV()));
    connect(SpinValue, SIGNAL(valueChanged(double)), this, SLOT(ChangedHSV()));
    connect(SpinRed, SIGNAL(valueChanged(double)), this, SLOT(ChangedRGB()));
    connect(SpinGreen, SIGNAL(valueChanged(double)), this, SLOT(ChangedRGB()));
    connect(SpinBlue, SIGNAL(valueChanged(double)), this, SLOT(ChangedRGB()));
    connect(SpinAlpha, SIGNAL(valueChanged(double)), this, SLOT(ChangedAlpha()));
    connect(SliderExposure, SIGNAL(valueChanged(int)), this, SLOT(ChangedExposure()));
    connect(ColorArea, SIGNAL(valueChanged(double, double)), this, SLOT(ChangedArea(double, double)));
    connect(ColorRange, SIGNAL(valueChanged(double)), this, SLOT(ChangedRange(double)));
    connect(LineHEX, SIGNAL(editingFinished()), this, SLOT(ChangedHEX()));
    connect(ButtonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(ButtonCancel, SIGNAL(clicked()), this, SLOT(reject()));
  }

  ColorCompare->SetInitialColor(initial);

  ShowAlpha(m_bAlpha);
  ShowHDR(m_bHDR);
  ApplyColor();
}

xiiQtColorDialog::~xiiQtColorDialog()
{
  s_LastDialogGeometry = saveGeometry();
}

void xiiQtColorDialog::ShowAlpha(bool bEnable)
{
  m_bAlpha = bEnable;
  SpinAlpha->setVisible(bEnable);
  LabelAlpha->setVisible(bEnable);

  ApplyColor();
}

void xiiQtColorDialog::ShowHDR(bool bEnable)
{
  m_bHDR = bEnable;
  LineRed32->setVisible(bEnable);
  LineGreen32->setVisible(bEnable);
  LineBlue32->setVisible(bEnable);
  LabelExposure->setVisible(bEnable);
  LineExposure->setVisible(bEnable);
  SliderExposure->setVisible(bEnable);
  LabelR32->setVisible(bEnable);
  LabelG32->setVisible(bEnable);
  LabelB32->setVisible(bEnable);

  ApplyColor();
}

void xiiQtColorDialog::ApplyColor()
{
  xiiQtScopedBlockSignals _0(SpinAlpha, SliderExposure, LineExposure);
  xiiQtScopedBlockSignals _1(SpinRed, SpinGreen, SpinBlue);
  xiiQtScopedBlockSignals _2(SpinHue, SpinSaturation, SpinValue);
  xiiQtScopedBlockSignals _4(LineRed32, LineGreen32, LineBlue32);
  xiiQtScopedBlockSignals _3(ColorRange, ColorArea);

  SpinAlpha->setValue(m_uiAlpha);
  SliderExposure->setValue(m_fExposureValue * 100.0f);
  LineExposure->setText(QString("+%1").arg(m_fExposureValue, 0, 'f', 2));

  SpinRed->setValue(m_uiGammaRed);
  SpinGreen->setValue(m_uiGammaGreen);
  SpinBlue->setValue(m_uiGammaBlue);

  SpinHue->setValue(m_uiHue);
  SpinSaturation->setValue(m_uiSaturation);
  SpinValue->setValue(m_fValue * 100.0);

  LineRed32->setText(QString("%1").arg(m_CurrentColor.r, 0, 'f', 2));
  LineGreen32->setText(QString("%1").arg(m_CurrentColor.g, 0, 'f', 2));
  LineBlue32->setText(QString("%1").arg(m_CurrentColor.b, 0, 'f', 2));

  ColorRange->SetHue(m_uiHue);
  ColorArea->SetHue(m_uiHue);
  ColorArea->SetSaturation(m_uiSaturation / 100.0f);
  ColorArea->SetValue(m_fValue);
  ColorCompare->SetNewColor(m_CurrentColor);

  xiiStringBuilder s;

  if (m_bAlpha)
  {
    s.Format("{0}{1}{2}{3}", xiiArgU(m_uiGammaRed, 2, true, 16, true), xiiArgU(m_uiGammaGreen, 2, true, 16, true), xiiArgU(m_uiGammaBlue, 2, true, 16, true),
             xiiArgU(m_uiAlpha, 2, true, 16, true));
  }
  else
  {
    s.Format("{0}{1}{2}", xiiArgU(m_uiGammaRed, 2, true, 16, true), xiiArgU(m_uiGammaGreen, 2, true, 16, true), xiiArgU(m_uiGammaBlue, 2, true, 16, true));
  }

  LineHEX->setText(s.GetData());
}

void xiiQtColorDialog::ChangedRGB()
{
  ExtractColorRGB();
  ApplyColor();
  Q_EMIT CurrentColorChanged(m_CurrentColor);
}

void xiiQtColorDialog::ChangedHSV()
{
  ExtractColorHSV();
  ApplyColor();
  Q_EMIT CurrentColorChanged(m_CurrentColor);
}

void xiiQtColorDialog::ChangedAlpha()
{
  m_uiAlpha        = (xiiUInt8)SpinAlpha->value();
  m_CurrentColor.a = xiiMath::ColorByteToFloat(m_uiAlpha);

  ApplyColor();
  Q_EMIT CurrentColorChanged(m_CurrentColor);
}

void xiiQtColorDialog::ChangedExposure()
{
  m_fExposureValue = SliderExposure->value() / 100.0f;

  RecomputeHDR();
  ApplyColor();
  Q_EMIT CurrentColorChanged(m_CurrentColor);
}

void xiiQtColorDialog::ChangedArea(double x, double y)
{
  m_uiSaturation = xiiMath::Min<xiiUInt8>(xiiMath::Round(x * 100.0), 100);
  m_fSaturation  = xiiMath::Clamp((float)x, 0.0f, 1.0f);
  m_fValue       = xiiMath::Clamp((float)y, 0.0f, 1.0f);

  RecomputeRGB();
  RecomputeHDR();

  ApplyColor();
  Q_EMIT CurrentColorChanged(m_CurrentColor);
}

void xiiQtColorDialog::ChangedRange(double x)
{
  m_uiHue = xiiMath::Clamp<xiiUInt16>(xiiMath::Round(x * 359), 0, 359);
  m_fHue  = (float)m_uiHue;

  RecomputeRGB();
  RecomputeHDR();

  ApplyColor();
  Q_EMIT CurrentColorChanged(m_CurrentColor);
}

void xiiQtColorDialog::ChangedHEX()
{
  xiiStringBuilder text = LineHEX->text().toUtf8().data();

  if (!text.StartsWith("#"))
  {
    text.Prepend("#");
  }

  bool     valid = false;
  xiiColor col   = xiiConversionUtils::GetColorByName(text, &valid);

  if (valid)
  {
    ComputeRgbAndHsv(col);
    RecomputeHDR();
  }

  ApplyColor();

  if (valid)
  {
    Q_EMIT CurrentColorChanged(m_CurrentColor);
  }
}

void xiiQtColorDialog::ExtractColorRGB()
{
  m_uiGammaRed   = (xiiUInt8)SpinRed->value();
  m_uiGammaGreen = (xiiUInt8)SpinGreen->value();
  m_uiGammaBlue  = (xiiUInt8)SpinBlue->value();

  RecomputeHSV();
  RecomputeHDR();
}

void xiiQtColorDialog::ExtractColorHSV()
{
  m_uiHue        = xiiMath::Clamp<float>(SpinHue->value(), 0, 359);
  m_uiSaturation = xiiMath::Min<xiiUInt8>(SpinSaturation->value(), 100);
  m_fValue       = xiiMath::Min<float>(SpinValue->value(), 100.0f);

  m_fHue        = (float)m_uiHue;
  m_fSaturation = xiiMath::Clamp(m_uiSaturation / 100.0f, 0.0f, 1.0f);
  m_fValue      = xiiMath::Clamp(m_fValue / 100.0f, 0.0f, 1.0f);

  RecomputeRGB();
  RecomputeHDR();
}

void xiiQtColorDialog::ComputeRgbAndHsv(const xiiColor& color)
{
  xiiColor ldrColor = color;
  ldrColor.NormalizeToLdrRange();

  xiiColorGammaUB gamma = ldrColor;
  m_uiGammaRed          = gamma.r;
  m_uiGammaGreen        = gamma.g;
  m_uiGammaBlue         = gamma.b;
  m_uiAlpha             = gamma.a;

  RecomputeHSV();
}

void xiiQtColorDialog::RecomputeRGB()
{
  xiiColor col;
  col.SetHSV(m_fHue, m_fSaturation, m_fValue);
  xiiColorGammaUB colGamma = col;

  m_uiGammaRed   = colGamma.r;
  m_uiGammaGreen = colGamma.g;
  m_uiGammaBlue  = colGamma.b;
}

void xiiQtColorDialog::RecomputeHSV()
{
  xiiColorGammaUB colGamma(m_uiGammaRed, m_uiGammaGreen, m_uiGammaBlue, 255);
  xiiColor        color = colGamma;

  color.GetHSV(m_fHue, m_fSaturation, m_fValue);

  m_uiHue        = (xiiUInt16)xiiMath::Round(m_fHue);
  m_uiSaturation = xiiMath::Min<xiiUInt8>(xiiMath::Round(m_fSaturation * 100.0), 100);
}

void xiiQtColorDialog::RecomputeHDR()
{
  m_CurrentColor.SetHSV(m_fHue, m_fSaturation, m_fValue);
  m_CurrentColor.ApplyHdrExposureValue(m_fExposureValue);
  m_CurrentColor.a = xiiMath::ColorByteToFloat(m_uiAlpha);
}
