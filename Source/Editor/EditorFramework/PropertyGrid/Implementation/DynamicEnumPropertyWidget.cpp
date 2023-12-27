#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

xiiQtDynamicEnumPropertyWidget::xiiQtDynamicEnumPropertyWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int))) != nullptr, "connection failed");
}

void xiiQtDynamicEnumPropertyWidget::OnInit()
{
  XII_ASSERT_DEV(m_pProp->GetAttributeByType<xiiDynamicEnumAttribute>() != nullptr, "xiiQtDynamicEnumPropertyWidget was created without a xiiDynamicEnumAttribute!");

  const xiiDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<xiiDynamicEnumAttribute>();

  const auto& denum     = xiiDynamicEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& AllValues = denum.GetAllValidValues();

  xiiQtScopedBlockSignals bs(m_pWidget);

  for (auto it = AllValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pWidget->addItem(QString::fromUtf8(it.Value().GetData()), it.Key());
  }
}

void xiiQtDynamicEnumPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    xiiInt32 iIndex = m_pWidget->findData(value.ConvertTo<xiiInt64>());
    // XII_ASSERT_DEV(iIndex != -1, "Enum widget is set to an invalid value!"); // 'invalid value'
    m_pWidget->setCurrentIndex(iIndex);
  }
  else
  {
    m_pWidget->setCurrentIndex(-1);
  }
}

void xiiQtDynamicEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  xiiInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}
