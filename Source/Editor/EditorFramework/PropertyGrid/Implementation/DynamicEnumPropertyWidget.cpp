/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/PropertyGrid/DynamicEnumPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>

xiiMap<xiiString, QString> xiiQtDynamicEnumPropertyWidget::s_LastSearch;

xiiQtDynamicEnumPropertyWidget::xiiQtDynamicEnumPropertyWidget() :
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

void xiiQtDynamicEnumPropertyWidget::OnInit()
{
  XII_ASSERT_DEV(m_pProp->GetAttributeByType<xiiDynamicEnumAttribute>() != nullptr, "xiiQtDynamicEnumPropertyWidget was created without a xiiDynamicEnumAttribute!");

  const xiiDynamicEnumAttribute* pAttr = m_pProp->GetAttributeByType<xiiDynamicEnumAttribute>();

  m_sEnumAttribute = pAttr->GetDynamicEnumName();

  m_pEnum = &xiiDynamicEnum::GetDynamicEnum(m_sEnumAttribute);

  m_pMenu = new QMenu(m_pButton);
  m_pMenu->setToolTipsVisible(false);
  connect(m_pMenu, &QMenu::aboutToShow, this, &xiiQtDynamicEnumPropertyWidget::onMenuAboutToShow);
  m_pButton->setMenu(m_pMenu);
}

void xiiQtDynamicEnumPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  m_pButton->setText(xiiMakeQString(m_pEnum->GetValueName(value.ConvertTo<xiiInt64>())));
}

void xiiQtDynamicEnumPropertyWidget::onMenuAboutToShow()
{
  m_pMenu->clear();

  m_pSearchableMenu = new xiiQtSearchableMenu(m_pMenu);

  connect(m_pSearchableMenu, &xiiQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant) {
    if (variant.typeId() == QMetaType::QString)
    {
      if (variant.toString() == "<cmd>")
      {
        xiiActionManager::ExecuteAction({}, m_pEnum->GetEditCommand(), xiiActionContext(const_cast<xiiDocument*>(m_pGrid->GetDocument())), m_pEnum->GetEditCommandValue()).AssertSuccess();
      }
    }
    else
    {
      InternalSetValue(variant.toLongLong());
      BroadcastValueChanged(variant.toLongLong());
    }

    m_pMenu->close();
  });

  connect(m_pSearchableMenu, &xiiQtSearchableMenu::SearchTextChanged, m_pMenu, [this](const QString& sText) {
    s_LastSearch[m_sEnumAttribute] = sText;
  });

  if (!m_pEnum->GetEditCommand().IsEmpty())
  {
    m_pSearchableMenu->AddItem("< Edit Values... >", "", QString("<cmd>"), QIcon(":/GuiFoundation/Icons/Edit.svg"));
  }

  const auto& allValues = m_pEnum->GetAllValidValues();

  for (auto it = allValues.GetIterator(); it.IsValid(); ++it)
  {
    m_pSearchableMenu->AddItem(it.Value(), "", it.Key());
  }

  m_pMenu->addAction(m_pSearchableMenu);

  // Important to do this last to make sure the search bar gets focus.
  m_pSearchableMenu->Finalize(s_LastSearch[m_sEnumAttribute]);
}
