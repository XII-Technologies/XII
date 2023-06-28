#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

#include <QFrame>
#include <QLabel>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QComboBox;
class QStandardItemModel;
class QStandardItem;
class QToolButton;
class QMenu;
class xiiDocumentObject;
class xiiQtDoubleSpinBox;
class QSlider;

/// *** CHECKBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorCheckboxWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorCheckboxWidget();

  virtual void mousePressEvent(QMouseEvent* pEv) override;

private Q_SLOTS:
  void on_StateChanged_triggered(int state);

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const xiiVariant& value) override;

  QHBoxLayout* m_pLayout;
  QCheckBox*   m_pWidget;
};



/// *** DOUBLE SPINBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorDoubleSpinboxWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorDoubleSpinboxWidget(xiiInt8 iNumComponents);

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  bool                    m_bUseTemporaryTransaction = false;
  bool                    m_bTemporaryCommand        = false;
  xiiInt8                 m_iNumComponents           = 0;
  xiiEnum<xiiVariantType> m_OriginalType;
  QHBoxLayout*            m_pLayout    = nullptr;
  xiiQtDoubleSpinBox*     m_pWidget[4] = {};
};

/// *** TIME SPINBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorTimeWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorTimeWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  bool                m_bTemporaryCommand;
  QHBoxLayout*        m_pLayout;
  xiiQtDoubleSpinBox* m_pWidget;
};

/// *** ANGLE SPINBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorAngleWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorAngleWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  bool                m_bTemporaryCommand;
  QHBoxLayout*        m_pLayout;
  xiiQtDoubleSpinBox* m_pWidget;
};

/// *** INT SPINBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorIntSpinboxWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorIntSpinboxWidget(xiiInt8 iNumComponents, xiiInt32 iMinValue, xiiInt32 iMaxValue);
  ~xiiQtPropertyEditorIntSpinboxWidget();

private Q_SLOTS:
  void SlotValueChanged();
  void SlotSliderValueChanged(int value);
  void on_EditingFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  bool                    m_bUseTemporaryTransaction = false;
  bool                    m_bTemporaryCommand        = false;
  xiiInt8                 m_iNumComponents           = 0;
  xiiEnum<xiiVariantType> m_OriginalType;
  QHBoxLayout*            m_pLayout    = nullptr;
  xiiQtDoubleSpinBox*     m_pWidget[4] = {};
  QSlider*                m_pSlider    = nullptr;
};

/// *** QUATERNION ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorQuaternionWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorQuaternionWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  bool                m_bTemporaryCommand;
  QHBoxLayout*        m_pLayout;
  xiiQtDoubleSpinBox* m_pWidget[3];
};


/// *** LINEEDIT ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorLineEditWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorLineEditWidget();

protected Q_SLOTS:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QLineEdit*   m_pWidget;
};


/// *** COLOR ***

class XII_GUIFOUNDATION_DLL xiiQtColorButtonWidget : public QFrame
{
  Q_OBJECT

public:
  explicit xiiQtColorButtonWidget(QWidget* pParent);
  void SetColor(const xiiVariant& color);

Q_SIGNALS:
  void clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  QPalette m_Pal;
};

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorColorWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorColorWidget();

private Q_SLOTS:
  void on_Button_triggered();
  void on_CurrentColor_changed(const xiiColor& color);
  void on_Color_reset();
  void on_Color_accepted();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  bool                    m_bExposeAlpha;
  QHBoxLayout*            m_pLayout;
  xiiQtColorButtonWidget* m_pWidget;
  xiiVariant              m_OriginalValue;
};


/// *** ENUM COMBOBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorEnumWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorEnumWidget();

private Q_SLOTS:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QComboBox*   m_pWidget;
  xiiInt64     m_iCurrentEnum;
};


/// *** BITFLAGS COMBOBOX ***

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorBitflagsWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorBitflagsWidget();
  virtual ~xiiQtPropertyEditorBitflagsWidget();

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void on_Menu_aboutToHide();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  xiiMap<xiiInt64, QCheckBox*> m_Constants;
  QHBoxLayout*                 m_pLayout;
  QPushButton*                 m_pWidget;
  QMenu*                       m_pMenu;
  xiiInt64                     m_iCurrentBitflags;
};


/// *** CURVE1D ***

class XII_GUIFOUNDATION_DLL xiiQtCurve1DButtonWidget : public QLabel
{
  Q_OBJECT

public:
  explicit xiiQtCurve1DButtonWidget(QWidget* pParent);

  void UpdatePreview(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange);

Q_SIGNALS:
  void clicked();

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
};

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorCurve1DWidget : public xiiQtPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorCurve1DWidget();

private Q_SLOTS:
  void on_Button_triggered();

protected:
  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  void         UpdatePreview();

protected:
  QHBoxLayout*              m_pLayout = nullptr;
  xiiQtCurve1DButtonWidget* m_pButton = nullptr;
};
