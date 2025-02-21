#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

/// *** Tag Set ***

xiiQtPropertyEditorTagSetWidget::xiiQtPropertyEditorTagSetWidget() :
  xiiQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QPushButton(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pMenu = nullptr;
  m_pMenu = new QMenu(m_pWidget);
  m_pMenu->setToolTipsVisible(true);
  m_pWidget->setMenu(m_pMenu);
  m_pLayout->addWidget(m_pWidget);

  connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(on_Menu_aboutToShow()));
}

xiiQtPropertyEditorTagSetWidget::~xiiQtPropertyEditorTagSetWidget()
{
  m_Tags.Clear();
  m_pWidget->setMenu(nullptr);

  delete m_pMenu;
  m_pMenu = nullptr;
}

void xiiQtPropertyEditorTagSetWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtPropertyWidget::SetSelection(items);
  InternalUpdateValue();
}

void xiiQtPropertyEditorTagSetWidget::OnInit()
{
  XII_ASSERT_DEV(m_pProp->GetCategory() == xiiPropertyCategory::Set && (m_pProp->GetSpecificType() == xiiGetStaticRTTI<xiiConstCharPtr>() || m_pProp->GetSpecificType() == xiiGetStaticRTTI<xiiStringView>()), "xiiQtPropertyEditorTagSetWidget only works with xiiTagSet.");

  // Retrieve tag categories.
  const xiiTagSetWidgetAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiTagSetWidgetAttribute>();
  XII_ASSERT_DEV(pAssetAttribute != nullptr, "xiiQtPropertyEditorTagSetWidget needs xiiTagSetWidgetAttribute to be set.");
  xiiStringBuilder                 sTagFilter = pAssetAttribute->GetTagFilter();
  xiiHybridArray<xiiStringView, 4> categories;
  sTagFilter.Split(false, categories, ";");

  // Get tags by categories.
  xiiHybridArray<const xiiToolsTag*, 16> tags;
  xiiToolsTagRegistry::GetTagsByCategory(categories, tags);

  xiiStringView sCurrentCategory = {};

  // Add valid tags to menu.
  for (const xiiToolsTag* pTag : tags)
  {
    if (!pTag->m_sCategory.IsEqual(sCurrentCategory))
    {
      /*QAction* pCategory = */ m_pMenu->addSection(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"), QLatin1String("[") + QString(pTag->m_sCategory.GetData()) + QLatin1String("]"));

      sCurrentCategory = pTag->m_sCategory;

      // remove category from list, as it was added once

      /// \todo xiiStringView is POD? -> array<stringview>::Remove(stringview) fails, because of memcmp
      // categories.Remove(sCurrentCategory);

      for (xiiUInt32 i = 0; i < categories.GetCount(); ++i)
      {
        if (categories[i] == sCurrentCategory)
        {
          categories.RemoveAtAndCopy(i);
          break;
        }
      }
    }

    QWidgetAction* pAction   = new QWidgetAction(m_pMenu);
    QCheckBox*     pCheckBox = new QCheckBox(pTag->m_sName.GetData(), m_pMenu);
    pCheckBox->setCheckable(true);
    pCheckBox->setCheckState(Qt::Unchecked);
    pCheckBox->setProperty("Tag", pTag->m_sName.GetData());
    connect(pCheckBox, &QCheckBox::clicked, this, &xiiQtPropertyEditorTagSetWidget::onCheckBoxClicked);
    pAction->setDefaultWidget(pCheckBox);

    m_Tags.PushBack(pCheckBox);
    m_pMenu->addAction(pAction);
  }

  xiiStringBuilder tmp;

  // if a tag category is empty, it will never show up in the menu, thus the user doesn't know the name of the valid category
  // therefore, for every empty category, add an entry
  for (const auto& catname : categories)
  {
    /*QAction* pCategory = */ m_pMenu->addSection(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"), QLatin1String("[") + QString(catname.GetData(tmp)) + QLatin1String("]"));
  }
}

void xiiQtPropertyEditorTagSetWidget::InternalUpdateValue()
{
  xiiMap<xiiString, xiiUInt32> tags;
  // Count used tags of each object in the selection.
  for (auto& item : m_Items)
  {
    xiiHybridArray<xiiVariant, 16> currentSetValues;
    xiiStatus                      status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
    XII_ASSERT_DEV(status.m_Result.Succeeded(), "Failed to get tag keys!");
    for (const xiiVariant& key : currentSetValues)
    {
      XII_ASSERT_DEV(key.GetType() == xiiVariantType::String || key.GetType() == xiiVariantType::StringView, "Tags are supposed to be of type string or string view!");
      if (key.IsA<xiiStringView>())
        tags[key.Get<xiiStringView>()]++;
      else
        tags[key.Get<xiiString>()]++;
    }
  }

  // Update checkbox state
  QString   sText;
  xiiUInt32 uiCount = m_Items.GetCount();
  for (QCheckBox* pCheckBox : m_Tags)
  {
    xiiString value  = pCheckBox->property("Tag").toString().toUtf8().data();
    xiiUInt32 uiUsed = tags[value];

    xiiQtScopedBlockSignals b(pCheckBox);
    if (uiUsed == 0)
    {
      pCheckBox->setCheckState(Qt::CheckState::Unchecked);
    }
    else if (uiUsed == uiCount)
    {
      pCheckBox->setCheckState(Qt::CheckState::Checked);
      sText += value.GetData();
      sText += "|";
    }
    else
    {
      pCheckBox->setCheckState(Qt::CheckState::PartiallyChecked);
      sText = "<Multiple Values>|"; // string is shrunk by one character (see below), so | is a dummy
    }
  }

  xiiQtScopedBlockSignals b(m_pWidget);
  if (!sText.isEmpty())
    sText = sText.left(sText.size() - 1);
  else
    sText = "<none>";

  // m_pWidget->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"));
  m_pWidget->setText(sText);
}

void xiiQtPropertyEditorTagSetWidget::on_Menu_aboutToShow()
{
  m_pMenu->setMinimumWidth(m_pWidget->geometry().width());
}

void xiiQtPropertyEditorTagSetWidget::onCheckBoxClicked(bool bChecked)
{
  QCheckBox* pCheckBox = qobject_cast<QCheckBox*>(sender());
  xiiVariant value     = pCheckBox->property("Tag").toString().toUtf8().data();
  if (pCheckBox->isChecked())
  {
    m_pObjectAccessor->StartTransaction("Add Tag");

    // Add tag to all objects in selection that don't have it yet.
    for (auto& item : m_Items)
    {
      xiiHybridArray<xiiVariant, 16> currentSetValues;

      xiiStatus status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
      XII_ASSERT_DEV(status.m_Result.Succeeded(), "Failed to get tag keys!");
      if (!currentSetValues.Contains(value))
      {
        auto res = m_pObjectAccessor->InsertValue(item.m_pObject, m_pProp, value, -1);
        if (res.m_Result.Failed())
        {
          XII_REPORT_FAILURE("Failed to add '{0}' tag to tag set", value.Get<xiiString>());
        }
      }
    }
  }
  else
  {
    m_pObjectAccessor->StartTransaction("Remove Tag");

    xiiRemoveObjectPropertyCommand cmd;
    cmd.m_sProperty = m_pProp->GetPropertyName();

    // Remove tag from all objects in selection that have it.
    for (auto& item : m_Items)
    {
      xiiHybridArray<xiiVariant, 16> currentSetValues;
      xiiStatus                      status = m_pObjectAccessor->GetValues(item.m_pObject, m_pProp, currentSetValues);
      XII_ASSERT_DEV(status.m_Result.Succeeded(), "Failed to get tag keys!");
      xiiUInt32 uiIndex = currentSetValues.IndexOf(value);
      if (uiIndex != -1)
      {
        auto res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, uiIndex);
        if (res.m_Result.Failed())
        {
          XII_REPORT_FAILURE("Failed to remove '{0}' tag from tag set", value.Get<xiiString>());
        }
      }
    }
  }

  m_pObjectAccessor->FinishTransaction();
}
