/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_ColorGradientEditorWidget.h>

#include <QWidget>

class QMouseEvent;

class XII_GUIFOUNDATION_DLL xiiQtColorGradientEditorWidget : public QWidget, public Ui_ColorGradientEditorWidget
{
  Q_OBJECT

public:
  explicit xiiQtColorGradientEditorWidget(QWidget* pParent);
  ~xiiQtColorGradientEditorWidget();

  void                    SetColorGradient(const xiiColorGradient& gradient);
  const xiiColorGradient& GetColorGradient() const { return m_Gradient; }

  void ShowColorPicker() { on_ButtonColor_clicked(); }
  void SetScrubberPosition(xiiUInt64 uiTick);

  void FrameGradient();

Q_SIGNALS:
  void ColorCpAdded(double fPosX, const xiiColorGammaUB& color);
  void ColorCpMoved(xiiInt32 iIndex, float fNewPosX);
  void ColorCpDeleted(xiiInt32 iIndex);
  void ColorCpChanged(xiiInt32 iIndex, const xiiColorGammaUB& color);

  void AlphaCpAdded(double fPosX, xiiUInt8 uiAlpha);
  void AlphaCpMoved(xiiInt32 iIndex, double fNewPosX);
  void AlphaCpDeleted(xiiInt32 iIndex);
  void AlphaCpChanged(xiiInt32 iIndex, xiiUInt8 uiAlpha);

  void IntensityCpAdded(double fPosX, float fIntensity);
  void IntensityCpMoved(xiiInt32 iIndex, double fNewPosX);
  void IntensityCpDeleted(xiiInt32 iIndex);
  void IntensityCpChanged(xiiInt32 iIndex, float fIntensity);

  void NormalizeRange();

  void BeginOperation();
  void EndOperation(bool bCommit);

private Q_SLOTS:
  void on_ButtonFrame_clicked();
  void on_GradientWidget_selectionChanged(xiiInt32 colorCP, xiiInt32 alphaCP, xiiInt32 intensityCP);
  void on_SpinPosition_valueChanged(double value);
  void on_SpinAlpha_valueChanged(int value);
  void on_SliderAlpha_valueChanged(int value);
  void on_SliderAlpha_sliderPressed();
  void on_SliderAlpha_sliderReleased();
  void on_SpinIntensity_valueChanged(double value);
  void on_ButtonColor_clicked();
  void onCurrentColorChanged(const xiiColor& col);
  void onColorAccepted();
  void onColorReset();
  void on_ButtonNormalize_clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;

private:
  void UpdateCpUi();

  QPalette         m_Pal;
  xiiInt32         m_iSelectedColorCP;
  xiiInt32         m_iSelectedAlphaCP;
  xiiInt32         m_iSelectedIntensityCP;
  xiiColorGradient m_Gradient;

  xiiColorGammaUB m_PickColorStart;
  xiiColorGammaUB m_PickColorCurrent;
};
