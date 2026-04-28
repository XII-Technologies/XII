/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

class QSlider;

class xiiQtExposedBoneWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtExposedBoneWidget();

  virtual void SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items) override;

private Q_SLOTS:
  void onBeginTemporary();
  void onEndTemporary();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

  bool                m_bTemporaryCommand = false;
  QHBoxLayout*        m_pLayout           = nullptr;
  xiiQtDoubleSpinBox* m_pRotWidget[3];
};
