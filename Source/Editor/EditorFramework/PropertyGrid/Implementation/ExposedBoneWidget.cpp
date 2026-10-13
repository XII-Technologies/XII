/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/ExposedBoneWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QBoxLayout>

xiiQtExposedBoneWidget::xiiQtExposedBoneWidget()
{
  m_pRotWidget[0] = nullptr;
  m_pRotWidget[1] = nullptr;
  m_pRotWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (xiiInt32 c = 0; c < 3; ++c)
  {
    m_pRotWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pRotWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pRotWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pRotWidget[c]->setSingleStep(1.0);
    m_pRotWidget[c]->setAccelerated(true);
    m_pRotWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pRotWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pRotWidget[c]);

    connect(m_pRotWidget[c], SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
    connect(m_pRotWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtExposedBoneWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtStandardPropertyWidget::SetSelection(items);
  XII_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<xiiExposedBone>(), "Selection does not match xiiExposedBone.");
}

void xiiQtExposedBoneWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void xiiQtExposedBoneWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtExposedBoneWidget::SlotValueChanged()
{
  if (m_OldValue.GetReflectedType() != xiiGetStaticRTTI<xiiExposedBone>())
    return;

  onBeginTemporary();

  xiiExposedBone bone = m_OldValue.Get<xiiExposedBone>();

  {
    xiiAngle x = xiiAngle::MakeFromDegree(m_pRotWidget[0]->value());
    xiiAngle y = xiiAngle::MakeFromDegree(m_pRotWidget[1]->value());
    xiiAngle z = xiiAngle::MakeFromDegree(m_pRotWidget[2]->value());

    bone.m_Transform.m_qRotation = xiiQuat::MakeFromEulerAngles(x, y, z);
  }

  BroadcastValueChanged(xiiVariant(bone));
}

void xiiQtExposedBoneWidget::OnInit()
{
}

void xiiQtExposedBoneWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b0(m_pRotWidget[0]);
  xiiQtScopedBlockSignals b1(m_pRotWidget[1]);
  xiiQtScopedBlockSignals b2(m_pRotWidget[2]);

  if (value.GetReflectedType() == xiiGetStaticRTTI<xiiExposedBone>())
  {
    const xiiExposedBone* pBone = static_cast<const xiiExposedBone*>(value.GetData());
    xiiAngle              x, y, z;
    pBone->m_Transform.m_qRotation.GetAsEulerAngles(x, y, z);

    m_pRotWidget[0]->setValue(x.GetDegree());
    m_pRotWidget[1]->setValue(y.GetDegree());
    m_pRotWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pRotWidget[0]->setValueInvalid();
    m_pRotWidget[1]->setValueInvalid();
    m_pRotWidget[2]->setValueInvalid();
  }
}
