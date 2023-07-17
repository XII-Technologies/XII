#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDoubleSpinBox>
class xiiVariant;

class XII_GUIFOUNDATION_DLL xiiQtDoubleSpinBox : public QDoubleSpinBox
{
  Q_OBJECT
public:
  explicit xiiQtDoubleSpinBox(QWidget* pParent, bool bIntMode = false);

  void SetIntMode(bool bEnable);

  void setDisplaySuffix(xiiStringView sSuffix);
  void setDefaultValue(double value);
  void setDefaultValue(const xiiVariant& val);
  using QDoubleSpinBox::setMaximum;
  using QDoubleSpinBox::setMinimum;
  void setMinimum(const xiiVariant& val);
  void setMaximum(const xiiVariant& val);

  virtual QString textFromValue(double fVal) const override;
  virtual double  valueFromText(const QString& sText) const override;

  void   setValueInvalid();
  void   setValue(double fVal);
  void   setValue(const xiiVariant& val);
  double value() const;

protected:
  virtual void focusInEvent(QFocusEvent* event) override;
  virtual void focusOutEvent(QFocusEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual bool event(QEvent* event) override;

private Q_SLOTS:
  void onCustomContextMenuRequested();

private:
  QString         m_sSuffix;
  double          m_fDefaultValue;
  mutable double  m_fDisplayedValue;
  mutable QString m_sDisplayedText;
  mutable bool    m_bInvalid;
  bool            m_bModified;
  bool            m_bIntMode;
  bool            m_bDragging;
  double          m_fStartDragValue;
  QPoint          m_LastDragPos;
  xiiInt32        m_iDragDelta;
};
