/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/ColorDlgWidgets.moc.h>
#include <GuiFoundation/ui_ColorDialog.h>
#include <QDialog>

class QLineEdit;
class xiiQtDoubleSpinBox;
class QPushButton;
class QSlider;


class XII_GUIFOUNDATION_DLL xiiQtColorDialog : public QDialog, Ui_ColorDialog
{
  Q_OBJECT
public:
  xiiQtColorDialog(const xiiColor& initial, QWidget* pParent);
  ~xiiQtColorDialog();

  void ShowAlpha(bool bEnable);
  void ShowHDR(bool bEnable);

  static QByteArray GetLastDialogGeometry() { return s_LastDialogGeometry; }

Q_SIGNALS:
  void CurrentColorChanged(const xiiColor& color);
  void ColorSelected(const xiiColor& color);

private Q_SLOTS:
  void ChangedRGB();
  void ChangedAlpha();
  void ChangedExposure();
  void ChangedHSV();
  void ChangedArea(double x, double y);
  void ChangedRange(double x);
  void ChangedHEX();

private:
  bool m_bAlpha;
  bool m_bHDR;

  float m_fHue;
  float m_fSaturation;
  float m_fValue;

  xiiUInt16 m_uiHue;
  xiiUInt8  m_uiSaturation;

  xiiUInt8 m_uiGammaRed;
  xiiUInt8 m_uiGammaGreen;
  xiiUInt8 m_uiGammaBlue;

  xiiUInt8 m_uiAlpha;
  float    m_fExposureValue;

  xiiColor m_CurrentColor;

  static QByteArray s_LastDialogGeometry;

private:
  void ApplyColor();

  void RecomputeHDR();

  void ExtractColorRGB();
  void ExtractColorHSV();

  void ComputeRgbAndHsv(const xiiColor& color);
  void RecomputeRGB();
  void RecomputeHSV();
};
