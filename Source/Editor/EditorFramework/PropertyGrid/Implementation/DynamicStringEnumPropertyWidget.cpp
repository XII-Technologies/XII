/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/EditDynamicEnumsDlg.moc.h>
#include <EditorFramework/PropertyGrid/DynamicStringEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

xiiMap<xiiString, QString> xiiQtDynamicStringEnumPropertyWidget::s_LastSearch;

xiiQtDynamicStringEnumPropertyWidget::xiiQtDynamicStringEnumPropertyWidget() :
  xiiQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Select");
  m_pButton->setStyleSheet("QPushButton { text-align:left; padding-left:5px; padding-top:3px; padding-bottom:3px; }");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addWidget(m_pButton);
}

void xiiQtDynamicStringEnumPropertyWidget::OnInit()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  XII_ASSERT_DEV(m_pProp->GetAttributeByType<xiiDynamicStringEnumAttribute>() != nullptr, "xiiQtDynamicStringEnumPropertyWidget was created without a xiiDynamicStringEnumAttribute!");
  xiiVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  XII_ASSERT_DEV(type == xiiVariantType::String || type == xiiVariantType::HashedString || type == xiiVariantType::StringView, "xiiDynamicStringEnumAttribute can only be used with string types.s");
#endif

  const xiiDynamicStringEnumAttribute* pAttribute = m_pProp->GetAttributeByType<xiiDynamicStringEnumAttribute>();

  m_sEnumAttribute = pAttribute->GetDynamicEnumName();

  m_pEnum = &xiiDynamicStringEnum::GetDynamicEnum(m_sEnumAttribute);

  if (auto pDefaultValueAttr = m_pProp->GetAttributeByType<xiiDefaultValueAttribute>())
  {
    m_pEnum->AddValidValue(pDefaultValueAttr->GetValue().ConvertTo<xiiString>(), true);
  }

  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &xiiQtDynamicStringEnumPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
}

void xiiQtDynamicStringEnumPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  m_pButton->setText(xiiMakeQString(value.ConvertTo<xiiString>()));
}

void xiiQtDynamicStringEnumPropertyWidget::SetNewValue(xiiStringView sNewValue)
{
  xiiVariant           v;
  xiiVariantType::Enum type = m_pProp->GetSpecificType()->GetVariantType();
  if (type == xiiVariantType::String || type == xiiVariantType::StringView)
  {
    v = xiiVariant(sNewValue);
  }
  else if (type == xiiVariantType::HashedString)
  {
    xiiHashedString s;
    s.Assign(sNewValue);
    v = s;
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }

  InternalSetValue(v);
  BroadcastValueChanged(v);
}

void xiiQtDynamicStringEnumPropertyWidget::onMenuAboutToShow()
{
  m_pMenu->clear();

  m_pSearchableMenu = new xiiQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &xiiQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant) {
    if (variant.toString() == "<item>")
    {
      SetNewValue(sName.toUtf8().data());
    }
    else if (variant.toString() == "<edit>")
    {
      xiiQtEditDynamicEnumsDlg dlg(m_pEnum, this);
      if (dlg.exec() == QDialog::Accepted)
      {
        xiiInt32 iEnum = dlg.GetSelectedItem();
        if (iEnum >= 0)
        {
          SetNewValue(m_pEnum->GetAllValidValues()[iEnum]);
        }
      }
    }
    else if (variant.toString() == "<cmd>")
    {
      xiiActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), xiiActionContext(const_cast<xiiDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();
    }

    m_pMenu->close();
  });

  connect(m_pSearchableMenu, &xiiQtSearchableMenu::SearchTextChanged, m_pMenu, [this](const QString& sText) {
    s_LastSearch[m_sEnumAttribute] = sText;
  });

  const auto& allValues = m_pEnum->GetAllValidValues();

  for (const auto& value : allValues)
  {
    m_pSearchableMenu->AddItem(value, "", QString("<item>"));
  }

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<cmd>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }
  else if (!m_pEnum->GetStorageFile().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<edit>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }

  m_pMenu->addAction(m_pSearchableMenu);

  // Important to do this last to make sure the search bar gets focus.
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumAttribute]);
}
