#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

xiiQtDynamicStringEnumPropertyWidget::xiiQtDynamicStringEnumPropertyWidget() :
  xiiQtStandardPropertyWidget()
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

void xiiQtDynamicStringEnumPropertyWidget::OnInit()
{
  XII_ASSERT_DEV(m_pProp->GetAttributeByType<xiiDynamicStringEnumAttribute>() != nullptr, "xiiQtDynamicStringEnumPropertyWidget was created without a xiiDynamicStringEnumAttribute!");

  const xiiDynamicStringEnumAttribute* pAttr = m_pProp->GetAttributeByType<xiiDynamicStringEnumAttribute>();

  m_pEnum = &xiiDynamicStringEnum::GetDynamicEnum(pAttr->GetDynamicEnumName());

  if (auto pDefaultValueAttr = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    m_pEnum->AddValidValue(pDefaultValueAttr->GetValue().ConvertTo<xiiString>(), true);
  }

  const auto& AllValues = m_pEnum->GetAllValidValues();

  xiiQtScopedBlockSignals bs(m_pWidget);
  m_pWidget->clear();

  for (const auto& val : AllValues)
  {
    m_pWidget->addItem(QString::fromUtf8(val.GetData()));
  }

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pWidget->addItem("< Edit Values... >", QString("<cmd>"));
  }
  else if (!m_pEnum->GetStorageFile().IsEmpty())
  {
    m_pWidget->addItem("< Edit Values... >", QString("<edit>"));
  }
}

void xiiQtDynamicStringEnumPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);

  if (value.IsValid())
  {
    m_iLastIndex = m_pWidget->findText(value.ConvertTo<xiiString>().GetData());
  }
  else
  {
    m_iLastIndex = -1;
  }

  m_pWidget->setCurrentIndex(m_iLastIndex);
}

void xiiQtDynamicStringEnumPropertyWidget::on_CurrentEnum_changed(int iEnum)
{
  if (m_pWidget->currentData() == QString("<cmd>"))
  {
    iEnum = m_iLastIndex;
    m_pWidget->setCurrentIndex(iEnum);

    xiiActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), xiiActionContext(const_cast<xiiDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();

    return;
  }

  if (m_pWidget->currentData() == QString("<edit>"))
  {
    xiiQtEditDynamicEnumsDlg dlg(m_pEnum, this);
    if (dlg.exec() == QDialog::Accepted)
    {
      iEnum = dlg.GetSelectedItem();
      OnInit();
    }
    else
    {
      iEnum = m_iLastIndex;
    }

    m_pWidget->setCurrentIndex(iEnum);
    return;
  }

  m_iLastIndex   = iEnum;
  QString sValue = m_pWidget->itemText(iEnum);
  BroadcastValueChanged(sValue.toUtf8().data());
}
