#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

xiiQtDynamicEnumPropertyWidget::xiiQtDynamicEnumPropertyWidget() : xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QComboBox(this);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CurrentEnum_changed(int))) != nullptr, "connection failed");
}

void xiiQtDynamicEnumPropertyWidget::OnInit()
{
  XII_ASSERT_DEV(m_pProp->GetAttributeByType<xiiDynamicEnumAttribute>() != nullptr, "xiiQtDynamicEnumPropertyWidget was created without a xiiDynamicEnumAttribute!");

  const xiiDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<xiiDynamicEnumAttribute>();

  m_pDynamicEnum            = &xiiDynamicEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());
  const auto& allEnumValues = m_pDynamicEnum->GetAllValidValues();

  xiiQtScopedBlockSignals bs(m_pWidget);

  for (auto it = allEnumValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pWidget->addItem(QString::fromUtf8(it.Value().GetData()), it.Key());
  }

  if (!m_pDynamicEnum->GetEditCommand().IsEmpty())
  {
    m_pWidget->addItem("< Edit Values... >", QString("<cmd>"));
  }
}

void xiiQtDynamicEnumPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_iLastIndex = m_pWidget->findData(value.ConvertTo<xiiInt64>());
  }
  else
  {
    m_iLastIndex = -1;
  }

  m_pWidget->setCurrentIndex(m_iLastIndex);
}

void xiiQtDynamicEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  if (m_pWidget->currentData() == QString("<cmd>"))
  {
    iEnum = m_iLastIndex;
    m_pWidget->setCurrentIndex(iEnum);

    xiiActionManager::ExecuteAction({}, m_pDynamicEnum->GetEditCommand(), xiiActionContext(const_cast<xiiDocument*>(m_pGrid->GetDocument())), m_pDynamicEnum->GetEditCommandValue()).AssertSuccess();

    return;
  }

  m_iLastIndex    = m_pWidget->currentIndex();
  xiiInt64 iValue = m_pWidget->itemData(iEnum).toLongLong();
  BroadcastValueChanged(iValue);
}
