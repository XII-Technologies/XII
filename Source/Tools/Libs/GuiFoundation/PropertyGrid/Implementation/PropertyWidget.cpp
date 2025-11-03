#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QWidgetAction>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <qcheckbox.h>
#include <qlayout.h>

/// *** CHECKBOX ***

xiiQtPropertyEditorCheckboxWidget::xiiQtPropertyEditorCheckboxWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QCheckBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(stateChanged(int)), this, SLOT(on_StateChanged_triggered(int))) != nullptr, "signal/slot connection failed");
}

void xiiQtPropertyEditorCheckboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_pWidget->setTristate(false);
    m_pWidget->setChecked(value.ConvertTo<bool>() ? Qt::Checked : Qt::Unchecked);
  }
  else
  {
    m_pWidget->setTristate(true);
    m_pWidget->setCheckState(Qt::CheckState::PartiallyChecked);
  }
}

void xiiQtPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* pEv)
{
  QWidget::mousePressEvent(pEv);

  m_pWidget->toggle();
}

void xiiQtPropertyEditorCheckboxWidget::on_StateChanged_triggered(int state)
{
  if (state == Qt::PartiallyChecked)
  {
    xiiQtScopedBlockSignals b(m_pWidget);

    m_pWidget->setCheckState(Qt::Checked);
    m_pWidget->setTristate(false);
  }

  BroadcastValueChanged((state != Qt::Unchecked) ? true : false);
}

/// *** FLOAT SPINBOX ***

xiiQtPropertyEditorFloatSpinboxWidget::xiiQtPropertyEditorFloatSpinboxWidget(xiiInt8 iNumComponents) :
  xiiQtStandardPropertyWidget()
{
  XII_ASSERT_DEBUG(iNumComponents <= 4, "Only up to 4 components are supported");

  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  xiiStringView         sLabels[]     = {"X", "Y", "Z", "W"};
  const xiiColorGammaUB labelColors[] = {xiiColorScheme::LightUI(xiiColorScheme::Red), xiiColorScheme::LightUI(xiiColorScheme::Green), xiiColorScheme::LightUI(xiiColorScheme::Blue), xiiColorScheme::LightUI(xiiColorScheme::Gray)};

  for (xiiInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    if (m_iNumComponents > 1)
    {
      QLabel*  pLabel  = new QLabel(xiiMakeQString(sLabels[c]));
      QPalette palette = pLabel->palette();
      palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
      pLabel->setPalette(palette);
      m_pLayout->addWidget(pLabel);
    }

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorFloatSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<xiiNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction    = (pNoTemporaryTransactions == nullptr);

  if (const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());
        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec2>())
        {
          xiiVec2 value = pClamp->GetMinValue().ConvertTo<xiiVec2>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec2>())
        {
          xiiVec2 value = pClamp->GetMaxValue().ConvertTo<xiiVec2>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec3>())
        {
          xiiVec3 value = pClamp->GetMinValue().ConvertTo<xiiVec3>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec3>())
        {
          xiiVec3 value = pClamp->GetMaxValue().ConvertTo<xiiVec3>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec4>())
        {
          xiiVec4 value = pClamp->GetMinValue().ConvertTo<xiiVec4>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec4>())
        {
          xiiVec4 value = pClamp->GetMaxValue().ConvertTo<xiiVec4>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0]);

        if (pDefault->GetValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<double>());
        }
        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<xiiVec2>())
        {
          xiiVec2 value = pDefault->GetValue().ConvertTo<xiiVec2>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<xiiVec3>())
        {
          xiiVec3 value = pDefault->GetValue().ConvertTo<xiiVec3>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<xiiVec4>())
        {
          xiiVec4 value = pDefault->GetValue().ConvertTo<xiiVec4>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(xiiMakeQString(pMinValueText->GetText()));
    }
  }
}

void xiiQtPropertyEditorFloatSpinboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();
  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = xiiVariantType::Float;
  }

  if (value.IsValid())
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValue(value.ConvertTo<float>());
        break;
      case 2:
        m_pWidget[0]->setValue(value.ConvertTo<xiiVec2>().x);
        m_pWidget[1]->setValue(value.ConvertTo<xiiVec2>().y);
        break;
      case 3:
        m_pWidget[0]->setValue(value.ConvertTo<xiiVec3>().x);
        m_pWidget[1]->setValue(value.ConvertTo<xiiVec3>().y);
        m_pWidget[2]->setValue(value.ConvertTo<xiiVec3>().z);
        break;
      case 4:
        m_pWidget[0]->setValue(value.ConvertTo<xiiVec4>().x);
        m_pWidget[1]->setValue(value.ConvertTo<xiiVec4>().y);
        m_pWidget[2]->setValue(value.ConvertTo<xiiVec4>().z);
        m_pWidget[3]->setValue(value.ConvertTo<xiiVec4>().w);
        break;
    }
  }
  else
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValueInvalid();
        break;
      case 2:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        break;
      case 3:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        break;
      case 4:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        m_pWidget[3]->setValueInvalid();
        break;
    }
  }
}

void xiiQtPropertyEditorFloatSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorFloatSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged(xiiVariant(m_pWidget[0]->value()).ConvertTo(m_OriginalType));
      break;
    case 2:
      BroadcastValueChanged(xiiVec2(m_pWidget[0]->value(), m_pWidget[1]->value()));
      break;
    case 3:
      BroadcastValueChanged(xiiVec3(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
      break;
    case 4:
      BroadcastValueChanged(xiiVec4(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
      break;
  }
}

/// *** DOUBLE SPINBOX ***

xiiQtPropertyEditorDoubleSpinboxWidget::xiiQtPropertyEditorDoubleSpinboxWidget(xiiInt8 iNumComponents) :
  xiiQtStandardPropertyWidget()
{
  XII_ASSERT_DEBUG(iNumComponents <= 4, "Only up to 4 components are supported");

  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  xiiStringView         sLabels[]     = {"X", "Y", "Z", "W"};
  const xiiColorGammaUB labelColors[] = {xiiColorScheme::LightUI(xiiColorScheme::Red), xiiColorScheme::LightUI(xiiColorScheme::Green), xiiColorScheme::LightUI(xiiColorScheme::Blue), xiiColorScheme::LightUI(xiiColorScheme::Gray)};

  for (xiiInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    if (m_iNumComponents > 1)
    {
      QLabel*  pLabel  = new QLabel(xiiMakeQString(sLabels[c]));
      QPalette palette = pLabel->palette();
      palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
      pLabel->setPalette(palette);
      m_pLayout->addWidget(pLabel);
    }

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorDoubleSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<xiiNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction    = (pNoTemporaryTransactions == nullptr);

  if (const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());
        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec2d>())
        {
          xiiVec2d value = pClamp->GetMinValue().ConvertTo<xiiVec2d>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec2d>())
        {
          xiiVec2d value = pClamp->GetMaxValue().ConvertTo<xiiVec2d>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec3d>())
        {
          xiiVec3d value = pClamp->GetMinValue().ConvertTo<xiiVec3d>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec3d>())
        {
          xiiVec3d value = pClamp->GetMaxValue().ConvertTo<xiiVec3d>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec4d>())
        {
          xiiVec4d value = pClamp->GetMinValue().ConvertTo<xiiVec4d>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec4d>())
        {
          xiiVec4d value = pClamp->GetMaxValue().ConvertTo<xiiVec4d>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0]);

        if (pDefault->GetValue().CanConvertTo<double>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<double>());
        }
        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<xiiVec2d>())
        {
          xiiVec2d value = pDefault->GetValue().ConvertTo<xiiVec2d>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<xiiVec3d>())
        {
          xiiVec3d value = pDefault->GetValue().ConvertTo<xiiVec3d>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<xiiVec4d>())
        {
          xiiVec4d value = pDefault->GetValue().ConvertTo<xiiVec4d>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(xiiMakeQString(pMinValueText->GetText()));
    }
  }
}

void xiiQtPropertyEditorDoubleSpinboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();
  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = xiiVariantType::Double;
  }

  if (value.IsValid())
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValue(value.ConvertTo<double>());
        break;
      case 2:
        m_pWidget[0]->setValue(value.ConvertTo<xiiVec2>().x);
        m_pWidget[1]->setValue(value.ConvertTo<xiiVec2>().y);
        break;
      case 3:
        m_pWidget[0]->setValue(value.ConvertTo<xiiVec3>().x);
        m_pWidget[1]->setValue(value.ConvertTo<xiiVec3>().y);
        m_pWidget[2]->setValue(value.ConvertTo<xiiVec3>().z);
        break;
      case 4:
        m_pWidget[0]->setValue(value.ConvertTo<xiiVec4>().x);
        m_pWidget[1]->setValue(value.ConvertTo<xiiVec4>().y);
        m_pWidget[2]->setValue(value.ConvertTo<xiiVec4>().z);
        m_pWidget[3]->setValue(value.ConvertTo<xiiVec4>().w);
        break;
    }
  }
  else
  {
    switch (m_iNumComponents)
    {
      case 1:
        m_pWidget[0]->setValueInvalid();
        break;
      case 2:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        break;
      case 3:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        break;
      case 4:
        m_pWidget[0]->setValueInvalid();
        m_pWidget[1]->setValueInvalid();
        m_pWidget[2]->setValueInvalid();
        m_pWidget[3]->setValueInvalid();
        break;
    }
  }
}

void xiiQtPropertyEditorDoubleSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bUseTemporaryTransaction && m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged(xiiVariant(m_pWidget[0]->value()).ConvertTo(m_OriginalType));
      break;
    case 2:
      BroadcastValueChanged(xiiVec2(m_pWidget[0]->value(), m_pWidget[1]->value()));
      break;
    case 3:
      BroadcastValueChanged(xiiVec3(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
      break;
    case 4:
      BroadcastValueChanged(xiiVec4(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
      break;
  }
}

/// *** TIME SPINBOX ***

xiiQtPropertyEditorTimeWidget::xiiQtPropertyEditorTimeWidget() :
  xiiQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new xiiQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(" sec");
    m_pWidget->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget->setMaximum(xiiMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorTimeWidget::OnInit()
{
  const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>();
  if (pClamp)
  {
    xiiQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>();
  if (pDefault)
  {
    xiiQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }
}

void xiiQtPropertyEditorTimeWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void xiiQtPropertyEditorTimeWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorTimeWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(xiiTime::MakeFromSeconds(m_pWidget->value()));
}


/// *** FLOAT ANGLE SPINBOX ***

xiiQtPropertyEditorFloatAngleWidget::xiiQtPropertyEditorFloatAngleWidget() :
  xiiQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new xiiQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(xiiStringUtf8(L"\u00B0").GetData());
    m_pWidget->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget->setMaximum(xiiMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(1);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorFloatAngleWidget::OnInit()
{
  const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>();
  if (pClamp)
  {
    xiiQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>();
  if (pDefault)
  {
    xiiQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }

  const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>();
  if (pSuffix)
  {
    m_pWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }

  const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>();
  if (pMinValueText)
  {
    m_pWidget->setSpecialValueText(xiiMakeQString(pMinValueText->GetText()));
  }
}

void xiiQtPropertyEditorFloatAngleWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void xiiQtPropertyEditorFloatAngleWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorFloatAngleWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(xiiAngle::MakeFromDegree(m_pWidget->value()));
}

/// *** DOUBLE ANGLE SPINBOX ***

xiiQtPropertyEditorDoubleAngleWidget::xiiQtPropertyEditorDoubleAngleWidget() :
  xiiQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  {
    m_pWidget = new xiiQtDoubleSpinBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setDisplaySuffix(xiiStringUtf8(L"\u00B0").GetData());
    m_pWidget->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget->setMaximum(xiiMath::Infinity<double>());
    m_pWidget->setSingleStep(0.1f);
    m_pWidget->setAccelerated(true);
    m_pWidget->setDecimals(1);

    policy.setHorizontalStretch(2);
    m_pWidget->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget, SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorDoubleAngleWidget::OnInit()
{
  const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>();
  if (pClamp)
  {
    xiiQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setMinimum(pClamp->GetMinValue());
    m_pWidget->setMaximum(pClamp->GetMaxValue());
  }

  const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>();
  if (pDefault)
  {
    xiiQtScopedBlockSignals bs(m_pWidget);
    m_pWidget->setDefaultValue(pDefault->GetValue());
  }

  const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>();
  if (pSuffix)
  {
    m_pWidget->setDisplaySuffix(pSuffix->GetSuffix());
  }

  const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>();
  if (pMinValueText)
  {
    m_pWidget->setSpecialValueText(xiiMakeQString(pMinValueText->GetText()));
  }
}

void xiiQtPropertyEditorDoubleAngleWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void xiiQtPropertyEditorDoubleAngleWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorDoubleAngleWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(xiiAngled::MakeFromDegree(m_pWidget->value()));
}

/// *** INT SPINBOX ***

xiiQtPropertyEditorIntSpinboxWidget::xiiQtPropertyEditorIntSpinboxWidget(xiiInt8 iNumComponents, xiiInt32 iMinValue, xiiInt32 iMaxValue) :
  xiiQtStandardPropertyWidget()
{
  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalStretch(2);

  for (xiiInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this, true);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(iMinValue);
    m_pWidget[c]->setMaximum(iMaxValue);
    m_pWidget[c]->setSingleStep(1);
    m_pWidget[c]->setAccelerated(true);

    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

xiiQtPropertyEditorIntSpinboxWidget::~xiiQtPropertyEditorIntSpinboxWidget() = default;

void xiiQtPropertyEditorIntSpinboxWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    if (m_pWidget[i])
      m_pWidget[i]->setReadOnly(bReadOnly);
  }

  if (m_pSlider)
  {
    m_pSlider->setDisabled(bReadOnly);
  }
}

void xiiQtPropertyEditorIntSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<xiiNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction    = (pNoTemporaryTransactions == nullptr);

  if (const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        const xiiInt32 iMinValue = pClamp->GetMinValue().ConvertTo<xiiInt32>();
        const xiiInt32 iMaxValue = pClamp->GetMaxValue().ConvertTo<xiiInt32>();

        xiiQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());

        if (pClamp->GetMinValue().IsValid() && pClamp->GetMaxValue().IsValid() && (iMaxValue - iMinValue) < 256 && m_bUseTemporaryTransaction)
        {
          xiiQtScopedBlockSignals bs2(m_pSlider);

          // we have to create the slider here, because in the constructor we don't know the real
          // min and max values from the xiiClampValueAttribute (only the rough type ranges)
          m_pSlider = new QSlider(this);
          m_pSlider->installEventFilter(this);
          m_pSlider->setOrientation(Qt::Orientation::Horizontal);
          m_pSlider->setMinimum(iMinValue);
          m_pSlider->setMaximum(iMaxValue);

          m_pLayout->insertWidget(0, m_pSlider, 5); // make it take up most of the space

          connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
        }

        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec2I32>())
        {
          xiiVec2I32 value = pClamp->GetMinValue().ConvertTo<xiiVec2I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec2I32>())
        {
          xiiVec2I32 value = pClamp->GetMaxValue().ConvertTo<xiiVec2I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec3I32>())
        {
          xiiVec3I32 value = pClamp->GetMinValue().ConvertTo<xiiVec3I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec3I32>())
        {
          xiiVec3I32 value = pClamp->GetMaxValue().ConvertTo<xiiVec3I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec4I32>())
        {
          xiiVec4I32 value = pClamp->GetMinValue().ConvertTo<xiiVec4I32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec4I32>())
        {
          xiiVec4I32 value = pClamp->GetMaxValue().ConvertTo<xiiVec4I32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pSlider);

        if (pDefault->GetValue().CanConvertTo<xiiInt32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiInt32>());

          if (m_pSlider)
          {
            m_pSlider->setValue(pDefault->GetValue().ConvertTo<xiiInt32>());
          }
        }
        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<xiiVec2I32>())
        {
          xiiVec2I32 value = pDefault->GetValue().ConvertTo<xiiVec2I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<xiiVec3I32>())
        {
          xiiVec3I32 value = pDefault->GetValue().ConvertTo<xiiVec3I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<xiiVec4I32>())
        {
          xiiVec4I32 value = pDefault->GetValue().ConvertTo<xiiVec4I32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(xiiMakeQString(pMinValueText->GetText()));
    }
  }
}

void xiiQtPropertyEditorIntSpinboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3], m_pSlider);

  auto           prop = GetProperty();
  const xiiRTTI* type = prop->GetSpecificType();
  m_OriginalType      = type->GetVariantType();
  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = xiiVariantType::Int32;
  }

  switch (m_iNumComponents)
  {
    case 1:
      m_pWidget[0]->setValue(value.ConvertTo<xiiInt32>());

      if (m_pSlider)
      {
        m_pSlider->setValue(value.ConvertTo<xiiInt32>());
      }

      break;
    case 2:
      m_pWidget[0]->setValue(value.ConvertTo<xiiVec2I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<xiiVec2I32>().y);
      break;
    case 3:
      m_pWidget[0]->setValue(value.ConvertTo<xiiVec3I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<xiiVec3I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<xiiVec3I32>().z);
      break;
    case 4:
      m_pWidget[0]->setValue(value.ConvertTo<xiiVec4I32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<xiiVec4I32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<xiiVec4I32>().z);
      m_pWidget[3]->setValue(value.ConvertTo<xiiVec4I32>().w);
      break;
  }
}

void xiiQtPropertyEditorIntSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  xiiVariant newValue;
  switch (m_iNumComponents)
  {
    case 1:
      newValue = m_pWidget[0]->value();

      if (m_pSlider)
      {
        xiiQtScopedBlockSignals b0(m_pSlider);
        m_pSlider->setValue((xiiInt32)m_pWidget[0]->value());
      }

      break;
    case 2:
      newValue = xiiVec2I32(m_pWidget[0]->value(), m_pWidget[1]->value());
      break;
    case 3:
      newValue = xiiVec3I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value());
      break;
    case 4:
      newValue = xiiVec4I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value());
      break;
  }

  BroadcastValueChanged(newValue.ConvertTo(m_OriginalType));
}

void xiiQtPropertyEditorIntSpinboxWidget::onBeginTemporary()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void xiiQtPropertyEditorIntSpinboxWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  {
    xiiQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged(xiiVariant(m_pSlider->value()).ConvertTo(m_OriginalType));
}

void xiiQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

/// *** UINT SPINBOX ***

xiiQtPropertyEditorUIntSpinboxWidget::xiiQtPropertyEditorUIntSpinboxWidget(xiiInt8 iNumComponents, xiiUInt32 uiMinValue, xiiUInt32 uiMaxValue) :
  xiiQtStandardPropertyWidget()
{
  m_iNumComponents = iNumComponents;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalStretch(2);

  for (xiiInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this, true);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(uiMinValue);
    m_pWidget[c]->setMaximum(uiMaxValue);
    m_pWidget[c]->setSingleStep(1);
    m_pWidget[c]->setAccelerated(true);

    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

xiiQtPropertyEditorUIntSpinboxWidget::~xiiQtPropertyEditorUIntSpinboxWidget() = default;

void xiiQtPropertyEditorUIntSpinboxWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  for (xiiUInt32 i = 0; i < 4; ++i)
  {
    if (m_pWidget[i])
      m_pWidget[i]->setReadOnly(bReadOnly);
  }

  if (m_pSlider)
  {
    m_pSlider->setDisabled(bReadOnly);
  }
}

void xiiQtPropertyEditorUIntSpinboxWidget::OnInit()
{
  auto pNoTemporaryTransactions = m_pProp->GetAttributeByType<xiiNoTemporaryTransactionsAttribute>();
  m_bUseTemporaryTransaction    = (pNoTemporaryTransactions == nullptr);

  if (const xiiClampValueAttribute* pClamp = m_pProp->GetAttributeByType<xiiClampValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        const xiiUInt32 uiMinValue = pClamp->GetMinValue().ConvertTo<xiiUInt32>();
        const xiiUInt32 uiMaxValue = pClamp->GetMaxValue().ConvertTo<xiiUInt32>();

        xiiQtScopedBlockSignals bs(m_pWidget[0]);
        m_pWidget[0]->setMinimum(pClamp->GetMinValue());
        m_pWidget[0]->setMaximum(pClamp->GetMaxValue());

        if (pClamp->GetMinValue().IsValid() && pClamp->GetMaxValue().IsValid() && (uiMaxValue - uiMinValue) < 256 && m_bUseTemporaryTransaction)
        {
          xiiQtScopedBlockSignals bs2(m_pSlider);

          // we have to create the slider here, because in the constructor we don't know the real
          // min and max values from the xiiClampValueAttribute (only the rough type ranges)
          m_pSlider = new QSlider(this);
          m_pSlider->installEventFilter(this);
          m_pSlider->setOrientation(Qt::Orientation::Horizontal);
          m_pSlider->setMinimum(uiMinValue);
          m_pSlider->setMaximum(uiMaxValue);

          m_pLayout->insertWidget(0, m_pSlider, 5); // make it take up most of the space

          connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
        }

        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec2U32>())
        {
          xiiVec2U32 value = pClamp->GetMinValue().ConvertTo<xiiVec2U32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec2U32>())
        {
          xiiVec2U32 value = pClamp->GetMaxValue().ConvertTo<xiiVec2U32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec3U32>())
        {
          xiiVec3U32 value = pClamp->GetMinValue().ConvertTo<xiiVec3U32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec3U32>())
        {
          xiiVec3U32 value = pClamp->GetMaxValue().ConvertTo<xiiVec3U32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pClamp->GetMinValue().CanConvertTo<xiiVec4U32>())
        {
          xiiVec4U32 value = pClamp->GetMinValue().ConvertTo<xiiVec4U32>();
          m_pWidget[0]->setMinimum(value.x);
          m_pWidget[1]->setMinimum(value.y);
          m_pWidget[2]->setMinimum(value.z);
          m_pWidget[3]->setMinimum(value.w);
        }
        if (pClamp->GetMaxValue().CanConvertTo<xiiVec4U32>())
        {
          xiiVec4U32 value = pClamp->GetMaxValue().ConvertTo<xiiVec4U32>();
          m_pWidget[0]->setMaximum(value.x);
          m_pWidget[1]->setMaximum(value.y);
          m_pWidget[2]->setMaximum(value.z);
          m_pWidget[3]->setMaximum(value.w);
        }
        break;
      }
    }
  }

  if (const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    switch (m_iNumComponents)
    {
      case 1:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pSlider);

        if (pDefault->GetValue().CanConvertTo<xiiInt32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiInt32>());

          if (m_pSlider)
          {
            m_pSlider->setValue(pDefault->GetValue().ConvertTo<xiiInt32>());
          }
        }
        break;
      }
      case 2:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1]);

        if (pDefault->GetValue().CanConvertTo<xiiVec2U32>())
        {
          xiiVec2U32 value = pDefault->GetValue().ConvertTo<xiiVec2U32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<xiiVec3U32>())
        {
          xiiVec3U32 value = pDefault->GetValue().ConvertTo<xiiVec3U32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<xiiVec4U32>())
        {
          xiiVec4U32 value = pDefault->GetValue().ConvertTo<xiiVec4U32>();
          m_pWidget[0]->setDefaultValue(value.x);
          m_pWidget[1]->setDefaultValue(value.y);
          m_pWidget[2]->setDefaultValue(value.z);
          m_pWidget[3]->setDefaultValue(value.w);
        }
        break;
      }
    }
  }

  if (const xiiSuffixAttribute* pSuffix = m_pProp->GetAttributeByType<xiiSuffixAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  if (const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>())
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(xiiMakeQString(pMinValueText->GetText()));
    }
  }
}

void xiiQtPropertyEditorUIntSpinboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3], m_pSlider);

  auto           prop = GetProperty();
  const xiiRTTI* type = prop->GetSpecificType();
  m_OriginalType      = type->GetVariantType();
  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = xiiVariantType::UInt32;
  }

  switch (m_iNumComponents)
  {
    case 1:
      m_pWidget[0]->setValue(value.ConvertTo<xiiUInt32>());

      if (m_pSlider)
      {
        m_pSlider->setValue(value.ConvertTo<xiiUInt32>());
      }

      break;
    case 2:
      m_pWidget[0]->setValue(value.ConvertTo<xiiVec2U32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<xiiVec2U32>().y);
      break;
    case 3:
      m_pWidget[0]->setValue(value.ConvertTo<xiiVec3U32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<xiiVec3U32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<xiiVec3U32>().z);
      break;
    case 4:
      m_pWidget[0]->setValue(value.ConvertTo<xiiVec4U32>().x);
      m_pWidget[1]->setValue(value.ConvertTo<xiiVec4U32>().y);
      m_pWidget[2]->setValue(value.ConvertTo<xiiVec4U32>().z);
      m_pWidget[3]->setValue(value.ConvertTo<xiiVec4U32>().w);
      break;
  }
}

void xiiQtPropertyEditorUIntSpinboxWidget::SlotValueChanged()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  xiiVariant newValue;
  switch (m_iNumComponents)
  {
    case 1:
      newValue = m_pWidget[0]->value();

      if (m_pSlider)
      {
        xiiQtScopedBlockSignals b0(m_pSlider);
        m_pSlider->setValue((xiiUInt32)m_pWidget[0]->value());
      }

      break;
    case 2:
      newValue = xiiVec2U32(m_pWidget[0]->value(), m_pWidget[1]->value());
      break;
    case 3:
      newValue = xiiVec3U32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value());
      break;
    case 4:
      newValue = xiiVec4U32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value());
      break;
  }

  BroadcastValueChanged(newValue.ConvertTo(m_OriginalType));
}

void xiiQtPropertyEditorUIntSpinboxWidget::onBeginTemporary()
{
  if (m_bUseTemporaryTransaction && !m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void xiiQtPropertyEditorUIntSpinboxWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorUIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  {
    xiiQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged(xiiVariant(m_pSlider->value()).ConvertTo(m_OriginalType));
}

void xiiQtPropertyEditorUIntSpinboxWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

xiiMap<xiiString, xiiQtImageSliderWidget::ImageGeneratorFunc> xiiQtImageSliderWidget::s_ImageGenerators;

xiiQtImageSliderWidget::xiiQtImageSliderWidget(ImageGeneratorFunc generator, double fMinValue, double fMaxValue, QWidget* pParent) :
  QWidget(pParent)
{
  m_Generator = generator;
  m_fMinValue = fMinValue;
  m_fMaxValue = fMaxValue;

  setAutoFillBackground(false);
}

void xiiQtImageSliderWidget::SetValue(double fValue)
{
  if (m_fValue == fValue)
    return;

  m_fValue = fValue;
  update();
}

void xiiQtImageSliderWidget::paintEvent(QPaintEvent* event)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);

  const QRect area = rect();

  if (area.width() != m_Image.width())
    UpdateImage();

  painter.drawTiledPixmap(area, QPixmap::fromImage(m_Image));

  const float factor = xiiMath::Unlerp(m_fMinValue, m_fMaxValue, m_fValue);

  const double pos = (int)(factor * area.width()) + 0.5f;

  const double top = area.top() + 0.5;
  const double bot = area.bottom() + 0.5;
  const double len = 5.0;
  const double wid = 2.0;

  const QColor col = qRgb(80, 80, 80);

  painter.setPen(col);
  painter.setBrush(col);

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, top));
    path.lineTo(QPointF(pos, top + len));
    path.lineTo(QPointF(pos + wid, top));
    path.closeSubpath();

    painter.drawPath(path);
  }

  {
    QPainterPath path;
    path.moveTo(QPointF(pos - wid, bot));
    path.lineTo(QPointF(pos, bot - len));
    path.lineTo(QPointF(pos + wid, bot));
    path.closeSubpath();

    painter.drawPath(path);
  }
}

void xiiQtImageSliderWidget::UpdateImage()
{
  const int width = rect().width();

  if (m_Generator)
  {
    m_Image = m_Generator(rect().width(), rect().height(), m_fMinValue, m_fMaxValue);
  }
  else
  {
    m_Image = QImage(width, 1, QImage::Format::Format_RGB32);

    xiiColorGammaUB cg = xiiColor::HotPink;
    for (int x = 0; x < width; ++x)
    {
      m_Image.setPixel(x, 0, qRgb(cg.r, cg.g, cg.b));
    }
  }
}

void xiiQtImageSliderWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons().testFlag(Qt::LeftButton))
  {
    const int width  = rect().width();
    const int height = rect().height();

    QPoint    coord = event->pos();
    const int x     = xiiMath::Clamp(coord.x(), 0, width - 1);

    const double fx  = (double)x / (width - 1);
    const double val = xiiMath::Lerp(m_fMinValue, m_fMaxValue, fx);

    valueChanged(val);
  }

  event->accept();
}

void xiiQtImageSliderWidget::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    Q_EMIT sliderPressed();
  }

  mouseMoveEvent(event);

  event->accept();
}

void xiiQtImageSliderWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    Q_EMIT sliderReleased();
  }
  event->accept();
}

/// *** SLIDER ***

xiiQtPropertyEditorSliderWidget::xiiQtPropertyEditorSliderWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
}

xiiQtPropertyEditorSliderWidget::~xiiQtPropertyEditorSliderWidget() = default;

void xiiQtPropertyEditorSliderWidget::OnInit()
{
  const xiiImageSliderUiAttribute* pSliderAttr = m_pProp->GetAttributeByType<xiiImageSliderUiAttribute>();
  const xiiClampValueAttribute*    pRange      = m_pProp->GetAttributeByType<xiiClampValueAttribute>();
  XII_ASSERT_DEV(pRange != nullptr, "xiiImageSliderUiAttribute always has to be compined with xiiClampValueAttribute to specify the valid range.");
  XII_ASSERT_DEV(pRange->GetMinValue().IsValid() && pRange->GetMaxValue().IsValid(), "The min and max values used with xiiImageSliderUiAttribute both have to be valid.");

  m_fMinValue = pRange->GetMinValue().ConvertTo<double>();
  m_fMaxValue = pRange->GetMaxValue().ConvertTo<double>();

  m_pSlider = new xiiQtImageSliderWidget(xiiQtImageSliderWidget::s_ImageGenerators[pSliderAttr->m_sImageGenerator], m_fMinValue, m_fMaxValue, this);

  m_pLayout->insertWidget(0, m_pSlider);
  connect(m_pSlider, SIGNAL(sliderPressed()), this, SLOT(onBeginTemporary()));
  connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(onEndTemporary()));
  connect(m_pSlider, SIGNAL(valueChanged(double)), this, SLOT(SlotSliderValueChanged(double)));

  if (const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    xiiQtScopedBlockSignals bs(m_pSlider);

    if (pDefault->GetValue().CanConvertTo<double>())
    {
      m_pSlider->SetValue(pDefault->GetValue().ConvertTo<double>());
    }
  }
}

void xiiQtPropertyEditorSliderWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pSlider);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = xiiVariantType::Double;
  }

  m_pSlider->SetValue(value.ConvertTo<double>());
}

void xiiQtPropertyEditorSliderWidget::SlotSliderValueChanged(double fValue)
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(xiiVariant(fValue).ConvertTo(m_OriginalType));

  m_pSlider->SetValue(fValue);
}

void xiiQtPropertyEditorSliderWidget::on_EditingFinished_triggered()
{
  onEndTemporary();
}

void xiiQtPropertyEditorSliderWidget::onBeginTemporary()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;
}

void xiiQtPropertyEditorSliderWidget::onEndTemporary()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

/// *** FLOAT QUATERNION ***

xiiQtPropertyEditorFloatQuaternionWidget::xiiQtPropertyEditorFloatQuaternionWidget() :
  xiiQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  xiiStringView         sLabels[]     = {"R", "P", "Y"};
  xiiStringView         sTooltip[]    = {"Roll (Rotation around the forward axis)", "Pitch (Rotation around the side axis)", "Yaw (Rotation around the up axis)"};
  const xiiColorGammaUB labelColors[] = {xiiColorScheme::LightUI(xiiColorScheme::Red), xiiColorScheme::LightUI(xiiColorScheme::Green), xiiColorScheme::LightUI(xiiColorScheme::Blue)};

  for (xiiInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    QLabel*  pLabel  = new QLabel(xiiMakeQString(sLabels[c]));
    QPalette palette = pLabel->palette();
    palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
    pLabel->setPalette(palette);
    pLabel->setToolTip(xiiMakeQString(sTooltip[c]));

    m_pLayout->addWidget(pLabel);
    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorFloatQuaternionWidget::OnInit() {}

void xiiQtPropertyEditorFloatQuaternionWidget::InternalSetValue(const xiiVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  xiiQtScopedBlockSignals b0(m_pWidget[0]);
  xiiQtScopedBlockSignals b1(m_pWidget[1]);
  xiiQtScopedBlockSignals b2(m_pWidget[2]);

  if (value.IsValid())
  {
    const xiiQuat qRot = value.ConvertTo<xiiQuat>();
    xiiAngle      x, y, z;
    qRot.GetAsEulerAngles(x, y, z);

    m_pWidget[0]->setValue(x.GetDegree());
    m_pWidget[1]->setValue(y.GetDegree());
    m_pWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pWidget[0]->setValueInvalid();
    m_pWidget[1]->setValueInvalid();
    m_pWidget[2]->setValueInvalid();
  }
}

void xiiQtPropertyEditorFloatQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorFloatQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  xiiAngle x = xiiAngle::MakeFromDegree(m_pWidget[0]->value());
  xiiAngle y = xiiAngle::MakeFromDegree(m_pWidget[1]->value());
  xiiAngle z = xiiAngle::MakeFromDegree(m_pWidget[2]->value());

  xiiQuat qRot = xiiQuat::MakeFromEulerAngles(x, y, z);

  BroadcastValueChanged(qRot);
}

/// *** DOUBLE QUATERNION ***

xiiQtPropertyEditorDoubleQuaternionWidget::xiiQtPropertyEditorDoubleQuaternionWidget() :
  xiiQtStandardPropertyWidget()
{
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  xiiStringView         sLabels[]     = {"R", "P", "Y"};
  xiiStringView         sTooltip[]    = {"Roll (Rotation around the forward axis)", "Pitch (Rotation around the side axis)", "Yaw (Rotation around the up axis)"};
  const xiiColorGammaUB labelColors[] = {xiiColorScheme::LightUI(xiiColorScheme::Red), xiiColorScheme::LightUI(xiiColorScheme::Green), xiiColorScheme::LightUI(xiiColorScheme::Blue)};

  for (xiiInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pWidget[c]->installEventFilter(this);
    m_pWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    QLabel*  pLabel  = new QLabel(xiiMakeQString(sLabels[c]));
    QPalette palette = pLabel->palette();
    palette.setColor(pLabel->foregroundRole(), QColor(labelColors[c].r, labelColors[c].g, labelColors[c].b));
    pLabel->setPalette(palette);
    pLabel->setToolTip(xiiMakeQString(sTooltip[c]));

    m_pLayout->addWidget(pLabel);
    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorDoubleQuaternionWidget::OnInit() {}

void xiiQtPropertyEditorDoubleQuaternionWidget::InternalSetValue(const xiiVariant& value)
{
  if (m_bTemporaryCommand)
    return;

  xiiQtScopedBlockSignals b0(m_pWidget[0]);
  xiiQtScopedBlockSignals b1(m_pWidget[1]);
  xiiQtScopedBlockSignals b2(m_pWidget[2]);

  if (value.IsValid())
  {
    const xiiQuatd qRot = value.ConvertTo<xiiQuatd>();
    xiiAngled      x, y, z;
    qRot.GetAsEulerAngles(x, y, z);

    m_pWidget[0]->setValue(x.GetDegree());
    m_pWidget[1]->setValue(y.GetDegree());
    m_pWidget[2]->setValue(z.GetDegree());
  }
  else
  {
    m_pWidget[0]->setValueInvalid();
    m_pWidget[1]->setValueInvalid();
    m_pWidget[2]->setValueInvalid();
  }
}

void xiiQtPropertyEditorDoubleQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorDoubleQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  xiiAngled x = xiiAngled::MakeFromDegree(m_pWidget[0]->value());
  xiiAngled y = xiiAngled::MakeFromDegree(m_pWidget[1]->value());
  xiiAngled z = xiiAngled::MakeFromDegree(m_pWidget[2]->value());

  xiiQuatd qRot = xiiQuatd::MakeFromEulerAngles(x, y, z);

  BroadcastValueChanged(qRot);
}

/// *** LINEEDIT ***

xiiQtPropertyEditorLineEditWidget::xiiQtPropertyEditorLineEditWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
}

void xiiQtPropertyEditorLineEditWidget::SetReadOnly(bool bReadOnly /*= true*/)
{
  m_pWidget->setReadOnly(bReadOnly);
}

void xiiQtPropertyEditorLineEditWidget::OnInit()
{
  if (m_pProp->GetAttributeByType<xiiReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
  {
    setEnabled(true);

    xiiQtScopedBlockSignals bs(m_pWidget);

    m_pWidget->setReadOnly(true);
    QPalette palette = m_pWidget->palette();
    palette.setColor(QPalette::Base, QColor(0, 0, 0, 0));
    m_pWidget->setPalette(palette);
  }
}

void xiiQtPropertyEditorLineEditWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  m_OriginalType = GetProperty()->GetSpecificType()->GetVariantType();

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = value.GetType();
  }

  if (m_OriginalType == xiiVariantType::Invalid)
  {
    m_OriginalType = xiiVariantType::String;
  }

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(xiiMakeQString(value.ConvertTo<xiiString>()));
  }
}

void xiiQtPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  xiiVariant v(value.toUtf8().data());
  BroadcastValueChanged(m_OriginalType != xiiVariantType::StringView ? v.ConvertTo(m_OriginalType) : v);
}

void xiiQtPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  xiiVariant v(m_pWidget->text().toUtf8().data());
  BroadcastValueChanged(m_OriginalType != xiiVariantType::StringView ? v.ConvertTo(m_OriginalType) : v);
}


/// *** COLOR ***

xiiQtColorButtonWidget::xiiQtColorButtonWidget(QWidget* pParent) :
  QFrame(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
}

void xiiQtColorButtonWidget::SetColor(const xiiVariant& color)
{
  if (color.IsValid())
  {
    xiiColor col0 = color.ConvertTo<xiiColor>();
    col0.NormalizeToLdrRange();

    const xiiColorGammaUB col = col0;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::SolidPattern));
    setPalette(m_Pal);
  }
  else
  {
    const xiiColorGammaUB col = xiiColor::LightGrey;

    QColor qol;
    qol.setRgb(col.r, col.g, col.b, col.a);

    m_Pal.setBrush(QPalette::Window, QBrush(qol, Qt::DiagCrossPattern));
    setPalette(m_Pal);
  }
}

void xiiQtColorButtonWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  QFrame::showEvent(event);
}

void xiiQtColorButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

QSize xiiQtColorButtonWidget::sizeHint() const
{
  return minimumSizeHint();
}

QSize xiiQtColorButtonWidget::minimumSizeHint() const
{
  QFontMetrics fm(font());

  QStyleOptionFrame opt;
  initStyleOption(&opt);
  return style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(20, fm.height()), this);
}

xiiQtPropertyEditorColorWidget::xiiQtPropertyEditorColorWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new xiiQtColorButtonWidget(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void xiiQtPropertyEditorColorWidget::OnInit()
{
  m_bExposeAlpha = (m_pProp->GetAttributeByType<xiiExposeColorAlphaAttribute>() != nullptr);
  m_bExposeAlpha |= (m_pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>());
}

void xiiQtPropertyEditorColorWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  m_OriginalValue = GetOldValue();
  m_pWidget->SetColor(value);

  m_bIsHDR = value.GetType() == xiiVariantType::Color;
}

void xiiQtPropertyEditorColorWidget::on_Button_triggered()
{
  Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  xiiColor temp = xiiColor::White;
  if (m_OriginalValue.IsValid())
  {
    temp = m_OriginalValue.ConvertTo<xiiColor>();
  }

  xiiQtUiServices::GetSingleton()->ShowColorDialog(temp, m_bExposeAlpha, m_bIsHDR, this, SLOT(on_CurrentColor_changed(const xiiColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void xiiQtPropertyEditorColorWidget::on_CurrentColor_changed(const xiiColor& color)
{
  xiiVariant col;

  if (!m_bIsHDR)
  {
    // xiiVariant does not down-cast to xiiColorGammaUB automatically
    col = xiiColorGammaUB(color);
  }
  else
  {
    col = color;
  }

  m_pWidget->SetColor(col);
  BroadcastValueChanged(col);
}

void xiiQtPropertyEditorColorWidget::on_Color_reset()
{
  m_pWidget->SetColor(m_OriginalValue);
  Broadcast(xiiPropertyEvent::Type::CancelTemporary);
}

void xiiQtPropertyEditorColorWidget::on_Color_accepted()
{
  m_OriginalValue = GetOldValue();
  Broadcast(xiiPropertyEvent::Type::EndTemporary);
}


/// *** ENUM COMBOBOX ***

xiiQtPropertyEditorEnumWidget::xiiQtPropertyEditorEnumWidget() :
  xiiQtStandardPropertyWidget()
{

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
}

void xiiQtPropertyEditorEnumWidget::OnInit()
{
  const xiiRTTI* pType = m_pProp->GetSpecificType();

  const xiiUInt32 uiCount = pType->GetProperties().GetCount();

  xiiHybridArray<const xiiAbstractProperty*, 16> props;

  // Start at 1 to skip default value.
  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != xiiPropertyCategory::Constant)
      continue;

    props.PushBack(pProp);
  }

  // this code path implements using multiple buttons in a row instead of a combobox, for small number of entries
  // it works for 2 elements, but often already looks bad with 3 elements
  // but even with 2 elements, it just adds visual clutter (unused values are now visible)
  // so I'm not going to enable it, but keep it in, in case we want to try it again in the future
  constexpr bool bUseButtons = false;

  if (bUseButtons && props.GetCount() <= XII_ARRAY_SIZE(m_pButtons))
  {
    for (xiiUInt32 i = 0; i < props.GetCount(); ++i)
    {
      auto pProp = props[i];

      const xiiAbstractConstantProperty* pConstant = static_cast<const xiiAbstractConstantProperty*>(pProp);

      m_pButtons[i] = new QPushButton(this);
      m_pButtons[i]->setText(xiiMakeQString(xiiTranslate(pConstant->GetPropertyName())));
      m_pButtons[i]->setCheckable(true);
      m_pButtons[i]->setProperty("value", pConstant->GetConstant().ConvertTo<xiiInt64>());

      connect(m_pButtons[i], SIGNAL(clicked(bool)), this, SLOT(on_ButtonClicked_changed(bool)));

      m_pLayout->addWidget(m_pButtons[i]);
    }
  }
  else
  {
    m_pWidget = new QComboBox(this);
    m_pWidget->installEventFilter(this);
    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pLayout->addWidget(m_pWidget);

    connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));

    xiiQtScopedBlockSignals bs(m_pWidget);

    for (xiiUInt32 i = 0; i < props.GetCount(); ++i)
    {
      auto pProp = props[i];

      const xiiAbstractConstantProperty* pConstant = static_cast<const xiiAbstractConstantProperty*>(pProp);

      m_pWidget->addItem(xiiMakeQString(xiiTranslate(pConstant->GetPropertyName())), pConstant->GetConstant().ConvertTo<xiiInt64>());
    }
  }
}

void xiiQtPropertyEditorEnumWidget::InternalSetValue(const xiiVariant& value)
{
  if (m_pWidget)
  {
    xiiInt32 iIndex = -1;
    if (value.IsValid())
    {
      iIndex = m_pWidget->findData(value.ConvertTo<xiiInt64>());
      XII_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    }

    xiiQtScopedBlockSignals b(m_pWidget);
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    const xiiInt64 iValue = value.ConvertTo<xiiInt64>();

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_pButtons); ++i)
    {
      if (m_pButtons[i])
      {
        const xiiInt64 iButtonValue = m_pButtons[i]->property("value").toLongLong();

        xiiQtScopedBlockSignals b(m_pButtons[i]);
        m_pButtons[i]->setChecked(iButtonValue == iValue);
      }
    }
  }
}

void xiiQtPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  const xiiInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}

void xiiQtPropertyEditorEnumWidget::on_ButtonClicked_changed(bool checked)
{
  if (QPushButton* pButton = qobject_cast<QPushButton*>(sender()))
  {
    const xiiInt64 iValue = pButton->property("value").toLongLong();
    BroadcastValueChanged(iValue);
  }
}

/// *** BITFLAGS COMBOBOX ***

xiiQtPropertyEditorBitflagsWidget::xiiQtPropertyEditorBitflagsWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

xiiQtPropertyEditorBitflagsWidget::~xiiQtPropertyEditorBitflagsWidget()
{
  m_pWidget->setMenu(nullptr);
  delete m_pMenu;
}

void xiiQtPropertyEditorBitflagsWidget::OnInit()
{
  const xiiRTTI* enumType = m_pProp->GetSpecificType();

  const xiiRTTI* pType   = enumType;
  xiiUInt32      uiCount = pType->GetProperties().GetCount();

  // Start at 1 to skip default value.
  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != xiiPropertyCategory::Constant)
      continue;

    const xiiAbstractConstantProperty* pConstant = static_cast<const xiiAbstractConstantProperty*>(pProp);

    QWidgetAction* pAction   = new QWidgetAction(m_pMenu);
    QCheckBox*     pCheckBox = new QCheckBox(xiiMakeQString(xiiTranslate(pConstant->GetPropertyName())), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<xiiInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
  }

  // sets all bits to clear or set
  {
    QWidgetAction* pAllAction = new QWidgetAction(m_pMenu);
    m_pAllButton              = new QPushButton(QString::fromUtf8("All"), m_pMenu);
    connect(m_pAllButton, &QPushButton::clicked, this, [this](bool bChecked) { SetAllChecked(true); });
    pAllAction->setDefaultWidget(m_pAllButton);
    m_pMenu->addAction(pAllAction);

    QWidgetAction* pClearAction = new QWidgetAction(m_pMenu);
    m_pClearButton              = new QPushButton(QString::fromUtf8("Clear"), m_pMenu);
    connect(m_pClearButton, &QPushButton::clicked, this, [this](bool bChecked) { SetAllChecked(false); });
    pClearAction->setDefaultWidget(m_pClearButton);
    m_pMenu->addAction(pClearAction);
  }
}

void xiiQtPropertyEditorBitflagsWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);
  m_iCurrentBitflags = value.ConvertTo<xiiInt64>();

  QString sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool    bChecked = (it.Key() & m_iCurrentBitflags) != 0;
    QString sName    = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
    }
    it.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);
}

void xiiQtPropertyEditorBitflagsWidget::SetAllChecked(bool bChecked)
{
  for (auto& pCheckBox : m_Constants)
  {
    pCheckBox.Value()->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
  }
}

void xiiQtPropertyEditorBitflagsWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void xiiQtPropertyEditorBitflagsWidget::on_Menu_aboutToHide()
{
  xiiInt64 iValue = 0;
  QString  sText;
  for (auto it = m_Constants.GetIterator(); it.IsValid(); ++it)
  {
    bool    bChecked = it.Value()->checkState() == Qt::Checked;
    QString sName    = it.Value()->text();
    if (bChecked)
    {
      sText += sName + "|";
      iValue |= it.Key();
    }
  }
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);

  m_pWidget->setText(sText);

  if (m_iCurrentBitflags != iValue)
  {
    m_iCurrentBitflags = iValue;
    BroadcastValueChanged(m_iCurrentBitflags);
  }
}


/// *** CURVE1D ***

xiiQtCurve1DButtonWidget::xiiQtCurve1DButtonWidget(QWidget* pParent) :
  QLabel(pParent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
  setScaledContents(true);
}

void xiiQtCurve1DButtonWidget::UpdatePreview(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange)
{
  xiiInt32 iNumPoints = 0;
  pObjectAccessor->GetCountByName(pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();

  xiiVariant                   v;
  xiiHybridArray<xiiVec2d, 32> points;
  points.Reserve(iNumPoints);

  double minX = fLowerExtents * 4800.0;
  double maxX = fUpperExtents * 4800.0;

  double minY = fLowerRange;
  double maxY = fUpperRange;

  for (xiiInt32 i = 0; i < iNumPoints; ++i)
  {
    const xiiDocumentObject* pPoint = pObjectAccessor->GetChildObjectByName(pCurveObject, "ControlPoints", i);

    xiiVec2d p;

    pObjectAccessor->GetValueByName(pPoint, "Tick", v).AssertSuccess();
    p.x = v.ConvertTo<double>();

    pObjectAccessor->GetValueByName(pPoint, "Value", v).AssertSuccess();
    p.y = v.ConvertTo<double>();

    points.PushBack(p);

    if (!bLowerFixed)
      minX = xiiMath::Min(minX, p.x);

    if (!bUpperFixed)
      maxX = xiiMath::Max(maxX, p.x);

    minY = xiiMath::Min(minY, p.y);
    maxY = xiiMath::Max(maxY, p.y);
  }

  const double pW = xiiMath::Max(10, size().width());
  const double pH = xiiMath::Clamp(size().height(), 5, 24);

  QPixmap pixmap((int)pW, (int)pH);
  pixmap.fill(palette().base().color());

  QPainter pt(&pixmap);
  pt.setPen(color);
  pt.setRenderHint(QPainter::RenderHint::Antialiasing);

  if (!points.IsEmpty())
  {
    points.Sort([](const xiiVec2d& lhs, const xiiVec2d& rhs) -> bool { return lhs.x < rhs.x; });

    const double normX = 1.0 / (maxX - minX);
    const double normY = 1.0 / (maxY - minY);

    QPainterPath path;

    {
      double startX = xiiMath::Min(minX, points[0].x);
      double startY = points[0].y;

      startX = (startX - minX) * normX;
      startY = 1.0 - ((startY - minY) * normY);

      path.moveTo((int)(startX * pW), (int)(startY * pH));
    }

    for (xiiUInt32 i = 0; i < points.GetCount(); ++i)
    {
      auto pt0 = points[i];
      pt0.x    = (pt0.x - minX) * normX;
      pt0.y    = 1.0 - ((pt0.y - minY) * normY);

      path.lineTo((int)(pt0.x * pW), (int)(pt0.y * pH));
    }

    {
      double endX = xiiMath::Max(maxX, points.PeekBack().x);
      double endY = points.PeekBack().y;

      endX = (endX - minX) * normX;
      endY = 1.0 - ((endY - minY) * normY);

      path.lineTo((int)(endX * pW), (int)(endY * pH));
    }

    pt.drawPath(path);
  }
  else
  {
    const double normY = 1.0 / (maxY - minY);
    double       valY  = 1.0 - ((fDefaultValue - minY) * normY);

    pt.drawLine(0, (int)(valY * pH), (int)pW, (int)(valY * pH));
  }

  setPixmap(pixmap);
}

void xiiQtCurve1DButtonWidget::mouseReleaseEvent(QMouseEvent* event)
{
  Q_EMIT clicked();
}

xiiQtPropertyEditorCurve1DWidget::xiiQtPropertyEditorCurve1DWidget() :
  xiiQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new xiiQtCurve1DButtonWidget(this);
  m_pButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pLayout->addWidget(m_pButton);

  XII_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_Button_triggered())) != nullptr, "signal/slot connection failed");
}

void xiiQtPropertyEditorCurve1DWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtPropertyWidget::SetSelection(items);

  UpdatePreview();
}

void xiiQtPropertyEditorCurve1DWidget::OnInit() {}
void xiiQtPropertyEditorCurve1DWidget::DoPrepareToDie() {}

void xiiQtPropertyEditorCurve1DWidget::UpdatePreview()
{
  if (m_Items.IsEmpty())
    return;

  const xiiDocumentObject*        pParent      = m_Items[0].m_pObject;
  const xiiDocumentObject*        pCurve       = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});
  const xiiColorAttribute*        pColorAttr   = m_pProp->GetAttributeByType<xiiColorAttribute>();
  const xiiCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<xiiCurveExtentsAttribute>();
  const xiiDefaultValueAttribute* pDefAttr     = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>();
  const xiiClampValueAttribute*   pClampAttr   = m_pProp->GetAttributeByType<xiiClampValueAttribute>();

  const bool            bLowerFixed = pExtentsAttr ? pExtentsAttr->m_bLowerExtentFixed : false;
  const bool            bUpperFixed = pExtentsAttr ? pExtentsAttr->m_bUpperExtentFixed : false;
  const double          fLowerExt   = pExtentsAttr ? pExtentsAttr->m_fLowerExtent : 0.0;
  const double          fUpperExt   = pExtentsAttr ? pExtentsAttr->m_fUpperExtent : 1.0;
  const xiiColorGammaUB color       = pColorAttr ? pColorAttr->GetColor() : xiiColor::GreenYellow;
  const double          fLowerRange = (pClampAttr && pClampAttr->GetMinValue().IsNumber()) ? pClampAttr->GetMinValue().ConvertTo<double>() : 0.0;
  const double          fUpperRange = (pClampAttr && pClampAttr->GetMaxValue().IsNumber()) ? pClampAttr->GetMaxValue().ConvertTo<double>() : 1.0;
  const double          fDefVal     = (pDefAttr && pDefAttr->GetValue().IsNumber()) ? pDefAttr->GetValue().ConvertTo<double>() : 0.0;

  m_pButton->UpdatePreview(m_pObjectAccessor, pCurve, QColor(color.r, color.g, color.b), fLowerExt, bLowerFixed, fUpperExt, bUpperFixed, fDefVal, fLowerRange, fUpperRange);
}

void xiiQtPropertyEditorCurve1DWidget::on_Button_triggered()
{
  const xiiDocumentObject*        pParent      = m_Items[0].m_pObject;
  const xiiDocumentObject*        pCurve       = m_pObjectAccessor->GetChildObjectByName(pParent, m_pProp->GetPropertyName(), {});
  const xiiColorAttribute*        pColorAttr   = m_pProp->GetAttributeByType<xiiColorAttribute>();
  const xiiCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<xiiCurveExtentsAttribute>();
  const xiiClampValueAttribute*   pClampAttr   = m_pProp->GetAttributeByType<xiiClampValueAttribute>();

  // TODO: would like to have one transaction open to finish/cancel at the end
  // but also be able to undo individual steps while editing
  // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->StartTransaction("Edit Curve");

  xiiQtCurveEditDlg* pDlg = new xiiQtCurveEditDlg(m_pObjectAccessor, pCurve, this);
  pDlg->restoreGeometry(xiiQtCurveEditDlg::GetLastDialogGeometry());

  if (pColorAttr)
  {
    pDlg->SetCurveColor(pColorAttr->GetColor());
  }

  if (pExtentsAttr)
  {
    pDlg->SetCurveExtents(pExtentsAttr->m_fLowerExtent, pExtentsAttr->m_bLowerExtentFixed, pExtentsAttr->m_fUpperExtent, pExtentsAttr->m_bUpperExtentFixed);
  }

  if (pClampAttr)
  {
    const double fLower = pClampAttr->GetMinValue().IsNumber() ? pClampAttr->GetMinValue().ConvertTo<double>() : -xiiMath::HighValue<double>();
    const double fUpper = pClampAttr->GetMaxValue().IsNumber() ? pClampAttr->GetMaxValue().ConvertTo<double>() : xiiMath::HighValue<double>();

    pDlg->SetCurveRanges(fLower, fUpper);
  }

  if (pDlg->exec() == QDialog::Accepted)
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->FinishTransaction();

    UpdatePreview();
  }
  else
  {
    // m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->CancelTransaction();
  }

  delete pDlg;
}
