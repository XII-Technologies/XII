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

void xiiQtPropertyEditorCheckboxWidget::mousePressEvent(QMouseEvent* ev)
{
  QWidget::mousePressEvent(ev);

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


/// *** DOUBLE SPINBOX ***

xiiQtPropertyEditorDoubleSpinboxWidget::xiiQtPropertyEditorDoubleSpinboxWidget(xiiInt8 iNumComponents) :
  xiiQtStandardPropertyWidget()
{
  XII_ASSERT_DEBUG(iNumComponents <= 4, "Only up to 4 components are supported");

  m_iNumComponents    = iNumComponents;
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;
  m_pWidget[3] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();

  for (xiiInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(0.1f);
    m_pWidget[c]->setAccelerated(true);

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorDoubleSpinboxWidget::OnInit()
{
  const xiiClampValueAttribute*   pClamp   = m_pProp->GetAttributeByType<xiiClampValueAttribute>();
  const xiiDefaultValueAttribute* pDefault = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>();
  const xiiSuffixAttribute*       pSuffix  = m_pProp->GetAttributeByType<xiiSuffixAttribute>();

  if (pClamp)
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

  if (pDefault)
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
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec2>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec2>().y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<xiiVec3>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec3>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec3>().y);
          m_pWidget[2]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec3>().z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<xiiVec4>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4>().y);
          m_pWidget[2]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4>().z);
          m_pWidget[3]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4>().w);
        }
        break;
      }
    }
  }

  if (pSuffix)
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setDisplaySuffix(pSuffix->GetSuffix());
    }
  }

  const xiiMinValueTextAttribute* pMinValueText = m_pProp->GetAttributeByType<xiiMinValueTextAttribute>();
  if (pMinValueText)
  {
    for (int i = 0; i < m_iNumComponents; ++i)
    {
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void xiiQtPropertyEditorDoubleSpinboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

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
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorDoubleSpinboxWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged(m_pWidget[0]->value());
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

  BroadcastValueChanged(xiiTime::Seconds(m_pWidget->value()));
}


/// *** ANGLE SPINBOX ***

xiiQtPropertyEditorAngleWidget::xiiQtPropertyEditorAngleWidget() :
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

void xiiQtPropertyEditorAngleWidget::OnInit()
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
    m_pWidget->setSpecialValueText(pMinValueText->GetText());
  }
}

void xiiQtPropertyEditorAngleWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b0(m_pWidget);
  m_pWidget->setValue(value);
}

void xiiQtPropertyEditorAngleWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorAngleWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  BroadcastValueChanged(xiiAngle::Degree(m_pWidget->value()));
}

/// *** INT SPINBOX ***


xiiQtPropertyEditorIntSpinboxWidget::xiiQtPropertyEditorIntSpinboxWidget(xiiInt8 iNumComponents, xiiInt32 iMinValue, xiiInt32 iMaxValue) :
  xiiQtStandardPropertyWidget()
{
  m_iNumComponents    = iNumComponents;
  m_bTemporaryCommand = false;

  m_pWidget[0] = nullptr;
  m_pWidget[1] = nullptr;
  m_pWidget[2] = nullptr;
  m_pWidget[3] = nullptr;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QSizePolicy policy = sizePolicy();
  policy.setHorizontalStretch(2);

  for (xiiInt32 c = 0; c < m_iNumComponents; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this, true);
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

void xiiQtPropertyEditorIntSpinboxWidget::OnInit()
{
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

        if (pClamp->GetMinValue().IsValid() && pClamp->GetMaxValue().IsValid() && (iMaxValue - iMinValue) < 256)
        {
          xiiQtScopedBlockSignals bs2(m_pSlider);

          // we have to create the slider here, because in the constructor we don't know the real
          // min and max values from the xiiClampValueAttribute (only the rough type ranges)
          m_pSlider = new QSlider(this);
          m_pSlider->setOrientation(Qt::Orientation::Horizontal);
          m_pSlider->setMinimum(iMinValue);
          m_pSlider->setMaximum(iMaxValue);

          m_pLayout->insertWidget(0, m_pSlider, 5); // make it take up most of the space
          connect(m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(SlotSliderValueChanged(int)));
          connect(m_pSlider, SIGNAL(sliderReleased()), this, SLOT(on_EditingFinished_triggered()));
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
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec2I32>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec2I32>().y);
        }
        break;
      }
      case 3:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2]);

        if (pDefault->GetValue().CanConvertTo<xiiVec3I32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec3I32>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec3I32>().y);
          m_pWidget[2]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec3I32>().z);
        }
        break;
      }
      case 4:
      {
        xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3]);

        if (pDefault->GetValue().CanConvertTo<xiiVec4I32>())
        {
          m_pWidget[0]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4I32>().x);
          m_pWidget[1]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4I32>().y);
          m_pWidget[2]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4I32>().z);
          m_pWidget[3]->setDefaultValue(pDefault->GetValue().ConvertTo<xiiVec4I32>().w);
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
      m_pWidget[i]->setSpecialValueText(pMinValueText->GetText());
    }
  }
}

void xiiQtPropertyEditorIntSpinboxWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals bs(m_pWidget[0], m_pWidget[1], m_pWidget[2], m_pWidget[3], m_pSlider);

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
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  switch (m_iNumComponents)
  {
    case 1:
      BroadcastValueChanged((xiiInt32)m_pWidget[0]->value());

      if (m_pSlider)
      {
        xiiQtScopedBlockSignals b0(m_pSlider);
        m_pSlider->setValue((xiiInt32)m_pWidget[0]->value());
      }

      break;
    case 2:
      BroadcastValueChanged(xiiVec2I32(m_pWidget[0]->value(), m_pWidget[1]->value()));
      break;
    case 3:
      BroadcastValueChanged(xiiVec3I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value()));
      break;
    case 4:
      BroadcastValueChanged(xiiVec4I32(m_pWidget[0]->value(), m_pWidget[1]->value(), m_pWidget[2]->value(), m_pWidget[3]->value()));
      break;
  }
}

void xiiQtPropertyEditorIntSpinboxWidget::SlotSliderValueChanged(int value)
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  {
    xiiQtScopedBlockSignals b0(m_pWidget[0]);
    m_pWidget[0]->setValue(value);
  }

  BroadcastValueChanged((xiiInt32)m_pSlider->value());
}

void xiiQtPropertyEditorIntSpinboxWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}


/// *** QUATERNION ***

xiiQtPropertyEditorQuaternionWidget::xiiQtPropertyEditorQuaternionWidget() :
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

  for (xiiInt32 c = 0; c < 3; ++c)
  {
    m_pWidget[c] = new xiiQtDoubleSpinBox(this);
    m_pWidget[c]->setMinimum(-xiiMath::Infinity<double>());
    m_pWidget[c]->setMaximum(xiiMath::Infinity<double>());
    m_pWidget[c]->setSingleStep(1.0);
    m_pWidget[c]->setAccelerated(true);
    m_pWidget[c]->setDisplaySuffix("\xC2\xB0");

    policy.setHorizontalStretch(2);
    m_pWidget[c]->setSizePolicy(policy);

    m_pLayout->addWidget(m_pWidget[c]);

    connect(m_pWidget[c], SIGNAL(editingFinished()), this, SLOT(on_EditingFinished_triggered()));
    connect(m_pWidget[c], SIGNAL(valueChanged(double)), this, SLOT(SlotValueChanged()));
  }
}

void xiiQtPropertyEditorQuaternionWidget::OnInit() {}

void xiiQtPropertyEditorQuaternionWidget::InternalSetValue(const xiiVariant& value)
{
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

void xiiQtPropertyEditorQuaternionWidget::on_EditingFinished_triggered()
{
  if (m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::EndTemporary);

  m_bTemporaryCommand = false;
}

void xiiQtPropertyEditorQuaternionWidget::SlotValueChanged()
{
  if (!m_bTemporaryCommand)
    Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  m_bTemporaryCommand = true;

  xiiAngle x = xiiAngle::Degree(m_pWidget[0]->value());
  xiiAngle y = xiiAngle::Degree(m_pWidget[1]->value());
  xiiAngle z = xiiAngle::Degree(m_pWidget[2]->value());

  xiiQuat qRot;
  qRot.SetFromEulerAngles(x, y, z);

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
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered()));
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

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(value.ConvertTo<xiiString>().GetData()));
  }
}

void xiiQtPropertyEditorLineEditWidget::on_TextChanged_triggered(const QString& value)
{
  BroadcastValueChanged(value.toUtf8().data());
}

void xiiQtPropertyEditorLineEditWidget::on_TextFinished_triggered()
{
  BroadcastValueChanged(m_pWidget->text().toUtf8().data());
}


/// *** COLOR ***

xiiQtColorButtonWidget::xiiQtColorButtonWidget(QWidget* parent) :
  QFrame(parent)
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

xiiQtPropertyEditorColorWidget::xiiQtPropertyEditorColorWidget() :
  xiiQtStandardPropertyWidget()
{
  m_bExposeAlpha = false;

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
}

void xiiQtPropertyEditorColorWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  m_OriginalValue = GetOldValue();
  m_pWidget->SetColor(value);
}

void xiiQtPropertyEditorColorWidget::on_Button_triggered()
{
  Broadcast(xiiPropertyEvent::Type::BeginTemporary);

  bool bShowHDR = false;

  xiiColor temp = xiiColor::White;
  if (m_OriginalValue.IsValid())
  {
    bShowHDR = m_OriginalValue.IsA<xiiColor>();

    temp = m_OriginalValue.ConvertTo<xiiColor>();
  }

  xiiQtUiServices::GetSingleton()->ShowColorDialog(
    temp, m_bExposeAlpha, bShowHDR, this, SLOT(on_CurrentColor_changed(const xiiColor&)), SLOT(on_Color_accepted()), SLOT(on_Color_reset()));
}

void xiiQtPropertyEditorColorWidget::on_CurrentColor_changed(const xiiColor& color)
{
  xiiVariant col;

  if (m_OriginalValue.IsA<xiiColorGammaUB>())
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

  m_pWidget = new QComboBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int)));
}

void xiiQtPropertyEditorEnumWidget::OnInit()
{
  const xiiRTTI* enumType = m_pProp->GetSpecificType();

  xiiQtScopedBlockSignals bs(m_pWidget);

  xiiStringBuilder sTemp;
  const xiiRTTI*   pType   = enumType;
  xiiUInt32        uiCount = pType->GetProperties().GetCount();
  // Start at 1 to skip default value.
  for (xiiUInt32 i = 1; i < uiCount; ++i)
  {
    auto pProp = pType->GetProperties()[i];

    if (pProp->GetCategory() != xiiPropertyCategory::Constant)
      continue;

    const xiiAbstractConstantProperty* pConstant = static_cast<const xiiAbstractConstantProperty*>(pProp);

    m_pWidget->addItem(QString::fromUtf8(xiiTranslate(pConstant->GetPropertyName())), pConstant->GetConstant().ConvertTo<xiiInt64>());
  }
}

void xiiQtPropertyEditorEnumWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    xiiInt32 iIndex = m_pWidget->findData(value.ConvertTo<xiiInt64>());
    XII_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!");
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void xiiQtPropertyEditorEnumWidget::on_CurrentEnum_changed(int iEnum)
{
  xiiInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
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
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
  connect(m_pMenu, SIGNAL(aboutToHide()), this, SLOT(on_Menu_aboutToHide()));
}

xiiQtPropertyEditorBitflagsWidget::~xiiQtPropertyEditorBitflagsWidget()
{
  m_Constants.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
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
    QCheckBox*     pCheckBox = new QCheckBox(QString::fromUtf8(xiiTranslate(pConstant->GetPropertyName())), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pAction->setDefaultWidget(pCheckBox);

    m_Constants[pConstant->GetConstant().ConvertTo<xiiInt64>()] = pCheckBox;
    m_pMenu->addAction(pAction);
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

xiiQtCurve1DButtonWidget::xiiQtCurve1DButtonWidget(QWidget* parent) :
  QLabel(parent)
{
  setAutoFillBackground(true);
  setCursor(Qt::PointingHandCursor);
  setScaledContents(true);
}

void xiiQtCurve1DButtonWidget::UpdatePreview(xiiObjectAccessorBase* pObjectAccessor, const xiiDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange)
{
  xiiInt32 iNumPoints = 0;
  pObjectAccessor->GetCount(pCurveObject, "ControlPoints", iNumPoints);

  xiiVariant                   v;
  xiiHybridArray<xiiVec2d, 32> points;
  points.Reserve(iNumPoints);

  double minX = fLowerExtents * 4800.0;
  double maxX = fUpperExtents * 4800.0;

  double minY = fLowerRange;
  double maxY = fUpperRange;

  for (xiiInt32 i = 0; i < iNumPoints; ++i)
  {
    const xiiDocumentObject* pPoint = pObjectAccessor->GetChildObject(pCurveObject, "ControlPoints", i);

    xiiVec2d p;

    pObjectAccessor->GetValue(pPoint, "Tick", v);
    p.x = v.ConvertTo<double>();

    pObjectAccessor->GetValue(pPoint, "Value", v);
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
  const xiiDocumentObject*        pCurve       = m_pObjectAccessor->GetChildObject(pParent, m_pProp->GetPropertyName(), {});
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
  const xiiDocumentObject*        pCurve       = m_pObjectAccessor->GetChildObject(pParent, m_pProp->GetPropertyName(), {});
  const xiiColorAttribute*        pColorAttr   = m_pProp->GetAttributeByType<xiiColorAttribute>();
  const xiiCurveExtentsAttribute* pExtentsAttr = m_pProp->GetAttributeByType<xiiCurveExtentsAttribute>();
  const xiiClampValueAttribute*   pClampAttr   = m_pProp->GetAttributeByType<xiiClampValueAttribute>();

  // TODO: would like to have one transaction open to finish/cancel at the end
  // but also be able to undo individual steps while editing
  //m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->StartTransaction("Edit Curve");

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
    //m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->FinishTransaction();

    UpdatePreview();
  }
  else
  {
    //m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->CancelTransaction();
  }

  delete pDlg;
}
