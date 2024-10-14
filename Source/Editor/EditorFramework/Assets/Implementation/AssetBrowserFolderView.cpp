#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserFolderView.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
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

  xiiStringBuilder sAbsPath = m_sParentFolder;
  sAbsPath.AppendPath(sTemp);
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
    xiiQtUiServices::ShowGlobalStatusBarMessage(res.m_sMessage.GetView());
    e->ignore();
  }
  else
  {
    xiiQtUiServices::ShowGlobalStatusBarMessage({});
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

  return xiiStatus(XII_SUCCESS);
}

void eqQtAssetBrowserFolderView::dropEvent(QDropEvent* e)
{
  xiiQtUiServices::ShowGlobalStatusBarMessage({});
  xiiHybridArray<xiiString, 1> files;
  xiiString                    sTargetFolder;
  // Always accept and call base class to end the drop operation as a no-op in the base class.
  e->accept();
  QTreeWidget::dropEvent(e);
  if (canDrop(e, files, sTargetFolder).Failed())
  {
    return;
  }

  QMessageBox::StandardButton choice = xiiQtUiServices::MessageBoxQuestion(xiiFmt("Move {} items into '{}'?", files.GetCount(), sTargetFolder), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No);
  if (choice == QMessageBox::StandardButton::No)
    return;

  xiiStringBuilder sNewLocation;
  for (const xiiString& sFile : files)
  {
    sNewLocation = sTargetFolder;
    sNewLocation.AppendPath(xiiPathUtils::GetFileNameAndExtension(sFile));
    if (xiiOSFile::MoveFileOrDirectory(sFile, sNewLocation).Failed())
    {
      xiiLog::Error("Failed to move '{}' to '{}'", sFile, sNewLocation);
    }
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sNewLocation);
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sFile);
  }
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
  if (QTreeWidgetItem* pCurrentItem = currentItem())
  {
    QModelIndex                 id         = indexFromItem(pCurrentItem);
    QString                     sQtAbsPath = id.data(xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();
    xiiString                   sAbsPath   = qtToXIIString(sQtAbsPath);
    QMessageBox::StandardButton choice     = xiiQtUiServices::MessageBoxQuestion(xiiFmt("Do you want to delete the folder\n'{}'?", sAbsPath), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Yes);
    if (choice == QMessageBox::StandardButton::Cancel)
      return;

    if (!QFile::moveToTrash(sQtAbsPath))
    {
      xiiLog::Error("Failed to delete folder '{}'", sAbsPath);
    }
    xiiFileSystemModel::GetSingleton()->NotifyOfChange(sAbsPath);
  }
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
    pNewParent->setText(0, QLatin1String("<root>"));

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
    const bool bIsDataDir = sCurPathToItem.IsEmpty();
    pNewParent            = new QTreeWidgetItem();
    pNewParent->setText(0, sQtFolderName);
    pNewParent->setData(0, xiiQtAssetBrowserModel::UserRoles::AbsolutePath, xiiMakeQString(path.GetAbsolutePath().GetView()));
    pNewParent->setData(0, xiiQtAssetBrowserModel::UserRoles::RelativePath, xiiMakeQString(path.GetDataDirParentRelativePath()));
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
