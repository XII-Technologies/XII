#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <GuiFoundation/GuiFoundationDLL.h>
#include <QFile>

xiiQtAssetBrowserWidget::xiiQtAssetBrowserWidget(QWidget* pParent) :
  QWidget(pParent)
{
  setupUi(this);

  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();

  ButtonListMode->setVisible(false);
  ButtonIconMode->setVisible(false);
  ResetTypeFilter->setEnabled(false);

  m_pFilter = new xiiQtAssetBrowserFilter(this);
  m_pFilter->SetShowItemsInSubFolders(pPreferences->m_bAssetBrowserShowItemsInSubFolders);

  TreeFolderFilter->SetFilter(m_pFilter);

  m_pModel = new xiiQtAssetBrowserModel(this, m_pFilter);
  SearchWidget->setPlaceholderText("Search Assets");

  IconSizeSlider->setValue(50);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  ListAssets->setDragEnabled(true);
  ListAssets->setAcceptDrops(true);
  ListAssets->setDropIndicatorShown(true);
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

  ButtonShowItemsSubFolders->setEnabled(true);
  ButtonShowItemsSubFolders->setChecked(m_pFilter->GetShowItemsInSubFolders());
  XII_VERIFY(connect(ButtonShowItemsSubFolders, SIGNAL(toggled(bool)), this, SLOT(OnShowSubFolderItemsToggled())) != nullptr, "signal/slot connection failed");

  XII_VERIFY(connect(m_pFilter, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pFilter, SIGNAL(FilterChanged()), this, SLOT(OnFilterChanged())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pModel, SIGNAL(modelReset()), this, SLOT(OnModelReset())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pModel, &xiiQtAssetBrowserModel::editingFinished, this, &xiiQtAssetBrowserWidget::OnFileEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");

  XII_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(OnAssetSelectionChanged(const QItemSelection&, const QItemSelection&))) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(ListAssets->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnAssetSelectionCurrentChanged(const QModelIndex&, const QModelIndex&))) != nullptr, "signal/slot connection failed");
  connect(SearchWidget, &xiiQtSearchWidget::textChanged, this, &xiiQtAssetBrowserWidget::OnSearchWidgetTextChanged);

  UpdateAssetTypes();

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::AssetCuratorEventHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::ProjectEventHandler, this));

  setAcceptDrops(true);
}

xiiQtAssetBrowserWidget::~xiiQtAssetBrowserWidget()
{
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::ProjectEventHandler, this));
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserWidget::AssetCuratorEventHandler, this));

  ListAssets->setModel(nullptr);
}

void xiiQtAssetBrowserWidget::dragEnterEvent(QDragEnterEvent* pEvent)
{
  if (!pEvent->source())
    pEvent->acceptProposedAction();
}

void xiiQtAssetBrowserWidget::dragMoveEvent(QDragMoveEvent* pEvent)
{
  pEvent->acceptProposedAction();
}

void xiiQtAssetBrowserWidget::dragLeaveEvent(QDragLeaveEvent* pEvent)
{
  pEvent->accept();
}

static void CleanUpFiles(xiiArrayPtr<xiiString> files)
{
  for (const auto& file : files)
  {
    xiiOSFile::DeleteFile(file).IgnoreResult();
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }
}

void xiiQtAssetBrowserWidget::dropEvent(QDropEvent* pEvent)
{
  const QMimeData* mime = pEvent->mimeData();
  if (!mime->hasUrls())
  {
    pEvent->ignore();
  }

  pEvent->acceptProposedAction();

  xiiStringBuilder sTargetDir;

  if (TreeFolderFilter->currentItem() != nullptr)
  {
    sTargetDir = TreeFolderFilter->currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  }

  if (sTargetDir.IsEmpty())
  {
    xiiQtUiServices::MessageBoxInformation("Please first select a folder in the asset browser as the destination for the file import.");
    return;
  }

  QList<QUrl>                   urlList = mime->urls();
  xiiHybridArray<xiiString, 16> assetsToImport;

  // if we leave this function prematurely, delete all these temp files
  XII_SCOPE_EXIT(CleanUpFiles(assetsToImport));

  bool overWriteAll = false;
  for (qsizetype i = 0, count = qMin(urlList.size(), qsizetype(32)); i < count; ++i)
  {
    QUrl      url = urlList.at(i);
    QFileInfo fileinfo(url.toLocalFile());

    if (!fileinfo.exists())
      continue;

    // build source and destination paths info
    xiiStringBuilder srcPath = urlList.at(i).path().toUtf8().constData();
    srcPath.TrimWordStart("/"); // remove the "/" at the beginning of the source path

    xiiStringBuilder dstPath = sTargetDir;
    dstPath.AppendPath(fileinfo.fileName().toUtf8().constData());

    // Move the file/folder
    if (fileinfo.isDir())
    {
      if (!overWriteAll && xiiOSFile::ExistsDirectory(dstPath))
      {
        const auto res = xiiQtUiServices::MessageBoxQuestion(xiiFmt("This folder already exists:\n'{}'\n\nOverwrite existing files inside it?", dstPath), QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel);

        switch (res)
        {
          case QMessageBox::Yes:
            break;

          case QMessageBox::YesToAll:
            overWriteAll = true;
            break;

          case QMessageBox::Cancel:
          default:
            return;
        }
      }

      if (xiiOSFile::CopyFolder(srcPath, dstPath, &assetsToImport) != XII_SUCCESS)
      {
        xiiQtUiServices::MessageBoxWarning(xiiFmt("Failed to copy\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", srcPath, dstPath));
        return;
      }
    }
    else if (fileinfo.isFile())
    {
      if (!overWriteAll && xiiOSFile::ExistsFile(dstPath))
      {
        const auto res = xiiQtUiServices::MessageBoxQuestion(xiiFmt("This file already exists:\n'{}'\n\nOverwrite it?", dstPath), QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel, QMessageBox::Cancel);

        switch (res)
        {
          case QMessageBox::Yes:
            break;
          case QMessageBox::YesToAll:
            overWriteAll = true;
            break;
          case QMessageBox::Cancel:
          default:
            return;
        }
      }

      if (xiiOSFile::CopyFile(srcPath, dstPath) != XII_SUCCESS)
      {
        xiiQtUiServices::MessageBoxWarning(xiiFmt("Failed to copy\n\n'{}'\n\nto\n\n'{}'\n\nAborting operation.", srcPath, dstPath));
        return;
      }

      assetsToImport.PushBack(dstPath);
    }
  }

  for (const auto& file : assetsToImport)
  {
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(file);
  }

  QTimer::singleShot(1, this, [=]() {
    // return to the OS and import with a slight delay, otherwise the drop operation blocks the OS
    xiiAssetDocumentGenerator::ImportAssets(assetsToImport);
    //
  });

  // now that we've successfully imported the assets, clear this list so that the files don't get deleted
  assetsToImport.Clear();
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
    xiiQtScopedBlockSignals block(TypeFilter);

    TypeFilter->clear();

    if (m_Mode == Mode::Browser)
    {
      // '<All Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("<All Files>"));

      // '<Importable Files>' Filter
      TypeFilter->addItem(QIcon(QLatin1String(":/EditorFramework/Icons/ImportableFileType.svg")), QLatin1String("<Importable Files>"));
    }

    // '<All Assets>' Filter
    TypeFilter->addItem(QIcon(QLatin1String(":/AssetIcons/Icons/AllAssets.svg")), QLatin1String("<All Assets>"));

    for (const auto& it : assetTypes)
    {
      TypeFilter->addItem(xiiQtUiServices::GetCachedIconResource(it.Value()->m_sIcon, xiiColorScheme::GetCategoryColor(it.Value()->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::AssetMenuIcon)), QString::fromUtf8(it.Key(), it.Key().GetElementCount()));
      TypeFilter->setItemData(TypeFilter->count() - 1, QString::fromUtf8(it.Value()->m_sDocumentTypeName, it.Value()->m_sDocumentTypeName.GetElementCount()), Qt::UserRole);
    }
  }

  // make sure to apply the previously active type filter settings to the UI
  if (m_Mode == Mode::Browser)
  {
    xiiSet<xiiString> importExtensions;
    xiiAssetDocumentGenerator::GetSupportsFileTypes(importExtensions);
    m_pFilter->UpdateImportExtensions(importExtensions);
  }

  OnTypeFilterChanged();
}

void xiiQtAssetBrowserWidget::SetMode(Mode mode)
{
  if (m_Mode == mode)
    return;

  m_Mode = mode;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pToolbar->show();
      TreeFolderFilter->SetDialogMode(false);
      ListAssets->SetDialogMode(false);
      break;
    case Mode::FilePicker:
      TypeFilter->setVisible(false);
      ResetTypeFilter->setVisible(false);
      [[fallthrough]];
    case Mode::AssetPicker:
      m_pToolbar->hide();
      TreeFolderFilter->SetDialogMode(true);
      ListAssets->SetDialogMode(true);
      break;
  }

  UpdateAssetTypes();
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
      m_pFilter->Reset();
    }
    break;
    default:
      break;
  }
}

void xiiQtAssetBrowserWidget::AddAssetCreatorMenu(QMenu* pMenu, bool useSelectedAsset)
{
  if (m_Mode != Mode::Browser)
    return;

  const xiiHybridArray<xiiDocumentManager*, 16>& managers = xiiDocumentManager::GetAllDocumentManagers();

  xiiDynamicArray<const xiiDocumentTypeDescriptor*> documentTypes;

  QMenu* pSubMenu = pMenu->addMenu(QIcon(":/GuiFoundation/Icons/DocumentAdd.svg"), "New");

  xiiStringBuilder sTypeFilter = m_pFilter->GetTypeFilter();

  for (xiiDocumentManager* pMan : managers)
  {
    if (!pMan->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocumentManager>())
      continue;

    pMan->GetSupportedDocumentTypes(documentTypes);
  }

  documentTypes.Sort([](const xiiDocumentTypeDescriptor* a, const xiiDocumentTypeDescriptor* b) -> bool { return xiiTranslate(a->m_sDocumentTypeName).Compare(xiiTranslate(b->m_sDocumentTypeName)) < 0; });

  QAction* pAction = pSubMenu->addAction(xiiMakeQString(xiiTranslate("Folder")));
  pAction->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Folder.svg"));
  connect(pAction, &QAction::triggered, static_cast<eqQtAssetBrowserFolderView*>(TreeFolderFilter), &eqQtAssetBrowserFolderView::NewFolder);

  pSubMenu->addSeparator();

  for (const xiiDocumentTypeDescriptor* desc : documentTypes)
  {
    if (!desc->m_bCanCreate || desc->m_sFileExtension.IsEmpty())
      continue;

    QAction* pAction = pSubMenu->addAction(xiiMakeQString(xiiTranslate(desc->m_sDocumentTypeName)));
    pAction->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(desc->m_sIcon, xiiColorScheme::GetCategoryColor(desc->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::MenuEntryIcon)));
    pAction->setProperty("AssetType", desc->m_sDocumentTypeName.GetData());
    pAction->setProperty("AssetManager", QVariant::fromValue<void*>(desc->m_pManager));
    pAction->setProperty("Extension", desc->m_sFileExtension.GetData());
    pAction->setProperty("UseSelection", useSelectedAsset);

    connect(pAction, &QAction::triggered, this, &xiiQtAssetBrowserWidget::NewAsset);
  }
}

void xiiQtAssetBrowserWidget::AddImportedViaMenu(QMenu* pMenu)
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  // Find all uses
  xiiSet<xiiUuid> importedVia;
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(xiiQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (!bImportable)
      continue;

    xiiString sAbsPath = qtToXIIString(m_pModel->data(id, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
    xiiAssetCurator::GetSingleton()->FindAllUses(sAbsPath, importedVia);
  }

  // Sort by path
  xiiHybridArray<xiiUuid, 8> importedViaSorted;
  {
    importedViaSorted.Reserve(importedVia.GetCount());
    xiiAssetCurator::xiiLockedSubAssetTable allAssets = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
    for (const xiiUuid& guid : importedVia)
    {
      if (allAssets->Contains(guid))
        importedViaSorted.PushBack(guid);
    }

    importedViaSorted.Sort([&](const xiiUuid& a, const xiiUuid& b) -> bool { return allAssets->Find(a).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath().Compare(allAssets->Find(b).Value().m_pAssetInfo->m_Path.GetDataDirParentRelativePath()) < 0; });
  }

  if (importedViaSorted.IsEmpty())
    return;

  // Create actions to open
  QMenu* pSubMenu = pMenu->addMenu("Imported via");
  pSubMenu->setIcon(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")));

  for (const xiiUuid& guid : importedViaSorted)
  {
    const xiiAssetCurator::xiiLockedSubAsset pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
    QIcon                                    icon      = xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, xiiColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::OverlayIcon));
    QString                                  sRelPath  = xiiMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

    QAction* pAction = pSubMenu->addAction(sRelPath);
    pAction->setIcon(icon);
    pAction->setProperty("AbsPath", xiiMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath()));
    connect(pAction, &QAction::triggered, this, &xiiQtAssetBrowserWidget::OnOpenImportReferenceAsset);
  }
}

void xiiQtAssetBrowserWidget::GetSelectedImportableFiles(xiiDynamicArray<xiiString>& out_Files) const
{
  out_Files.Clear();

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const bool bImportable = id.data(xiiQtAssetBrowserModel::UserRoles::Importable).toBool();
    if (bImportable)
    {
      out_Files.PushBack(qtToXIIString(id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString()));
    }
  }
}

void xiiQtAssetBrowserWidget::on_ListAssets_clicked(const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void xiiQtAssetBrowserWidget::on_ListAssets_activated(const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

  Q_EMIT ItemSelected(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
}

void xiiQtAssetBrowserWidget::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
  const xiiUuid                               guid     = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();

  if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Asset | xiiAssetBrowserItemFlags::SubAsset))
  {
    if (guid.IsValid())
    {
      xiiAssetCurator::GetSingleton()->UpdateAssetLastAccessTime(guid);
    }
    Q_EMIT ItemChosen(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsSet(xiiAssetBrowserItemFlags::File))
  {
    Q_EMIT ItemChosen(xiiUuid::MakeInvalid(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
  else if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory))
  {
    m_pFilter->SetPathFilter(m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data());
  }
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

void xiiQtAssetBrowserWidget::on_ResetTypeFilter_clicked()
{
  switch (m_Mode)
  {
    case Mode::Browser:
      TypeFilter->setCurrentIndex(2);
      break;
    case Mode::AssetPicker:
    case Mode::FilePicker:
      TypeFilter->setCurrentIndex(0);
      break;
  }
}

void xiiQtAssetBrowserWidget::OnTextFilterChanged()
{
  OnFilterChanged();

  const QString sText = xiiMakeQString(m_pFilter->GetTextFilter());
  if (SearchWidget->text() != sText)
  {
    SearchWidget->setText(sText);
    QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
  }
}

void xiiQtAssetBrowserWidget::OnFilterChanged()
{
  const QString sText = xiiMakeQString(m_pFilter->GetTextFilter());
  ButtonShowItemsSubFolders->setEnabled(sText.isEmpty());
  ButtonShowItemsSubFolders->blockSignals(true);
  ButtonShowItemsSubFolders->setChecked(!sText.isEmpty() || m_pFilter->GetShowItemsInSubFolders());
  ButtonShowItemsSubFolders->blockSignals(false);
}

void xiiQtAssetBrowserWidget::OnTypeFilterChanged()
{
  xiiStringBuilder       sTemp;
  const xiiStringBuilder sFilter(";", m_pFilter->GetTypeFilter(), ";");

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

    if (iNumChecked == ((m_Mode != Mode::Browser) ? 1 : 3))
      TypeFilter->setCurrentIndex(iCheckedFilter);
    else
      TypeFilter->setCurrentIndex((m_Mode != Mode::Browser) ? 0 : 2); // "<All Assets>"

    int index = TypeFilter->currentIndex();

    switch (m_Mode)
    {
      case Mode::Browser:
        m_pFilter->SetShowNonImportableFiles(index == 0);
        m_pFilter->SetShowFiles(index == 0 || index == 1);
        break;
      case Mode::AssetPicker:
        m_pFilter->SetShowNonImportableFiles(false);
        m_pFilter->SetShowFiles(false);
        break;
      case Mode::FilePicker:
        m_pFilter->SetShowNonImportableFiles(true);
        m_pFilter->SetShowFiles(true);
        break;
    }
  }

  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void xiiQtAssetBrowserWidget::OnPathFilterChanged()
{
  QTimer::singleShot(0, this, SLOT(OnSelectionTimer()));
}

void xiiQtAssetBrowserWidget::OnSearchWidgetTextChanged(const QString& text)
{
  m_pFilter->SetTextFilter(text.toUtf8().data());
}

void xiiQtAssetBrowserWidget::keyPressEvent(QKeyEvent* e)
{
  QWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && m_Mode == Mode::Browser)
  {
    e->accept();
    DeleteSelection();
    return;
  }

  if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
  {
    e->accept();
    OnListOpenAssetDocument();
    return;
  }
}

void xiiQtAssetBrowserWidget::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->accept();
    xiiStringBuilder sPath = m_pFilter->GetPathFilter();
    if (sPath.IsEmpty())
      return;
    sPath.PathParentDirectory();
    sPath.Trim("/");

    m_pFilter->SetPathFilter(sPath);
    return;
  }

  QWidget::mousePressEvent(e);
}

void xiiQtAssetBrowserWidget::RenameCurrent()
{
  m_bOpenAfterRename = false;

  if (ListAssets->currentIndex().isValid())
  {
    ListAssets->edit(ListAssets->currentIndex());
  }
}

void xiiQtAssetBrowserWidget::DeleteSelection()
{
  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)id.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (itemType.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory))
    {
      xiiQtUiServices::MessageBoxWarning(xiiFmt("Sub-assets and data directories can't be deleted."));
      return;
    }
  }

  QMessageBox::StandardButton choice = xiiQtUiServices::MessageBoxQuestion(xiiFmt("Delete the selected file?\n\nThis operation cannot be undone."), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
  if (choice == QMessageBox::StandardButton::Cancel)
    return;

  for (const QModelIndex& id : selection)
  {
    const xiiBitflags<xiiAssetBrowserItemFlags> itemType   = (xiiAssetBrowserItemFlags::Enum)id.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    QString                                     sQtAbsPath = id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    xiiString                                   sAbsPath   = qtToXIIString(sQtAbsPath);

    if (itemType.IsSet(xiiAssetBrowserItemFlags::File))
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        xiiLog::Error("Failed to delete file '{}'", sAbsPath);
      }
    }
    else
    {
      if (!QFile::moveToTrash(sQtAbsPath))
      {
        xiiLog::Error("Failed to delete folder '{}'", sAbsPath);
      }
    }
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
}

void xiiQtAssetBrowserWidget::OnImportAsAboutToShow()
{
  QMenu* pMenu = qobject_cast<QMenu*>(sender());

  if (!pMenu->actions().isEmpty())
    return;

  xiiHybridArray<xiiString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  xiiSet<xiiString> extensions;
  xiiStringBuilder  sExt;
  for (const auto& file : filesToImport)
  {
    sExt = file.GetFileExtension();
    sExt.ToLower();
    extensions.Insert(sExt);
  }

  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  xiiAssetDocumentGenerator::CreateGenerators(generators);
  xiiHybridArray<xiiAssetDocumentGenerator::ImportMode, 16> importModes;

  for (xiiAssetDocumentGenerator* pGen : generators)
  {
    for (xiiStringView ext : extensions)
    {
      if (pGen->SupportsFileType(ext))
      {
        pGen->GetImportModes({}, importModes);
        break;
      }
    }
  }

  for (const auto& mode : importModes)
  {
    QAction* act = pMenu->addAction(QIcon(xiiMakeQString(mode.m_sIcon)), xiiMakeQString(xiiTranslate(mode.m_sName)));
    act->setData(xiiMakeQString(mode.m_sName));
    connect(act, &QAction::triggered, this, &xiiQtAssetBrowserWidget::OnImportAsClicked);
  }

  xiiAssetDocumentGenerator::DestroyGenerators(generators);
}

void xiiQtAssetBrowserWidget::OnImportAsClicked()
{
  xiiHybridArray<xiiString, 8> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  QAction*  act   = qobject_cast<QAction*>(sender());
  xiiString sMode = qtToXIIString(act->data().toString());

  xiiHybridArray<xiiAssetDocumentGenerator*, 16> generators;
  xiiAssetDocumentGenerator::CreateGenerators(generators);

  xiiHybridArray<xiiAssetDocumentGenerator::ImportMode, 16> importModes;
  for (xiiAssetDocumentGenerator* pGen : generators)
  {
    importModes.Clear();
    pGen->GetImportModes({}, importModes);

    for (const auto& mode : importModes)
    {
      if (mode.m_sName == sMode)
      {
        for (const xiiString& file : filesToImport)
        {
          if (pGen->SupportsFileType(file))
          {
            pGen->Import(file, sMode, false).LogFailure();
          }
        }

        goto done;
      }
    }
  }

done:
  xiiAssetDocumentGenerator::DestroyGenerators(generators);
}

void xiiQtAssetBrowserWidget::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAssetCuratorEvent::Type::AssetListReset:
      UpdateAssetTypes();
      break;
    default:
      break;
  }
}

void xiiQtAssetBrowserWidget::on_TreeFolderFilter_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  const bool bClickedValid = TreeFolderFilter->indexAt(pt).isValid();

  if (bClickedValid)
  {
    const bool bIsRoot = TreeFolderFilter->currentItem() && TreeFolderFilter->currentItem() == TreeFolderFilter->topLevelItem(0);
    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), TreeFolderFilter, SLOT(TreeOpenExplorer()));
    }

    if (TreeFolderFilter->currentItem() && !bIsRoot)
    {
      // Delete
      const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)TreeFolderFilter->currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      QAction*                                    pDelete  = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), TreeFolderFilter, &eqQtAssetBrowserFolderView::DeleteFolder);
      if (itemType.IsSet(xiiAssetBrowserItemFlags::DataDirectory))
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Data directories can't be deleted.");
      }

      // Create
      AddAssetCreatorMenu(&m, false);
    }

    m.addSeparator();
  }

  {
    QAction* pAction = m.addAction(QLatin1String("Show Items in hidden folders"), this, SLOT(OnShowHiddenFolderItemsToggled()));
    pAction->setCheckable(true);
    pAction->setChecked(m_pFilter->GetShowItemsInHiddenFolders());
    pAction->setToolTip("Whether to ignore '_data' folders when showing items in sub-folders is enabled.");
  }

  {
    QAction* pAction = m.addAction(QIcon(":/GuiFoundation/Icons/SaveAll.svg"), QLatin1String("Re-save Assets in Folder"), this, SLOT(OnResaveAssets()));
    pAction->setToolTip("Opens every document and saves it. Used to get all documents to the latest version.");
  }

  m.exec(TreeFolderFilter->viewport()->mapToGlobal(pt));
}

void xiiQtAssetBrowserWidget::on_TypeFilter_currentIndexChanged(int index)
{
  xiiQtScopedBlockSignals block(TypeFilter);

  xiiStringBuilder sFilter;

  switch (m_Mode)
  {
    case Mode::Browser:
      m_pFilter->SetShowNonImportableFiles(index == 0);
      m_pFilter->SetShowFiles(index == 0 || index == 1);
      ResetTypeFilter->setEnabled(index != 2);
      break;
    case Mode::AssetPicker:
      m_pFilter->SetShowNonImportableFiles(false);
      m_pFilter->SetShowFiles(false);
      ResetTypeFilter->setEnabled(index != 0);
      break;
    case Mode::FilePicker:
      m_pFilter->SetShowNonImportableFiles(true);
      m_pFilter->SetShowFiles(true);
      ResetTypeFilter->setEnabled(false);
      break;
  }


  switch (index)
  {
    case 0:
    case 1:
    case 2:
      // all filters enabled
      // might be different for dialogs
      sFilter = m_sAllTypesFilter;
      break;

    default:
      sFilter.Set(";", TypeFilter->itemData(index, Qt::UserRole).toString().toUtf8().data(), ";");
      break;
  }

  m_pFilter->SetTypeFilter(sFilter);
}

void xiiQtAssetBrowserWidget::OnShowSubFolderItemsToggled()
{
  m_pFilter->SetShowItemsInSubFolders(!m_pFilter->GetShowItemsInSubFolders());

  xiiEditorPreferencesUser* pPreferences             = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  pPreferences->m_bAssetBrowserShowItemsInSubFolders = m_pFilter->GetShowItemsInSubFolders();
}

void xiiQtAssetBrowserWidget::OnShowHiddenFolderItemsToggled()
{
  m_pFilter->SetShowItemsInHiddenFolders(!m_pFilter->GetShowItemsInHiddenFolders());
}

void xiiQtAssetBrowserWidget::OnResaveAssets()
{
  if (QTreeWidgetItem* pCurrentItem = TreeFolderFilter->currentItem())
  {
    QModelIndex      id       = TreeFolderFilter->indexFromItem(pCurrentItem);
    xiiStringBuilder sAbsPath = qtToXIIString(id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString());

    xiiAssetCurator::GetSingleton()->ResaveAllAssets(sAbsPath);
  }
}

void xiiQtAssetBrowserWidget::on_ListAssets_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  if (ListAssets->selectionModel()->hasSelection())
  {
    bool bShowDocumentActions = false;

    if (m_Mode == Mode::Browser)
    {
      QString sTitle = "Open Selection";
      QIcon   icon   = QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg"));

      bool bShowOpenWith = false;

      if (ListAssets->selectionModel()->selectedIndexes().count() == 1)
      {
        const QModelIndex                           firstItem = ListAssets->selectionModel()->selectedIndexes()[0];
        const xiiBitflags<xiiAssetBrowserItemFlags> itemType  = (xiiAssetBrowserItemFlags::Enum)firstItem.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
        if (itemType.IsAnySet(xiiAssetBrowserItemFlags::Asset | xiiAssetBrowserItemFlags::SubAsset))
        {
          sTitle               = "Open Document";
          bShowDocumentActions = true;
        }
        else if (itemType.IsSet(xiiAssetBrowserItemFlags::File))
        {
          sTitle        = "Open File";
          bShowOpenWith = true;
        }
        else if (itemType.IsAnySet(xiiAssetBrowserItemFlags::DataDirectory | xiiAssetBrowserItemFlags::Folder))
        {
          sTitle = "Enter Folder";
          icon   = QIcon(QLatin1String(":/EditorFramework/Icons/Folder.svg"));
        }
      }
      m.setDefaultAction(m.addAction(icon, sTitle, this, SLOT(OnListOpenAssetDocument())));

      if (bShowOpenWith)
      {
        m.addAction(icon, "Open With...", this, SLOT(OnListOpenFileWith()));
      }
    }
    else
      m.setDefaultAction(m.addAction(QLatin1String("Select"), this, SLOT(OnListOpenAssetDocument())));

    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/ZoomOut.svg")), QLatin1String("Filter to this Path"), this, SLOT(OnFilterToThisPath()));
    m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnListOpenExplorer()));

    if (bShowDocumentActions)
    {
      m.addAction(QIcon(QLatin1String(":/EditorFramework/Icons/TransformAsset.svg")), QLatin1String("Transform"), this, SLOT(OnTransform()));
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnListCopyAssetGuid()));

      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct references to this asset"), this, [&]() { OnListFindAllReferences(false); });
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Search.svg")), QLatin1String("Find all direct and indirect references to this asset"), this, [&]() { OnListFindAllReferences(true); });
    }
  }


  if (m_Mode == Mode::Browser && ListAssets->selectionModel()->hasSelection())
  {
    QModelIndexList selection   = ListAssets->selectionModel()->selectedIndexes();
    bool            bImportable = false;
    bool            bAllFiles   = true;
    for (const QModelIndex& id : selection)
    {
      bImportable |= id.data(xiiQtAssetBrowserModel::UserRoles::Importable).toBool();

      const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)id.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory))
      {
        bAllFiles = false;
      }
    }

    // Rename
    {
      bool bCanRename = true;

      QModelIndex id = ListAssets->currentIndex();

      const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)id.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
      if (itemType.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory))
      {
        bCanRename = false;
      }

      QAction* pRename = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Rename.svg")), QLatin1String("Rename"), this, SLOT(RenameCurrent()));
      pRename->setShortcut(QKeySequence("F2"));
      if (!bCanRename)
      {
        pRename->setEnabled(false);
        pRename->setToolTip("Sub-assets and data directories can't be renamed.");
      }
    }

    // Delete
    {
      QAction* pDelete = m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Delete.svg")), QLatin1String("Delete"), this, SLOT(DeleteSelection()));
      pDelete->setShortcut(QKeySequence("Del"));
      if (!bAllFiles)
      {
        pDelete->setEnabled(false);
        pDelete->setToolTip("Sub-assets and data directories can't be deleted.");
      }
    }

    // Import assets
    if (bImportable)
    {
      m.addSeparator();
      m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), QLatin1String("Import..."), this, SLOT(ImportSelection()));
      QMenu* imp = m.addMenu(QIcon(QLatin1String(":/GuiFoundation/Icons/Import.svg")), "Import As");
      connect(imp, &QMenu::aboutToShow, this, &xiiQtAssetBrowserWidget::OnImportAsAboutToShow);
      AddImportedViaMenu(&m);
    }
  }

  m.addSeparator();

  auto pSortAction = m.addAction(QLatin1String("Sort by Recently Used"), this, SLOT(OnListToggleSortByRecentlyUsed()));
  pSortAction->setCheckable(true);
  pSortAction->setChecked(m_pFilter->GetSortByRecentUse());

  m.addSeparator();
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
    // Only enter folders on a single selection. Otherwise the results are undefined.
    const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    if (selection.count() > 1 && itemType.IsAnySet(xiiAssetBrowserItemFlags::DataDirectory | xiiAssetBrowserItemFlags::Folder))
      continue;
    on_ListAssets_doubleClicked(index);
  }
}

void xiiQtAssetBrowserWidget::OnListOpenFileWith()
{
  if (!ListAssets->currentIndex().isValid())
    return;

  xiiString sPath = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

  xiiQtUiServices::OpenWith(sPath);
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

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Copied asset GUID: {}", tmp), xiiTime::MakeFromSeconds(5));
}

void xiiQtAssetBrowserWidget::OnFilterToThisPath()
{
  if (!ListAssets->selectionModel()->hasSelection())
    return;

  xiiStringBuilder sPath = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  sPath.PathParentDirectory();
  sPath.Trim("/");

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
  sFilter.SetFormat("{}:{}", transitive ? "ref-all" : "ref", sAssetGuid);
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

    const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
    Q_EMIT  ItemSelected(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
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

    const xiiBitflags<xiiAssetBrowserItemFlags> itemType = (xiiAssetBrowserItemFlags::Enum)index.data(xiiQtAssetBrowserModel::UserRoles::ItemFlags).toInt();

    xiiUuid guid = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>();
    Q_EMIT  ItemSelected(guid, m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString(), m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString(), itemType.GetValue());
  }
}

void xiiQtAssetBrowserWidget::OnModelReset()
{
  Q_EMIT ItemCleared();
}

void xiiQtAssetBrowserWidget::NewAsset()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  xiiAssetDocumentManager* pManager       = (xiiAssetDocumentManager*)pSender->property("AssetManager").value<void*>();
  xiiString                sAssetType     = pSender->property("AssetType").toString().toUtf8().data();
  xiiString                sStartFileName = xiiTranslate(sAssetType);
  xiiString                sExtension     = pSender->property("Extension").toString().toUtf8().data();
  bool                     useSelection   = pSender->property("UseSelection").toBool();

  QString sStartDir;

  // find path
  {
    if (TreeFolderFilter->selectionModel()->hasSelection())
    {
      auto idx  = TreeFolderFilter->selectionModel()->selection().indexes()[0];
      sStartDir = TreeFolderFilter->itemFromIndex(idx)->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
    }

    // this will take precedence
    if (useSelection && ListAssets->selectionModel()->hasSelection())
    {
      xiiString sPath = m_pModel->data(ListAssets->currentIndex(), xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();

      if (!sPath.IsEmpty())
      {
        xiiStringBuilder temp = sPath;
        sPath                 = temp.GetFileDirectory();

        sStartDir = sPath.GetData();

        sStartFileName = temp.GetFileName();
      }
    }
  }

  if (sStartDir.isEmpty())
  {
    // this happens when the root node is selected
    sStartDir = xiiToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  }

  xiiStringBuilder sNewAsset = qtToXIIString(sStartDir);
  xiiStringBuilder sBaseFileName;
  xiiPathUtils::MakeValidFilename(sStartFileName, ' ', sBaseFileName);
  sNewAsset.AppendFormat("/{}.{}", sBaseFileName, sExtension);

  for (xiiUInt32 i = 2; xiiOSFile::ExistsFile(sNewAsset); i++)
  {
    sNewAsset = qtToXIIString(sStartDir);
    sNewAsset.AppendFormat("/{}{}.{}", sBaseFileName, i, sExtension);
  }

  sNewAsset.MakeCleanPath();

  xiiDocument* pDoc;
  if (pManager->CreateDocument(sAssetType, sNewAsset, pDoc, xiiDocumentFlags::Default).m_Result.Failed())
  {
    xiiLog::Error("Failed to create document: {}", sNewAsset);
    return;
  }

  {
    xiiStringBuilder sRelativePath = sNewAsset;
    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelativePath))
    {
      m_pFilter->SetTemporaryPinnedItem(sRelativePath);
    }
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewAsset);
    m_pModel->OnFileSystemUpdate();
  }

  xiiInt32 iNewIndex = m_pModel->FindIndex(sNewAsset);
  if (iNewIndex != -1)
  {
    m_bOpenAfterRename = true;
    QModelIndex idx    = m_pModel->index(iNewIndex, 0);
    ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
    ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    ListAssets->edit(idx);
  }
}

void xiiQtAssetBrowserWidget::OnFileEditingFinished(const QString& sAbsPath, const QString& sNewName, bool bIsAsset)
{
  xiiStringBuilder sOldPath = qtToXIIString(sAbsPath);
  xiiStringBuilder sNewPath = sOldPath;
  sNewPath.ChangeFileName(qtToXIIString(sNewName));

  if (sOldPath != sNewPath)
  {
    if (xiiOSFile::MoveFileOrDirectory(sOldPath, sNewPath).Failed())
    {
      xiiLog::Error("Failed to rename '{}' to '{}'", sOldPath, sNewPath);
      return;
    }

    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sOldPath);

    // If we have a temporary item, make sure that any renames ensure that the item is still set as the new temporary
    // A common case is: a type filter is active that excludes a newly created asset. Thus, on creation the new asset is set as the pinned item. Editing of the item is started and the user gives it a new name and we end up here. We want the item to remain pinned.
    if (!m_pFilter->GetTemporaryPinnedItem().IsEmpty())
    {
      xiiStringBuilder sOldRelativePath = sOldPath;
      xiiStringBuilder sNewRelativePath = sNewPath;
      if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sOldRelativePath) && xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewRelativePath))
      {
        if (sOldRelativePath == m_pFilter->GetTemporaryPinnedItem())
        {
          m_pFilter->SetTemporaryPinnedItem(sNewRelativePath);
        }
      }
    }
    m_pModel->OnFileSystemUpdate();

    // it is necessary to flush the events queued on the main thread, otherwise opening the asset may not work as intended
    xiiAssetCurator::GetSingleton()->MainThreadTick(true);

    xiiInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
    }
  }

  if (m_bOpenAfterRename)
  {
    m_bOpenAfterRename = false;

    xiiInt32 iNewIndex = m_pModel->FindIndex(sNewPath);
    if (iNewIndex != -1)
    {
      QModelIndex idx = m_pModel->index(iNewIndex, 0);
      on_ListAssets_doubleClicked(idx);
    }
  }
}

void xiiQtAssetBrowserWidget::ImportSelection()
{
  xiiHybridArray<xiiString, 4> filesToImport;
  GetSelectedImportableFiles(filesToImport);

  if (filesToImport.IsEmpty())
    return;

  xiiAssetDocumentGenerator::ImportAssets(filesToImport);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();
  for (const QModelIndex& id : selection)
  {
    Q_EMIT m_pModel->dataChanged(id, id);
  }
}

void xiiQtAssetBrowserWidget::OnOpenImportReferenceAsset()
{
  QAction*  pSender  = qobject_cast<QAction*>(sender());
  xiiString sAbsPath = qtToXIIString(pSender->property("AbsPath").toString());

  xiiQtEditorApp::GetSingleton()->OpenDocument(sAbsPath, xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList);
}

void xiiQtAssetBrowserWidget::OnListToggleSortByRecentlyUsed()
{
  m_pFilter->SetSortByRecentUse(!m_pFilter->GetSortByRecentUse());
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

void xiiQtAssetBrowserWidget::SetSelectedFile(xiiStringView sAbsPath)
{
  if (sAbsPath.IsEmpty())
    return;

  // cannot do this immediately, since the UI is probably still building up
  // ListAssets->scrollTo either hangs, or has no effect
  // so we put this into the message queue, and do it later
  QMetaObject::invokeMethod(this, "OnScrollToFile", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, xiiMakeQString(sAbsPath)));
}

void xiiQtAssetBrowserWidget::OnScrollToItem(xiiUuid preselectedAsset)
{
  const xiiAssetCurator::xiiLockedSubAsset pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(preselectedAsset);
  if (pSubAsset.isValid())
  {
    xiiStringBuilder sPath = xiiMakeQString(pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath()).toUtf8().data();
    sPath.PathParentDirectory();
    sPath.Trim("/");
    m_pFilter->SetPathFilter(sPath);
    m_pFilter->SetTextFilter("");
  }

  for (xiiInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    QModelIndex idx = m_pModel->index(i, 0);
    if (m_pModel->data(idx, xiiQtAssetBrowserModel::UserRoles::SubAssetGuid).value<xiiUuid>() == preselectedAsset)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void xiiQtAssetBrowserWidget::OnScrollToFile(QString sPreselectedFile)
{
  xiiStringBuilder sPath = sPreselectedFile.toUtf8().data();
  if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
  {
    sPath.PathParentDirectory();
    sPath.Trim("/");
    m_pFilter->SetPathFilter(sPath);
  }

  for (xiiInt32 i = 0; i < m_pModel->rowCount(); ++i)
  {
    QModelIndex idx = m_pModel->index(i, 0);

    if (m_pModel->data(idx, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).value<QString>() == sPreselectedFile)
    {
      ListAssets->selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectionFlag::ClearAndSelect);
      ListAssets->scrollTo(idx, QAbstractItemView::ScrollHint::EnsureVisible);
      return;
    }
  }

  raise();
}

void xiiQtAssetBrowserWidget::ShowOnlyTheseTypeFilters(xiiStringView sFilters)
{
  m_sAllTypesFilter.Clear();

  if (!sFilters.IsEmpty())
  {
    xiiStringBuilder       sFilter;
    const xiiStringBuilder sAllFilters(";", sFilters, ";");

    m_sAllTypesFilter = sAllFilters;

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

void xiiQtAssetBrowserWidget::UseFileExtensionFilters(xiiStringView sFileExtensions)
{
  m_pFilter->SetFileExtensionFilters(sFileExtensions);
}

void xiiQtAssetBrowserWidget::SetRequiredTag(xiiStringView sRequiredTag)
{
  m_pFilter->SetRequiredTag(sRequiredTag);
}
