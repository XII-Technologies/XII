#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>

xiiQtAssetBrowserWidget::xiiQtAssetBrowserWidget(QWidget* parent) :
  QWidget(parent)
{
  m_uiKnownAssetFolderCount = 0;
  m_bDialogMode             = false;

  setupUi(this);

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();

  ListTypeFilter->setVisible(!pPreferences->m_bAssetFilterCombobox);
  TypeFilter->setVisible(pPreferences->m_bAssetFilterCombobox);

  m_pFilter = new xiiQtAssetBrowserFilter(this);
  m_pModel  = new xiiQtAssetBrowserModel(this, m_pFilter);

  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // Tool Bar
  {
    m_pToolbar = new xiiQtToolBarActionMapView("Toolbar", this);
    xiiActionContext context;
    context.m_sMapping  = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    m_pToolbar->SetActionContext(context);
    m_pToolbar->setObjectName("AssetBrowserToolBar");
    ToolBarLayout->insertWidget(0, m_pToolbar);
  }

  TreeFolderFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  XII_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
  connect(SearchWidget, &xiiQtSearchWidget::textChanged, this, &xiiQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::ProjectEventHandler, this));
}

xiiQtAssetBrowserWidget::~xiiQtAssetBrowserWidget()
{
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::ProjectEventHandler, this));
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::AssetCuratorEventHandler, this));

  ListAssets->setModel(nullptr);
}

void xiiQtAssetBrowserWidget::UpdateAssetTypes()
{
  const auto& assetTypes0 = xiiAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  xiiMap<xiiString, const xiiDocumentTypeDescriptor*> assetTypes;
  for (auto it : assetTypes0)
  {
    assetTypes[xiiTranslate(it.Key())] = it.Value();
  }

  {
    xiiQtScopedBlockSignals block(ListTypeFilter);

    ListTypeFilter->clear();

    // 'All' Filter
    {
      QListWidgetItem* pItem = new QListWidgetItem(QIcon(QLatin1String(":/AssetIcons/All")), QLatin1String("<All>"));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Checked);
      pItem->setData(Qt::UserRole, QLatin1String("<All>"));

      ListTypeFilter->addItem(pItem);
    }

    for (const auto& it : assetTypes)
    {
      QListWidgetItem* pItem = new QListWidgetItem(xiiQtUiServices::GetCachedIconResource(it.Value()->m_sIcon), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Unchecked);
      pItem->setData(Qt::UserRole, QLatin1String(it.Value()->m_sDocumentTypeName));

      ListTypeFilter->addItem(pItem);
    }
  }

  {
    xiiQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    // 'All' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/All")), QLatin1String("<All>"));

    for (const auto& it : assetTypes)
    {
      TypeFilter->addItem(xiiQtUiServices::GetCachedIconResource(it.Value()->m_sIcon), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  UpdateDirectoryTree();

  // make sure to apply the previously active type filter settings to the UI
  OnTypeFilterChanged();
}

void xiiQtAssetBrowserWidget::SetDialogMode()
{
  m_pToolbar->hide();
  m_bDialogMode = true;

  ListAssets->SetDialogMode(true);
}

void xiiQtAssetBrowserWidget::SaveState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    Settings.setValue("SplitterGeometry", splitter->saveGeometry());
    Settings.setValue("SplitterState", splitter->saveState());
    Settings.setValue("IconSize", IconSizeSlider->value());
    Settings.setValue("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode);
  }
  Settings.endGroup();
}

void xiiQtAssetBrowserWidget::RestoreState(const char* szSettingsName)
{
  QSettings Settings;
  Settings.beginGroup(QLatin1String(szSettingsName));
  {
    splitter->restoreGeometry(Settings.value("SplitterGeometry", splitter->saveGeometry()).toByteArray());
    splitter->restoreState(Settings.value("SplitterState", splitter->saveState()).toByteArray());
    IconSizeSlider->setValue(Settings.value("IconSize", IconSizeSlider->value()).toInt());

    if (Settings.value("IconMode", ListAssets->viewMode() == QListView::ViewMode::IconMode).toBool())
      on_ButtonIconMode_clicked();
    else
      on_ButtonListMode_clicked();
  }
  Settings.endGroup();
}

void xiiQtAssetBrowserWidget::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectOpened:
    {
      // this is necessary to detect new asset types when a plugin has been loaded (on project load)
      UpdateAssetTypes();
    }
    break;
    case xiiToolsProjectEvent::Type::ProjectClosed:
    {
      // remove project structure from asset browser
      ClearDirectoryTree();

      m_pFilter->Reset();
    }
    break;
    default:
      break;
  }
}


void xiiQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset)
{
  if (m_bDialogMode)
    return;

  const xiiHybridArray<xiiDocumentManager*, 16>& managers = xiiDocumentManager::GetAllDocumentManagers();

  xiiDynamicArray<const xiiDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu("New");

  xiiStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (xiiDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const xiiDocumentTypeDescriptor* a, const xiiDocumentTypeDescriptor* b) -> bool { return xiiStringUtils::Compare(xiiTranslate(a->m_sDocumentTypeName), xiiTranslate(b->m_sDocumentTypeName)) < 0; });

  for (const xiiDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(xiiTranslate(desc->m_sDocumentTypeName));
    pAction->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &xiiQtAssetBrowserWidget::OnNewAsset);
  }
}

void xiiQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  Q_EMIT ItemSelected(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void xiiQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  Q_EMIT ItemSelected(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void xiiQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();

  if (guid.IsValid())
  {
    xiiAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
  }

  Q_EMIT ItemChosen(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
}

void xiiQtAssetBrowserWidget::on_ButtonListMode_clicked()
{
  m_pModel->SetIconMode(false);
  ListAssets->SetIconMode(false);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(true);
  ButtonIconMode->setChecked(false);
}

void xiiQtAssetBrowserWidget::on_ButtonIconMode_clicked()
{
  m_pModel->SetIconMode(true);
  ListAssets->SetIconMode(true);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(false);
  ButtonIconMode->setChecked(true);
}

void xiiQtAssetBrowserWidget::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void xiiQtAssetBrowserWidget::on_ListAssets_ViewZoomed(xiiInt32 iIconSizePercentage)
{
  xiiQtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void xiiQtAssetBrowserWidget::OnTextFilterChanged()
{
  SearchWidget->setText(QString::fromUtf8(m_pFilter->GetTextFilter()));

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void xiiQtAssetBrowserWidget::OnTypeFilterChanged()
{
  xiiStringBuilder       sTemp;
  const xiiStringBuilder sFilter(";", m_pFilter->GetTypeFilter(), ";");


  {
    xiiQtScopedBlockSignals _(ListTypeFilter);

    bool bNoneChecked = true;

    for (xiiInt32 i = 1; i < ListTypeFilter->count(); ++i)
    {
      sTemp.Set(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");

      const bool bChecked = sFilter.FindSubString(sTemp) != nullptr;

      ListTypeFilter->item(i)->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);

      if (bChecked)
        bNoneChecked = false;
    }

    ListTypeFilter->item(0)->setCheckState(bNoneChecked ? Qt::Checked : Qt::Unchecked);
  }

  {
    xiiQtScopedBlockSignals _(TypeFilter);

    xiiInt32 iCheckedFilter = 0;
    xiiInt32 iNumChecked    = 0;

    for (xiiInt32 i = 1; i < TypeFilter->count(); ++i)
    {
      sTemp.Set(";", TypeFilter->itemData(i, Qt::UserRole).toString().toUtf8().data(), ";");

      if (sFilter.FindSubString(sTemp) != nullptr)
      {
        ++iNumChecked;
        iCheckedFilter = i;
      }
    }

    if (iNumChecked == 1)
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex(0);
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}


void xiiQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void xiiQtAssetBrowserWidget::on_ListTypeFilter_itemChanged(QListWidgetItem* item)
{
  xiiQtScopedBlockSignals block(ListTypeFilter);

  if (item->data(Qt::UserRole).toString() == "<All>")
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate all others
      for (xiiInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
      }
    }
    else
    {
      xiiStringBuilder sFilter;

      // activate all others
      for (xiiInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (!m_sAllTypesFilter.IsEmpty())
        {
          sFilter.Set(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");

          if (m_sAllTypesFilter.FindSubString(sFilter) != nullptr)
            ListTypeFilter->item(i)->setCheckState(Qt::Checked);
          else
            ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
        }
        else
          ListTypeFilter->item(i)->setCheckState(Qt::Checked);
      }
    }
  }
  else
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate the 'all' button
      ListTypeFilter->item(0)->setCheckState(Qt::Unchecked);
    }
    else
    {
      bool bAnyChecked = false;

      for (xiiInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
          bAnyChecked = true;
      }

      // activate the 'All' item
      if (!bAnyChecked)
        ListTypeFilter->item(0)->setCheckState(Qt::Checked);
    }
  }

  xiiStringBuilder sFilter;

  for (xiiInt32 i = 1; i < ListTypeFilter->count(); ++i)
  {
    if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
      sFilter.Append(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");
  }

  if (sFilter.IsEmpty())         // all filters enabled
    sFilter = m_sAllTypesFilter; // might be different for dialogs

  m_pFilter->SetTypeFilter(sFilter);
}

void xiiQtAssetBrowserWidget::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      break;
    case xiiAssetCuratorEvent::Type::AssetAdded:
    case xiiAssetCuratorEvent::Type::AssetRemoved:
      UpdateDirectoryTree();
      break;
    default:
      break;
  }
}

void xiiQtAssetBrowserWidget::UpdateDirectoryTree()
{
  xiiQtScopedBlockSignals block(TreeFolderFilter);

  if (TreeFolderFilter->topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, QLatin1String("<root>"));

    TreeFolderFilter->addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);
  }

  const xiiSet<xiiString>& Folders = xiiAssetCurator::GetSingleton()->GetAllAssetFolders();

  if (m_uiKnownAssetFolderCount == Folders.GetCount())
    return;

  m_uiKnownAssetFolderCount = Folders.GetCount();

  xiiStringBuilder tmp;

  for (const auto& sDir : Folders)
  {
    tmp = sDir;

    if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(tmp))
      continue;

    BuildDirectoryTree(tmp, TreeFolderFilter->topLevelItem(0), "", false);
  }

  TreeFolderFilter->setSortingEnabled(true);
  TreeFolderFilter->sortItems(0, Qt::SortOrder::AscendingOrder);
}


void xiiQtAssetBrowserWidget::ClearDirectoryTree()
{
  TreeFolderFilter->clear();
  m_uiKnownAssetFolderCount = 0;
}

void xiiQtAssetBrowserWidget::BuildDirectoryTree(const char* szCurPath, QTreeWidgetItem* pParent, const char* szCurPathToItem, bool bIsHidden)
{
  if (xiiStringUtils::IsNullOrEmpty(szCurPath))
    return;

  const char* szNextSep = xiiStringUtils::FindSubString(szCurPath, "/");

  QTreeWidgetItem* pNewParent = nullptr;

  xiiString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = szCurPath;
  else
    sFolderName = xiiStringView(szCurPath, szNextSep);

  if (sFolderName.EndsWith_NoCase("_data"))
  {
    bIsHidden = true;
  }

  xiiStringBuilder sCurPath = szCurPathToItem;
  sCurPath.AppendPath(sFolderName);

  const QString sQtFolderName = QString::fromUtf8(sFolderName.GetData());

  for (xiiInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  pNewParent = new QTreeWidgetItem();
  pNewParent->setText(0, sQtFolderName);
  pNewParent->setData(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath, QString::fromUtf8(sCurPath.GetData()));

  if (bIsHidden)
  {
    pNewParent->setForeground(0, QColor::fromRgba(qRgb(110, 110, 120)));
  }

  pParent->addChild(pNewParent);

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(szNextSep + 1, pNewParent, sCurPath, bIsHidden);
}

void xiiQtAssetBrowserWidget::on_TreeFolderFilter_itemSelectionChanged()
{
  if (m_bTreeSelectionChangeInProgress)
    return;

  xiiStringBuilder sCurPath;

  if (!TreeFolderFilter->selectedItems().isEmpty())
  {
    sCurPath = TreeFolderFilter->selectedItems()[0]->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  }

  m_pFilter->SetPathFilter(sCurPath);
}

void xiiQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (TreeFolderFilter->currentItem())
  {
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this, SLOT(OnTreeOpenExplorer()));
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in sub-folders"), this, SLOT(OnShowSubFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInSubFolders());
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
  }

  AddAssetCreatorMenu(&m, false);

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void xiiQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  xiiQtScopedBlockSignals block(TypeFilter);

  xiiStringBuilder sFilter;

  if (index > 0)
  {
    sFilter.Set(";", TypeFilter->itemData(index, Qt::UserRole).toString().toUtf8().data(), ";");
  }
  else
  {
    // all filters enabled
    // might be different for dialogs
    sFilter = m_sAllTypesFilter;
  }

  m_pFilter->SetTypeFilter(sFilter);
}

void xiiQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());
}

void xiiQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void xiiQtAssetBrowserWidget::OnTreeOpenExplorer()
{
  if (!TreeFolderFilter->currentItem())
    return;

  xiiStringBuilder sPath = TreeFolderFilter->currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  if (!xiiQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sPath, true))
    return;

  xiiQtUiServices::OpenInExplorer(sPath, false);
}

void xiiQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    if (!m_bDialogMode)
      m.setDefaultAction(m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Document"), this, SLOT(OnListOpenAssetDocument())));
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/AssetNeedsTransform16.png")), QLatin1String("Transform"), this, SLOT(OnTransform()));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentGuid16.png")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search16.png")), QLatin1String("Find all direct references to this asset"), this, [&]() { OnListFindAllReferences(false); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search16.png")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]() { OnListFindAllReferences(true); });
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut16.png")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
  }

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());

  AddAssetCreatorMenu(&m, true);

  m.exec(ListAssets->viewport()->mapToGlobal(pt));
}

void xiiQtAssetBrowserWidget::OnListOpenAssetDocument()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  for (auto& index : selection)
  {
    xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();

    if (guid.IsValid())
    {
      xiiAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }

    Q_EMIT ItemChosen(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}


void xiiQtAssetBrowserWidget::OnTransform()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedRows();

  xiiProgressRange range("Transforming Assets", 1 + selection.length(), true);

  for (auto& index : selection)
  {
    if (range.WasCanceled())
      break;

    xiiUuid guid  = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AssetGuid).value<xiiUuid>();
    QString sPath = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString();
    range.BeginNextStep(sPath.toUtf8());
    xiiTransformStatus res = xiiAssetCurator::GetSingleton()->TransformAsset(guid, xiiTransformFlags::TriggeredManually);
    if (res.Failed())
    {
      xiiLog::Error("{0} ({1})", res.m_sMessage, sPath.toUtf8().data());
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  xiiAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
}

void xiiQtAssetBrowserWidget::OnListOpenExplorer()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiString sPath = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  xiiQtUiServices::OpenInExplorer(sPath, true);
}

void xiiQtAssetBrowserWidget::OnListCopyAssetGuid()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiStringBuilder tmp;
  xiiUuid          guid = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData*  mimeData  = new QMimeData();
  mimeData->setText(xiiConversionUtils::ToString(guid, tmp).GetData());
  clipboard->setMimeData(mimeData);

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Copied asset GUID: {}", tmp), xiiTime::Seconds(5));
}

void xiiQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiStringBuilder sPath = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

  if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
    return;

  m_pFilter->SetPathFilter(sPath);
}

void xiiQtAssetBrowserWidget::OnListFindAllReferences(bool transitive)
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiUuid          guid = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
  xiiStringBuilder sAssetGuid;
  xiiConversionUtils::ToString(guid, sAssetGuid);

  xiiStringBuilder sFilter;
  sFilter.Format("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
  m_pFilter->SetTextFilter(sFilter);
  m_pFilter->SetPathFilter("");
}

void xiiQtAssetBrowserWidget::OnSelectionTimer()
{
  if (m_pModel->rowCount() == 1)
  {
    auto index = m_pModel->index(0, 0);

    ListAssets->selectionModel()->select(index, QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }
}


void xiiQtAssetBrowserWidget::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
    Q_EMIT  ItemSelected(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}

void xiiQtAssetBrowserWidget::OnAssetSelectionCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
  if (!ListAssets->selectionModel()->hasSelection())
  {
    Q_EMIT ItemCleared();
  }
  else if (ListAssets->selectionModel()->selectedIndexes().size() == 1)
  {
    QModelIndex index = ListAssets->selectionModel()->selectedIndexes()[0];

    xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
    Q_EMIT  ItemSelected(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  }
}


void xiiQtAssetBrowserWidget::OnModelReset()
{
  Q_EMIT ItemCleared();
}


void xiiQtAssetBrowserWidget::OnNewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  xiiAssetDocumentManager* pManager            = (xiiAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  xiiString                sAssetType          = pSender->property("AssetType").toString().toUtf8().data();
  xiiString                sTranslateAssetType = xiiTranslate(sAssetType);
  xiiString                sExtension          = pSender->property("Extension").toString().toUtf8().data();
  bool                     useSelection        = pSender->property("UseSelection").toBool();

  QString sStartDir = xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();

  // find path
  {
    if (TreeFolderFilter->currentItem())
    {
      xiiStringBuilder sPath = TreeFolderFilter->currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty() && xiiQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sPath, true))
        sStartDir = sPath.GetData();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      xiiString sPath = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty() && xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        xiiStringBuilder temp = sPath;
        sPath                 = temp.GetFileDirectory();

        sStartDir = sPath.GetData();
      }
    }
  }

  xiiStringBuilder title("Create ", sTranslateAssetType), sFilter;

  sFilter.Format("{0} (*.{1})", sTranslateAssetType, sExtension);

  QString          sSelectedFilter = sFilter.GetData();
  xiiStringBuilder sOutput         = QFileDialog::getSaveFileName(QApplication::activeWindow(), title.GetData(), sStartDir, sFilter.GetData(), &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

  if (sOutput.IsEmpty())
    return;

  xiiDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sOutput, pDoc, xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList).m_Result.Succeeded())
  {
    pDoc->EnsureVisible();
  }
}

void xiiQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
}

void xiiQtAssetBrowserWidget::OnPathFilterChanged()
{
  const QString sPath = QString::fromUtf8(m_pFilter->GetPathFilter());

  if (TreeFolderFilter->topLevelItemCount() == 1)
  {
    if (m_bTreeSelectionChangeInProgress)
      return;

    m_bTreeSelectionChangeInProgress = true;
    TreeFolderFilter->clearSelection();
    SelectPathFilter(TreeFolderFilter->topLevelItem(0), sPath);
    m_bTreeSelectionChangeInProgress = false;
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

bool xiiQtAssetBrowserWidget::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString() == sPath)
  {
    pParent->setSelected(true);
    return true;
  }

  for (xiiInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (SelectPathFilter(pParent->child(i), sPath))
    {
      pParent->setExpanded(true);
      return true;
    }
  }

  return false;
}

void xiiQtAssetBrowserWidget::SetSelectedAsset(xiiUuid preselectedAsset)
{
  if (!preselectedAsset.IsValid())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToItem", Qt::ConnectionType::QueuedConnection, Q_ARG(xiiUuid, preselectedAsset));
}

void xiiQtAssetBrowserWidget::OnScrollToItem(xiiUuid preselectedAsset)
{
  for (xiiInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    if (m_pModel->data(m_pModel->index(i, 0), xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(m_pModel->index(i, 0), QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(m_pModel->index(i, 0), QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void xiiQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(const char* szFilters)
{
  m_sAllTypesFilter.Clear();

  if (!xiiStringUtils::IsNullOrEmpty(szFilters))
  {
    xiiStringBuilder       sFilter;
    const xiiStringBuilder sAllFilters(";", szFilters, ";");

    m_sAllTypesFilter = sAllFilters;

    {
      xiiQtScopedBlockSignals block(ListTypeFilter);

      for (xiiInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        sFilter.Set(";", ListTypeFilter->item(i)->data(Qt::UserRole).toString().toUtf8().data(), ";");

        if (sAllFilters.FindSubString(sFilter) == nullptr)
        {
          ListTypeFilter->item(i)->setHidden(true);
        }
      }
    }

    {
      xiiQtScopedBlockSignals block(TypeFilter);

      for (xiiInt32 i = TypeFilter->count(); i > 1; --i)
      {
        const xiiInt32 idx = i - 1;

        sFilter.Set(";", TypeFilter->itemData(idx, Qt::UserRole).toString().toUtf8().data(), ";");

        if (sAllFilters.FindSubString(sFilter) == nullptr)
        {
          TypeFilter->removeItem(idx);
        }
      }
    }
  }

  m_pFilter->SetTypeFilter(m_sAllTypesFilter);
}
