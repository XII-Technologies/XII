/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <QFileInfo>
#include <QToolTip>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

xiiFileNameValidator::xiiFileNameValidator(QObject* pParent, xiiStringView sParentFolder, xiiStringView sCurrentName) :
  QValidator(pParent), m_sParentFolder(sParentFolder), m_sCurrentName(sCurrentName)
{
}

QValidator::State xiiFileNameValidator::validate(QString& ref_sInput, int& ref_iPos) const
{
  xiiStringBuilder sTemp = ref_sInput.toUtf8().constData();
  if (sTemp.IsEmpty())
    return QValidator::State::Intermediate;
  if (xiiPathUtils::ContainsInvalidFilenameChars(sTemp))
    return QValidator::State::Invalid;
  if (sTemp.StartsWith_NoCase(" ") || sTemp.EndsWith(" "))
    return QValidator::State::Intermediate;

  if (!m_sCurrentName.IsEmpty() && sTemp == m_sCurrentName)
    return QValidator::State::Acceptable;
  if (!m_sCurrentName.IsEmpty() && sTemp == m_sCurrentName.GetFileName())
    return QValidator::State::Acceptable;

  xiiStringBuilder sAbsPath = m_sParentFolder;
  sAbsPath.AppendPath(sTemp);
  sAbsPath.Append(".", m_sCurrentName.GetFileExtension());

  if (xiiOSFile::ExistsDirectory(sAbsPath) || xiiOSFile::ExistsFile(sAbsPath))
    return QValidator::State::Intermediate;

  return QValidator::State::Acceptable;
}

xiiFolderNameDelegate::xiiFolderNameDelegate(QObject* pParent /*= nullptr*/) :
  QItemDelegate(pParent)
{
}

QWidget* xiiFolderNameDelegate::createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  xiiStringBuilder sAbsPath = index.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().constData();

  QLineEdit* editor = new QLineEdit(pParent);
  editor->setValidator(new xiiFileNameValidator(editor, sAbsPath.GetFileDirectory(), sAbsPath.GetFileNameAndExtension()));
  return editor;
}

void xiiFolderNameDelegate::setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const
{
  QString    sOldName  = index.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
  QLineEdit* pLineEdit = qobject_cast<QLineEdit*>(pEditor);
  emit       editingFinished(sOldName, pLineEdit->text());
}

eqQtAssetBrowserFolderView::eqQtAssetBrowserFolderView(QWidget* pParent) :
  QTreeWidget(pParent)
{
  viewport()->setAcceptDrops(true);
  setAcceptDrops(true);

  setDropIndicatorShown(true);
  setDefaultDropAction(Qt::MoveAction);

  SetDialogMode(false);

  setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  auto pDelegate = new xiiFolderNameDelegate(this);
  XII_VERIFY(connect(pDelegate, &xiiFolderNameDelegate::editingFinished, this, &eqQtAssetBrowserFolderView::OnFolderEditingFinished, Qt::QueuedConnection), "signal/slot connection failed");
  setItemDelegate(pDelegate);

  xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(xiiMakeDelegate(&eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler, this));
  xiiToolsProject::s_Events.AddEventHandler(xiiMakeDelegate(&eqQtAssetBrowserFolderView::ProjectEventHandler, this));

  XII_VERIFY(connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(OnItemSelectionChanged())) != nullptr, "signal/slot connection failed");

  UpdateDirectoryTree();
}

eqQtAssetBrowserFolderView::~eqQtAssetBrowserFolderView()
{
  xiiToolsProject::s_Events.RemoveEventHandler(xiiMakeDelegate(&eqQtAssetBrowserFolderView::ProjectEventHandler, this));
  xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(xiiMakeDelegate(&eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler, this));
}

void eqQtAssetBrowserFolderView::SetFilter(xiiQtAssetBrowserFilter* pFilter)
{
  m_pFilter = pFilter;
  XII_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(OnPathFilterChanged())) != nullptr, "signal/slot connection failed");
}

void eqQtAssetBrowserFolderView::SetDialogMode(bool bDialogMode)
{
  m_bDialogMode = bDialogMode;

  if (m_bDialogMode)
  {
    setDragEnabled(false);
    setDragDropMode(QAbstractItemView::DragDropMode::NoDragDrop);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
  }
  else
  {
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
  }
}

void eqQtAssetBrowserFolderView::NewFolder()
{
  QAction* pSender = qobject_cast<QAction*>(sender());

  if (!currentItem())
    return;

  xiiStringBuilder sPath      = currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  xiiStringBuilder sNewFolder = sPath;
  sNewFolder.AppendFormat("/NewFolder");

  for (xiiUInt32 i = 2; xiiOSFile::ExistsDirectory(sNewFolder); i++)
  {
    sNewFolder = sPath;
    sNewFolder.AppendFormat("/NewFolder{}", i);
  }

  if (xiiFileSystem::CreateDirectoryStructure(sNewFolder).Succeeded())
  {
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewFolder);
    OnFlushFileSystemEvents();

    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewFolder))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewFolder, topLevelItem(0), {});
      if (pItem)
      {
        m_bTreeSelectionChangeInProgress = true;
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        setCurrentItem(pItem);
        m_bTreeSelectionChangeInProgress = false;
        OnItemSelectionChanged(); // make sure the path filter is set to the new folder
        editItem(pItem);
      }
    }
  }
}

void eqQtAssetBrowserFolderView::OnFolderEditingFinished(const QString& sAbsPath, const QString& sNewName)
{
  xiiStringBuilder sPath    = sAbsPath.toUtf8().data();
  xiiStringBuilder sNewPath = sPath;
  sNewPath.ChangeFileNameAndExtension(sNewName.toUtf8().data());

  if (sPath != sNewPath)
  {
    if (xiiOSFile::MoveFileOrDirectory(sPath, sNewPath).Failed())
    {
      xiiLog::Error("Failed to rename '{}' to '{}'", sPath, sNewPath);
      return;
    }

    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewPath);
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sPath);
    OnFlushFileSystemEvents();

    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewPath))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewPath, topLevelItem(0), {});
      if (pItem)
      {
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        topLevelItem(0)->setSelected(false);
        setCurrentItem(pItem);
      }
    }
  }
}

void eqQtAssetBrowserFolderView::FileSystemModelFolderEventHandler(const xiiFolderChangedEvent& e)
{
  XII_LOCK(m_FolderStructureMutex);
  m_QueuedFolderEvents.PushBack(e);

  QTimer::singleShot(0, this, SLOT(OnFlushFileSystemEvents()));
}

void eqQtAssetBrowserFolderView::ProjectEventHandler(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectClosed:
    {
      // remove project structure from asset browser
      ClearDirectoryTree();
    }
    break;
    default:
      break;
  }
}

void eqQtAssetBrowserFolderView::dragMoveEvent(QDragMoveEvent* e)
{
  QTreeWidget::dragMoveEvent(e);

  xiiHybridArray<xiiString, 1> files;
  xiiString                    sTarget;
  xiiStatus                    res = canDrop(e, files, sTarget);
  if (res.Failed())
  {
    QToolTip::showText(viewport()->mapToGlobal(e->position().toPoint()), xiiMakeQString(res.GetMessageString().GetView()), this);
    e->ignore();
  }
  else
  {
    QToolTip::hideText();
  }
}

void eqQtAssetBrowserFolderView::mouseMoveEvent(QMouseEvent* e)
{
  // Only allow dragging with left mouse button.
  if (state() == DraggingState && !e->buttons().testFlag(Qt::MouseButton::LeftButton))
    return;

  QTreeWidget::mouseMoveEvent(e);
}

xiiStatus eqQtAssetBrowserFolderView::canDrop(QDropEvent* e, xiiDynamicArray<xiiString>& out_files, xiiString& out_sTargetFolder)
{
  if (!e->mimeData()->hasFormat("application/xiiEditor.files"))
  {
    return xiiStatus(XII_FAILURE);
  }

  DropIndicatorPosition dropIndicator = dropIndicatorPosition();
  if (dropIndicator != QAbstractItemView::OnItem)
  {
    return xiiStatus(XII_FAILURE);
  }

  auto action = e->dropAction();
  if (action != Qt::MoveAction)
  {
    return xiiStatus(XII_FAILURE);
  }

  out_files.Clear();
  QByteArray                 encodedData = e->mimeData()->data("application/xiiEditor.files");
  QDataStream                stream(&encodedData, QIODevice::ReadOnly);
  xiiHybridArray<QString, 1> files;
  stream >> files;

  QModelIndex dropIndex = indexAt(e->position().toPoint());
  if (dropIndex.isValid())
  {
    QString sAbsTarget = dropIndex.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    out_sTargetFolder  = qtToXIIString(sAbsTarget);

    for (const QString& sFile : files)
    {
      xiiString sFileToMove = qtToXIIString(sFile);
      out_files.PushBack(sFileToMove);

      if (xiiPathUtils::IsSubPath(sFileToMove, out_sTargetFolder))
      {
        return xiiStatus(xiiFmt("Can't move '{}' into its own sub-folder '{}'", sFileToMove, out_sTargetFolder));
      }
    }
  }

  return XII_SUCCESS;
}

void eqQtAssetBrowserFolderView::dropEvent(QDropEvent* e)
{
  QToolTip::hideText();
  xiiHybridArray<xiiString, 1> files;
  xiiString                    sTargetFolder;
  // Always accept and call base class to end the drop operation as a no-op in the base class.
  e->accept();
  QTreeWidget::dropEvent(e);
  if (canDrop(e, files, sTargetFolder).Failed())
    return;

  if (!MoveContentPaths(files, sTargetFolder))
    return;

  xiiStringBuilder sNewLocation = sTargetFolder;
  if (files.GetCount() == 1)
    sNewLocation.AppendPath(files[0].GetFileNameAndExtension());
  OnFlushFileSystemEvents();

  if (e->source() == this && files.GetCount() == 1)
  {
    if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sNewLocation))
    {
      QTreeWidgetItem* pItem = FindDirectoryTreeItem(sNewLocation, topLevelItem(0), {});
      if (pItem)
      {
        m_bTreeSelectionChangeInProgress = true;
        scrollToItem(pItem);
        clearSelection();
        pItem->setSelected(true);
        setCurrentItem(pItem);
        m_bTreeSelectionChangeInProgress = false;
      }
    }
  }
}

bool eqQtAssetBrowserFolderView::MoveContentPaths(xiiArrayPtr<const xiiString> paths, xiiStringView sTargetFolder)
{
  if (paths.IsEmpty() || sTargetFolder.IsEmpty() || !xiiOSFile::ExistsDirectory(sTargetFolder))
    return false;

  xiiSet<xiiString>          targets;
  xiiDynamicArray<xiiString> sourcesToMove;
  xiiDynamicArray<xiiString> destinations;
  for (const auto& sSource : paths)
  {
    xiiStringBuilder sDestination = sTargetFolder;
    sDestination.AppendPath(sSource.GetFileNameAndExtension());
    if (sSource == sDestination || sourcesToMove.Contains(sSource))
      continue;

    if (xiiPathUtils::IsSubPath(sSource, sTargetFolder) || targets.Contains(sDestination) ||
        QFileInfo::exists(xiiMakeQString(sDestination)))
    {
      xiiQtUiServices::MessageBoxWarning(xiiFmt("Cannot move '{}' to '{}'. The destination must be outside the source and the name must be available.", sSource, sDestination));
      return false;
    }
    targets.Insert(sDestination);
    sourcesToMove.PushBack(sSource);
    destinations.PushBack(sDestination);
  }

  if (sourcesToMove.IsEmpty() ||
      xiiQtUiServices::MessageBoxQuestion(xiiFmt("Move {} item(s) into '{}'?", sourcesToMove.GetCount(), sTargetFolder), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
    return false;

  bool bMoved = false;
  for (xiiUInt32 i = 0; i < sourcesToMove.GetCount(); ++i)
  {
    if (QFileInfo::exists(xiiMakeQString(destinations[i])) || xiiOSFile::MoveFileOrDirectory(sourcesToMove[i], destinations[i]).Failed())
    {
      xiiQtUiServices::MessageBoxWarning(xiiFmt("Could not move '{}' to '{}'. Check write permissions and whether the name is available.", sourcesToMove[i], destinations[i]));
      break;
    }

    xiiFileSystemModel::GetSingleton()->NotifyOfChange(destinations[i]);
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sourcesToMove[i]);
    bMoved = true;
  }
  return bMoved;
}

QStringList eqQtAssetBrowserFolderView::mimeTypes() const
{
  QStringList types;
  types << "application/xiiEditor.files";
  return types;
}

Qt::DropActions eqQtAssetBrowserFolderView::supportedDropActions() const
{
  return Qt::DropAction::MoveAction | Qt::DropAction::CopyAction;
}

QMimeData* eqQtAssetBrowserFolderView::mimeData(const QList<QTreeWidgetItem*>& items) const
{
  xiiHybridArray<QString, 1> files;
  for (const QTreeWidgetItem* pItem : items)
  {
    QModelIndex id   = indexFromItem(pItem);
    QString     text = id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    files.PushBack(text);
  }

  QByteArray  encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/xiiEditor.files", encodedData);
  return mimeData;
}

void eqQtAssetBrowserFolderView::keyPressEvent(QKeyEvent* e)
{
  QTreeWidget::keyPressEvent(e);

  if (e->key() == Qt::Key_Delete && !m_bDialogMode)
  {
    e->accept();
    DeleteFolder();
    return;
  }
}

void eqQtAssetBrowserFolderView::DeleteFolder()
{
  auto* pItem = currentItem();
  if (m_bDialogMode || !pItem || pItem == topLevelItem(0))
    return;

  const xiiBitflags<xiiAssetBrowserItemFlags> flags = static_cast<xiiAssetBrowserItemFlags::Enum>(pItem->data(0, xiiQtAssetBrowserModel::ItemFlags).toInt());
  if (flags.IsSet(xiiAssetBrowserItemFlags::Folder))
    DeleteContentPaths({pItem->data(0, xiiQtAssetBrowserModel::AbsolutePath).toString()});
}

void eqQtAssetBrowserFolderView::DeleteContentPaths(const QStringList& paths)
{
  if (paths.isEmpty())
    return;

  xiiHybridArray<xiiString, 8> folders;
  xiiSet<xiiString>            files;
  for (const auto& sPath : paths)
  {
    const xiiString sAbsolute = qtToXIIString(sPath);
    if (xiiOSFile::ExistsDirectory(sAbsolute))
      folders.PushBack(sAbsolute);
    else
      files.Insert(sAbsolute);
  }

  if (!folders.IsEmpty())
  {
    const auto knownFiles = xiiFileSystemModel::GetSingleton()->GetFiles();
    for (const auto& file : *knownFiles)
    {
      for (const auto& folder : folders)
      {
        if (xiiPathUtils::IsSubPath(folder, file.Key().GetAbsolutePath()))
        {
          files.Insert(file.Key().GetAbsolutePath());
          break;
        }
      }
    }
  }

  auto*           pCurator = xiiAssetCurator::GetSingleton();
  xiiSet<xiiUuid> references;
  for (auto* pManager : xiiDocumentManager::GetAllDocumentManagers())
  {
    for (const auto* pDocument : pManager->GetAllOpenDocuments())
    {
      if (pDocument->IsModified() && files.Contains(pDocument->GetDocumentPath()))
      {
        xiiQtUiServices::MessageBoxWarning(xiiFmt("Save or close '{}' before deleting it.", pDocument->GetDocumentPath()));
        return;
      }
    }
  }
  for (const auto& file : files)
    pCurator->FindAllUses(file, references);

  QStringList referringAssets;
  for (const auto& guid : references)
  {
    if (const auto asset = pCurator->GetSubAsset(guid))
    {
      if (!files.Contains(asset->m_pAssetInfo->m_Path.GetAbsolutePath()))
        referringAssets.push_back(xiiMakeQString(asset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath()));
    }
  }

  QString sQuestion = tr("Move %n selected item(s) to the trash?", nullptr, paths.size());
  if (!referringAssets.isEmpty())
  {
    sQuestion += tr("\n\n%n other asset(s) reference the selection. These references may break.", nullptr, referringAssets.size());
    sQuestion += QStringLiteral("\n\n") + referringAssets.mid(0, 6).join(QLatin1Char('\n'));
    if (referringAssets.size() > 6)
      sQuestion += tr("\n…and %1 more").arg(referringAssets.size() - 6);
  }

  const xiiString sMessage = qtToXIIString(sQuestion);
  if (xiiQtUiServices::MessageBoxQuestion(xiiFmt("{}", sMessage), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
    return;

  QStringList failed;
  for (const auto& sPath : paths)
  {
    const auto sAbsolute        = qtToXIIString(sPath);
    bool       bCoveredByFolder = false;
    for (const auto& folder : folders)
    {
      if (sAbsolute != folder && xiiPathUtils::IsSubPath(folder, sAbsolute))
      {
        bCoveredByFolder = true;
        break;
      }
    }
    if (bCoveredByFolder)
      continue;

    if (QFile::moveToTrash(sPath))
      xiiFileSystemModel::GetSingleton()->NotifyOfChange(sAbsolute);
    else
      failed.push_back(sPath);
  }
  if (!failed.isEmpty())
    xiiQtUiServices::MessageBoxWarning(xiiFmt("Could not move these paths to the trash:\n{}", qtToXIIString(failed.join(QLatin1Char('\n')))));
}

void eqQtAssetBrowserFolderView::OnFlushFileSystemEvents()
{
  XII_LOCK(m_FolderStructureMutex);

  for (const auto& e : m_QueuedFolderEvents)
  {
    switch (e.m_Type)
    {
      case xiiFolderChangedEvent::Type::FolderAdded:
      {
        BuildDirectoryTree(e.m_Path, e.m_Path.GetDataDirParentRelativePath(), topLevelItem(0), "", false);
      }
      break;

      case xiiFolderChangedEvent::Type::FolderRemoved:
      {
        RemoveDirectoryTreeItem(e.m_Path.GetDataDirParentRelativePath(), topLevelItem(0), "");
      }
      break;

      case xiiFolderChangedEvent::Type::ModelReset:
        UpdateDirectoryTree();
        break;

      default:
        break;
    }
  }

  m_QueuedFolderEvents.Clear();
}

void eqQtAssetBrowserFolderView::mouseDoubleClickEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->ignore();

    return;
  }
  QTreeWidget::mouseDoubleClickEvent(e);
}

void eqQtAssetBrowserFolderView::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::BackButton)
  {
    e->ignore();

    return;
  }

  QModelIndex inx = indexAt(e->pos());
  if (!inx.isValid())
    return;

  QTreeWidget::mousePressEvent(e);
}

void eqQtAssetBrowserFolderView::OnItemSelectionChanged()
{
  if (m_bTreeSelectionChangeInProgress)
    return;

  xiiStringBuilder sCurPath;

  if (!selectedItems().isEmpty())
  {
    sCurPath = selectedItems()[0]->data(0, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString().toUtf8().data();
  }

  m_pFilter->SetPathFilter(sCurPath);
}

void eqQtAssetBrowserFolderView::OnPathFilterChanged()
{
  const QString sPath = xiiMakeQString(m_pFilter->GetPathFilter());

  if (topLevelItemCount() == 1)
  {
    if (m_bTreeSelectionChangeInProgress)
      return;

    m_bTreeSelectionChangeInProgress = true;

    clearSelection();
    SelectPathFilter(topLevelItem(0), sPath);

    m_bTreeSelectionChangeInProgress = false;
  }
}

void eqQtAssetBrowserFolderView::TreeOpenExplorer()
{
  if (!currentItem())
    return;

  xiiStringBuilder sPath = currentItem()->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  xiiQtUiServices::OpenInExplorer(sPath, false);
}

bool eqQtAssetBrowserFolderView::SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath)
{
  if (pParent->data(0, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString() == sPath)
  {
    pParent->setSelected(true);

    setCurrentIndex(indexFromItem(pParent));

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
void eqQtAssetBrowserFolderView::UpdateDirectoryTree()
{
  xiiQtScopedBlockSignals block(this);

  if (topLevelItemCount() == 0)
  {
    QTreeWidgetItem* pNewParent = new QTreeWidgetItem();
    pNewParent->setText(0, tr("Content roots"));

    addTopLevelItem(pNewParent);

    pNewParent->setExpanded(true);

    selectionModel()->select(indexFromItem(pNewParent), QItemSelectionModel::SelectionFlag::ClearAndSelect);
  }

  auto Folders = xiiFileSystemModel::GetSingleton()->GetFolders();

  if (m_uiKnownAssetFolderCount == Folders->GetCount())
    return;

  m_uiKnownAssetFolderCount = Folders->GetCount();

  xiiStringBuilder tmp;

  for (const auto& sDir : *Folders)
  {
    BuildDirectoryTree(sDir.Key(), sDir.Key().GetDataDirParentRelativePath(), topLevelItem(0), "", false);
  }

  setSortingEnabled(true);
  sortItems(0, Qt::SortOrder::AscendingOrder);
}


void eqQtAssetBrowserFolderView::ClearDirectoryTree()
{
  clear();
  m_uiKnownAssetFolderCount = 0;
}

void eqQtAssetBrowserFolderView::BuildDirectoryTree(const xiiDataDirPath& path, xiiStringView sCurPath, QTreeWidgetItem* pParent, xiiStringView sCurPathToItem, bool bIsHidden)
{
  if (sCurPath.IsEmpty())
    return;

  const char* szNextSep = sCurPath.FindSubString("/");

  QTreeWidgetItem* pNewParent = nullptr;

  xiiString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = sCurPath;
  else
    sFolderName = xiiStringView(sCurPath.GetStartPointer(), szNextSep);

  if (sFolderName.EndsWith_NoCase("_data"))
  {
    bIsHidden = true;
  }

  xiiStringBuilder sCurPath2 = sCurPathToItem;
  sCurPath2.AppendPath(sFolderName);

  const QString sQtFolderName = xiiMakeQString(sFolderName.GetView());

  if (sQtFolderName == "AssetCache")
    return;

  for (xiiInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  { // #TODO_ASSET data for folder

    QString sPathAbs = pParent->data(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    QString sPathRel = pParent->data(0, xiiQtAssetBrowserModel::UserRoles::RelativePath).toString();

    if (sPathAbs.isEmpty())
    {
      sPathAbs = xiiMakeQString(path.GetAbsolutePath());
      sPathRel = xiiMakeQString(path.GetDataDirParentRelativePath());
    }
    else
    {
      sPathAbs += "/" + sQtFolderName;
      sPathRel += "/" + sQtFolderName;
    }

    const bool bIsDataDir = sCurPathToItem.IsEmpty();
    pNewParent            = new QTreeWidgetItem();
    pNewParent->setText(0, sQtFolderName);
    pNewParent->setData(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath, sPathAbs);
    pNewParent->setData(0, xiiQtAssetBrowserModel::UserRoles::RelativePath, sPathRel);
    xiiBitflags<xiiAssetBrowserItemFlags> flags = bIsDataDir ? xiiAssetBrowserItemFlags::DataDirectory : xiiAssetBrowserItemFlags::Folder;
    pNewParent->setData(0, xiiQtAssetBrowserModel::UserRoles::ItemFlags, (int)flags.GetValue());
    pNewParent->setIcon(0, xiiQtUiServices::GetCachedIconResource(bIsDataDir ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg"));
    if (!bIsDataDir)
      pNewParent->setFlags(pNewParent->flags() | Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsDragEnabled | Qt::ItemFlag::ItemIsDropEnabled);
    else
      pNewParent->setFlags(pNewParent->flags() | Qt::ItemFlag::ItemIsDropEnabled);

    if (bIsHidden)
    {
      pNewParent->setForeground(0, QColor::fromRgba(qRgb(110, 110, 120)));
    }

    pParent->addChild(pNewParent);
  }

godown:

  if (szNextSep == nullptr)
    return;

  BuildDirectoryTree(path, szNextSep + 1, pNewParent, sCurPath2, bIsHidden);
}

void eqQtAssetBrowserFolderView::RemoveDirectoryTreeItem(xiiStringView sCurPath, QTreeWidgetItem* pParent, xiiStringView sCurPathToItem)
{
  if (QTreeWidgetItem* pTreeItem = FindDirectoryTreeItem(sCurPath, pParent, sCurPathToItem))
  {
    delete pTreeItem;
  }
}

QTreeWidgetItem* eqQtAssetBrowserFolderView::FindDirectoryTreeItem(xiiStringView sCurPath, QTreeWidgetItem* pParent, xiiStringView sCurPathToItem)
{
  if (sCurPath.IsEmpty())
    return nullptr;

  const char* szNextSep = sCurPath.FindSubString("/");

  QTreeWidgetItem* pNewParent = nullptr;

  xiiString sFolderName;

  if (szNextSep == nullptr)
    sFolderName = sCurPath;
  else
    sFolderName = xiiStringView(sCurPath.GetStartPointer(), szNextSep);

  xiiStringBuilder sCurPath2 = sCurPathToItem;
  sCurPath2.AppendPath(sFolderName);

  const QString sQtFolderName = xiiMakeQString(sFolderName.GetView());

  for (xiiInt32 i = 0; i < pParent->childCount(); ++i)
  {
    if (pParent->child(i)->text(0) == sQtFolderName)
    {
      // item already exists
      pNewParent = pParent->child(i);
      goto godown;
    }
  }

  return nullptr;

godown:

  if (szNextSep == nullptr)
  {
    return pNewParent;
  }

  return FindDirectoryTreeItem(szNextSep + 1, pNewParent, sCurPath2);
}
