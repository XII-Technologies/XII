#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/VarianceTypes.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

class QSlider;

class xiiQtVarianceTypeWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtVarianceTypeWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();
  void SlotVarianceChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  bool                       m_bTemporaryCommand = false;
  QHBoxLayout*               m_pLayout           = nullptr;
  xiiQtDoubleSpinBox*        m_pValueWidget      = nullptr;
  QSlider*                   m_pVarianceWidget   = nullptr;
  xiiAbstractMemberProperty* m_pValueProp        = nullptr;
  xiiAbstractMemberProperty* m_pVarianceProp     = nullptr;
};
