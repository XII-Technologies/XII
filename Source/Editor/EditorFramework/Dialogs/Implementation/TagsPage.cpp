/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/TagsPage.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

xiiQtTagsPage::xiiQtTagsPage(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  LoadTags();
  FillList();

  on_TreeTags_itemSelectionChanged();
}

void xiiQtTagsPage::on_ButtonNewCategory_clicked()
{
  QString sResult = QInputDialog::getText(this, "Category Name", "Name:");

  if (sResult.isEmpty())
    return;

  TreeTags->clearSelection();

  const xiiString sName = sResult.toUtf8().data();

  if (m_CategoryToItem.Find(sName).IsValid())
  {
    xiiQtUiServices::GetSingleton()->MessageBoxInformation("A Category with this name already exists.");
  }
  else
  {
    auto* pItem = new QTreeWidgetItem(TreeTags);
    pItem->setText(0, sResult);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"));

    m_CategoryToItem[sName] = pItem;
  }

  m_CategoryToItem[sName]->setSelected(true);
}

void xiiQtTagsPage::on_ButtonNewTag_clicked()
{
  if (TreeTags->selectedItems().isEmpty())
    return;

  auto pItem = TreeTags->selectedItems()[0];

  if (!pItem)
    return;

  QString sResult = QInputDialog::getText(this, "Tag Name", "Name:");

  if (sResult.isEmpty())
    return;

  if (TreeTags->indexOfTopLevelItem(pItem) < 0)
    pItem = pItem->parent();

  auto pNewItem = CreateTagItem(pItem, sResult, false);
  pItem->setExpanded(true);

  TreeTags->clearSelection();
  pNewItem->setSelected(true);
  // TreeTags->editItem(pNewItem);
}

void xiiQtTagsPage::on_ButtonRemove_clicked()
{
  if (TreeTags->selectedItems().isEmpty())
    return;

  auto pItem = TreeTags->selectedItems()[0];

  if (!pItem)
    return;

  if (TreeTags->indexOfTopLevelItem(pItem) >= 0)
  {
    if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion("Do you really want to remove the entire Tag Category?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;

    m_CategoryToItem.Remove(pItem->text(0).toUtf8().data());
  }
  else
  {
    if (xiiQtUiServices::GetSingleton()->MessageBoxQuestion("Do you really want to remove this Tag?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
      return;
  }

  delete pItem;
}

void xiiQtTagsPage::on_TreeTags_itemSelectionChanged()
{
  const bool hasSelection = !TreeTags->selectedItems().isEmpty();

  ButtonRemove->setEnabled(hasSelection);
  ButtonNewTag->setEnabled(hasSelection);
}

void xiiQtTagsPage::LoadTags()
{
  m_Tags.Clear();

  xiiHybridArray<const xiiToolsTag*, 16> tags;
  xiiToolsTagRegistry::GetAllTags(tags);

  for (const xiiToolsTag* pTag : tags)
  {
    // hide the "Editor" tags from the user
    if (pTag->m_sCategory == "Editor")
      continue;

    auto& tag         = m_Tags.ExpandAndGetRef();
    tag.m_sCategory   = pTag->m_sCategory;
    tag.m_sName       = pTag->m_sName;
    tag.m_bBuiltInTag = pTag->m_bBuiltInTag;
  }
}

xiiResult xiiQtTagsPage::SaveTags()
{
  xiiHybridArray<const xiiToolsTag*, 16> current;
  xiiToolsTagRegistry::GetAllTags(current);
  xiiDynamicArray<xiiToolsTag> previous;
  for (const auto* pTag : current)
    previous.PushBack(*pTag);
  xiiToolsTagRegistry::Clear();

  for (const auto& tag : m_Tags)
  {
    if (!tag.m_bBuiltInTag)
    {
      xiiToolsTagRegistry::AddTag(tag);
    }
  }

  if (xiiQtEditorApp::GetSingleton()->SaveTagRegistry().Failed())
  {
    xiiToolsTagRegistry::Clear();
    for (const auto& tag : previous)
      xiiToolsTagRegistry::AddTag(tag);
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

void xiiQtTagsPage::FillList()
{
  xiiQtScopedBlockSignals    bs(TreeTags);
  xiiQtScopedUpdatesDisabled bu(TreeTags);

  m_CategoryToItem.Clear();
  TreeTags->clear();

  xiiSet<xiiString> TagCategories;

  for (const auto& tag : m_Tags)
  {
    TagCategories.Insert(tag.m_sCategory);
  }

  for (auto it = TagCategories.GetIterator(); it.IsValid(); ++it)
  {
    auto* pItem = new QTreeWidgetItem(TreeTags);
    pItem->setText(0, it.Key().GetData());
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    pItem->setIcon(0, xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Tag.svg"));

    m_CategoryToItem[it.Key()] = pItem;
  }

  for (const auto& tag : m_Tags)
  {
    QTreeWidgetItem* pParentItem = m_CategoryToItem[tag.m_sCategory];

    CreateTagItem(pParentItem, QString::fromUtf8(tag.m_sName.GetData()), tag.m_bBuiltInTag);


    pParentItem->setExpanded(true);
  }

  TreeTags->resizeColumnToContents(0);
}

void xiiQtTagsPage::GetTagsFromList()
{
  m_Tags.Clear();

  for (int sets = 0; sets < TreeTags->topLevelItemCount(); ++sets)
  {
    const auto*     pSetItem      = TreeTags->topLevelItem(sets);
    const xiiString sCategoryName = pSetItem->text(0).toUtf8().data();

    for (int childIdx = 0; childIdx < pSetItem->childCount(); ++childIdx)
    {
      const auto* pTagItem = pSetItem->child(childIdx);

      // ignore disabled items, those are the built-in ones
      if (!pTagItem->flags().testFlag(Qt::ItemFlag::ItemIsEnabled))
        continue;

      xiiToolsTag& cfg = m_Tags.ExpandAndGetRef();
      cfg.m_sCategory  = sCategoryName;


      cfg.m_sName = pTagItem->text(0).toUtf8().data();
    }
  }
}

QTreeWidgetItem* xiiQtTagsPage::CreateTagItem(QTreeWidgetItem* pParentItem, const QString& tag, bool bBuiltIn)
{
  auto* pItem = new QTreeWidgetItem(pParentItem);

  if (bBuiltIn)
  {
    pItem->setText(0, tag + QString(" (built in)"));
    pItem->setFlags(Qt::ItemFlags());
  }
  else
  {
    pItem->setText(0, tag);
    pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEditable);
  }

  return pItem;
}

xiiResult xiiQtTagsPage::ApplyChanges()
{
  GetTagsFromList();
  return SaveTags();
}
