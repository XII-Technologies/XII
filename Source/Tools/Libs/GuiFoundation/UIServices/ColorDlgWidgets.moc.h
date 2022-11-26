#pragma once

#include <Foundation/Math/Color.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QWidget>

class XII_GUIFOUNDATION_DLL xiiQtColorAreaWidget : public QWidget
{
  Q_OBJECT
public:
  xiiQtColorAreaWidget(QWidget* parent);

  float GetHue() const { return m_fHue; }
  void  SetHue(float hue);

  float GetSaturation() const { return m_fSaturation; }
  void  SetSaturation(float sat);

  float GetValue() const { return m_fValue; }
  void  SetValue(float val);

Q_SIGNALS:
  void valueChanged(double x, double y);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float  m_fHue;
  float  m_fSaturation;
  float  m_fValue;
};

class XII_GUIFOUNDATION_DLL xiiQtColorRangeWidget : public QWidget
{
  Q_OBJECT
public:
  xiiQtColorRangeWidget(QWidget* parent);

  float GetHue() const { return m_fHue; }
  void  SetHue(float hue);

Q_SIGNALS:
  void valueChanged(double x);

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;

  void UpdateImage();

  QImage m_Image;
  float  m_fHue;
};

class XII_GUIFOUNDATION_DLL xiiQtColorCompareWidget : public QWidget
{
  Q_OBJECT
public:
  xiiQtColorCompareWidget(QWidget* parent);

  void SetNewColor(const xiiColor& color);
  void SetInitialColor(const xiiColor& color);

protected:
  virtual void paintEvent(QPaintEvent*) override;

  xiiColor m_InitialColor;
  xiiColor m_NewColor;
};
