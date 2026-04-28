/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QBoxLayout>
#include <QSlider>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiQtVarianceTypeWidget::xiiQtVarianceTypeWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pValueWidget = new xiiQtDoubleSpinBox(this);
  m_pValueWidget->installEventFilter(m_pValueWidget);
  m_pValueWidget->setMinimum(-xiiMath::Infinity<double>());
  m_pValueWidget->setMaximum(xiiMath::Infinity<double>());
  m_pValueWidget->setSingleStep(0.1f);
  m_pValueWidget->setAccelerated(true);
  m_pValueWidget->setDecimals(2);

  m_pVarianceWidget = new QSlider(this);
  m_pVarianceWidget->setOrientation(Qt::Orientation::Horizontal);
  m_pVarianceWidget->setMinimum(0);
  m_pVarianceWidget->setMaximum(100);
  m_pVarianceWidget->setSingleStep(1);

  QLabel* pText = new QLabel("Variance:");
  pText->setToolTip("Random deviation of base value:\nSlider to the left -> 0 variance, no randomness at all.\nSlider in the middle -> 0.5 variance, value will be in range [0.5 * base ... 1.5 * base]\nSlider to the right -> full variance, value will be in range [0 ... 2 * base]\n\nNote that values deviate from base using a Bell curve, meaning that values close to 'base' are more likely.");

  m_pLayout->addWidget(m_pValueWidget);
  m_pLayout->addWidget(pText);
  m_pLayout->addWidget(m_pVarianceWidget);

  connect(m_pValueWidget, SIGNAL(editingFinished()), this, SLOT(onEndTemporary()));
  connect(m_pValueWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  connect(m_pVarianceWidget, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pVarianceWidget, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pVarianceWidget, SIGNAL(valueChanged(int)), this, SLOT(SlotVarianceChanged()));
}

void xiiQtVarianceTypeWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtStandardPropertyWidget::SetSelection(items);
  XII_ASSERT_DEBUG(m_pProp->GetSpecificType()->IsDerivedFrom<xiiVarianceTypeBaseFloat>() || m_pProp->GetSpecificType()->IsDerivedFrom<xiiVarianceTypeDouble>(), "Selection does not match xiiVarianceType.");
}

void xiiQtVarianceTypeWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void xiiQtVarianceTypeWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtVarianceTypeWidget::SlotValueChanged()
{
  onBeginTemporary();

  xiiVariant value;
  xiiToolsReflectionUtils::GetVariantFromFloat(m_pValueWidget->value(), m_pValueProp->GetSpecificType()->GetVariantType(), value);

  auto  obj   = m_OldValue.Get<xiiTypedObject>();
  void* pCopy = xiiReflectionSerializer::Clone(obj.m_pObject, obj.m_pType);
  xiiReflectionUtils::SetMemberPropertyValue(m_pValueProp, pCopy, value);
  xiiVariant newValue;
  newValue.MoveTypedObject(pCopy, obj.m_pType);

  BroadcastValueChanged(newValue);
}

void xiiQtVarianceTypeWidget::SlotVarianceChanged()
{
  double variance = xiiMath::Clamp<double>(m_pVarianceWidget->value() / 100.0, 0, 1);

  xiiVariant      newValue = m_OldValue;
  xiiTypedPointer ptr      = newValue.GetWriteAccess();
  xiiReflectionUtils::SetMemberPropertyValue(m_pVarianceProp, ptr.m_pObject, variance);

  BroadcastValueChanged(newValue);
}

void xiiQtVarianceTypeWidget::OnInit()
{
  m_pValueProp    = static_cast<const xiiAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Value"));
  m_pVarianceProp = static_cast<const xiiAbstractMemberProperty*>(GetProperty()->GetSpecificType()->FindPropertyByName("Variance"));

  // Property type adjustments
  xiiQtScopedBlockSignals bs(m_pValueWidget);
  const xiiRTTI*          pValueType = m_pValueProp->GetSpecificType();
  if (pValueType == xiiGetStaticRTTI<xiiTime>())
  {
    m_pValueWidget->setDisplaySuffix(" sec");
  }
  else if (pValueType == xiiGetStaticRTTI<xiiAngle>())
  {
    m_pValueWidget->setDisplaySuffix(xiiStringUtf8(L"\u00B0").GetData());
  }
  else if (pValueType == xiiGetStaticRTTI<xiiAngled>())
  {
    m_pValueWidget->setDisplaySuffix(xiiStringUtf8(L"\u00B0").GetData());
  }

  // Handle attributes
  if (const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>())
  {
    m_pValueWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }
  if (const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>())
  {
    if (pClamp->GetMinValue().CanConvertTo<double>() || pClamp->GetMinValue().IsA<xiiTime>() || pClamp->GetMinValue().IsA<xiiAngle>() || pClamp->GetMinValue().IsA<xiiAngled>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue());
    }
    else if (const xiiRTTI* pType = pClamp->GetMinValue().GetReflectedType(); pType && pType->IsDerivedFrom<xiiVarianceTypeFloat>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue()["Value"]);
      m_pVarianceWidget->setMinimum(static_cast<xiiInt32>(pClamp->GetMinValue()["Variance"].ConvertTo<double>() * 100.0));
    }
    else if (const xiiRTTI* pType = pClamp->GetMinValue().GetReflectedType(); pType && pType->IsDerivedFrom<xiiVarianceTypeDouble>())
    {
      m_pValueWidget->setMinimum(pClamp->GetMinValue()["Value"]);
      m_pVarianceWidget->setMinimum(static_cast<xiiInt32>(pClamp->GetMinValue()["Variance"].ConvertTo<double>() * 100.0));
    }
    if (pClamp->GetMaxValue().CanConvertTo<double>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue());
    }
    else if (const xiiRTTI* pType = pClamp->GetMaxValue().GetReflectedType(); pType && pType->IsDerivedFrom<xiiVarianceTypeFloat>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue()["Value"]);
      m_pVarianceWidget->setMaximum(static_cast<xiiInt32>(pClamp->GetMaxValue()["Variance"].ConvertTo<double>() * 100.0));
    }
    else if (const xiiRTTI* pType = pClamp->GetMaxValue().GetReflectedType(); pType && pType->IsDerivedFrom<xiiVarianceTypeDouble>())
    {
      m_pValueWidget->setMaximum(pClamp->GetMaxValue()["Value"]);
      m_pVarianceWidget->setMaximum(static_cast<xiiInt32>(pClamp->GetMaxValue()["Variance"].ConvertTo<double>() * 100.0));
    }
  }
  if (const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    if (pDefault->GetValue().CanConvertTo<double>() || pDefault->GetValue().IsA<xiiTime>() || pDefault->GetValue().IsA<xiiAngle>() || pDefault->GetValue().IsA<xiiAngled>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue());
    }
    else if (const xiiRTTI* pType = pDefault->GetValue().GetReflectedType(); pType && pType->IsDerivedFrom<xiiVarianceTypeFloat>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue()["Value"]);
    }
    else if (const xiiRTTI* pType = pDefault->GetValue().GetReflectedType(); pType && pType->IsDerivedFrom<xiiVarianceTypeDouble>())
    {
      m_pValueWidget->setDefaultValue(pDefault->GetValue()["Value"]);
    }
  }
}

void xiiQtVarianceTypeWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pValueWidget, m_pVarianceWidget);
  if (value.IsValid())
  {
    m_pValueWidget->setValue(value["Value"]);
    m_pVarianceWidget->setValue(value["Variance"].ConvertTo<double>() * 100.0);
  }
  else
  {
    m_pValueWidget->setValueInvalid();
    m_pVarianceWidget->setValue(50);
  }
}
