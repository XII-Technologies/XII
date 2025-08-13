#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

xiiQtAssetFilter::xiiQtAssetFilter(QObject* pParent) :
  QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// xiiQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct FileComparer
{
  FileComparer(xiiQtAssetBrowserModel* pModel, const xiiHashTable<xiiUuid, xiiSubAsset>& allAssets) :
    m_pModel(pModel), m_AllAssets(allAssets)
  {
    m_bSortByRecentlyUsed = m_pModel->m_pFilter->GetSortByRecentUse();
  }

  bool Less(const xiiQtAssetBrowserModel::VisibleEntry& a, const xiiQtAssetBrowserModel::VisibleEntry& b) const
  {
    if (a.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory) != b.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory))
      return a.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory);

    if (a.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory))
    {
      return xiiCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
    }

    const xiiSubAsset* pInfoA = nullptr;
    m_AllAssets.TryGetValue(a.m_Guid, pInfoA);

    const xiiSubAsset* pInfoB = nullptr;
    m_AllAssets.TryGetValue(b.m_Guid, pInfoB);

    xiiStringView sSortA;
    xiiStringView sSortB;
    if (pInfoA && !pInfoA->m_bMainAsset)
    {
      sSortA = pInfoA->GetName();
    }
    else
    {
      sSortA = a.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }
    if (pInfoB && !pInfoB->m_bMainAsset)
    {
      sSortB = pInfoB->GetName();
    }
    else
    {
      sSortB = b.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }

    if (m_bSortByRecentlyUsed)
    {
      if (pInfoA && pInfoB)
      {
        if (pInfoA->m_LastAccess != pInfoB->m_LastAccess)
        {
          return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
        }
      }
      else if (pInfoA && pInfoA->m_LastAccess.IsPositive())
      {
        return true;
      }
      else if (pInfoB && pInfoB->m_LastAccess.IsPositive())
      {
        return false;
      }

      // in all other cases, fall through and do the file name comparison
    }

    xiiInt32 iValue = sSortA.Compare_NoCase(sSortB);
    if (iValue == 0)
    {
      if (!pInfoA && !pInfoB)
        return xiiCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
      else if (pInfoA && pInfoB)
        return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
      else
        return pInfoA == nullptr;
    }
    return iValue < 0;
  }

  XII_ALWAYS_INLINE bool operator()(const xiiQtAssetBrowserModel::VisibleEntry& a, const xiiQtAssetBrowserModel::VisibleEntry& b) const
  {
    return Less(a, b);
  }

  xiiQtAssetBrowserModel*                   m_pModel = nullptr;
  const xiiHashTable<xiiUuid, xiiSubAsset>& m_AllAssets;
  bool                                      m_bSortByRecentlyUsed = false;
};

xiiQtAssetBrowserModel::xiiQtAssetBrowserModel(QObject* pParent, xiiQtAssetFilter* pFilter) :
  QAbstractItemModel(pParent), m_pFilter(pFilter)
{
}

xiiQtAssetBrowserModel::~xiiQtAssetBrowserModel()
{
  xiiFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(m_FileChangedSubscription);
  xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(m_FolderChangedSubscription);
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void xiiQtAssetBrowserModel::Initialize()
{
  XII_ASSERT_DEBUG(m_pFilter != nullptr, "xiiQtAssetBrowserModel requires a valid filter.");
  connect(m_pFilter, &xiiQtAssetFilter::FilterChanged, this, [this]() { resetModel(); });

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  XII_VERIFY(connect(xiiQtImageCache::GetSingleton(), &xiiQtImageCache::ImageLoaded, this, &xiiQtAssetBrowserModel::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(xiiQtImageCache::GetSingleton(), &xiiQtImageCache::ImageInvalidated, this, &xiiQtAssetBrowserModel::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");

  QWeakPointer<xiiQtAssetBrowserModel> pWeak = sharedFromThis();
  m_FileChangedSubscription                  = xiiFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler([pWeak](const xiiFileChangedEvent& e) {
    if (QSharedPointer<xiiQtAssetBrowserModel> strong = pWeak.toStrongRef())
    {
      strong->FileSystemFileEventHandler(e);
    }
  });
  m_FolderChangedSubscription                = xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler([pWeak](const xiiFolderChangedEvent& e) {
    if (QSharedPointer<xiiQtAssetBrowserModel> strong = pWeak.toStrongRef())
    {
      strong->FileSystemFolderEventHandler(e);
    }
  });
  xiiAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
}

void xiiQtAssetBrowserModel::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAssetCuratorEvent::Type::AssetUpdated:
    {
      VisibleEntry ve;
      ve.m_Guid         = e.m_AssetGuid;
      ve.m_sAbsFilePath = e.m_pInfo->m_pAssetInfo->m_Path;
      ve.m_Flags        = xiiAssetBrowserItemFlags::File;
      if (ve.m_Guid.IsValid())
      {
        ve.m_Flags |= xiiAssetBrowserItemFlags::Asset;
      }

      HandleEntry(ve, AssetOp::Updated);
      break;
    }
    case xiiAssetCuratorEvent::Type::AssetListReset:
    {
      m_ImportExtensions.Clear();
      xiiAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
      break;
    }
    default:
      break;
  }
}

xiiInt32 xiiQtAssetBrowserModel::FindAssetIndex(const xiiUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (xiiUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}

xiiInt32 xiiQtAssetBrowserModel::FindIndex(xiiStringView sAbsPath) const
{
  for (xiiUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_sAbsFilePath.GetAbsolutePath() == sAbsPath)
    {
      return i;
    }
  }
  return -1;
}

void xiiQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  m_EntriesToDisplay.Clear();
  m_DisplayedEntries.Clear();

  // Get Curator Mutex first to prevent deadlocks
  xiiAssetCurator::xiiLockedSubAssetTable   AllAssetsLocked = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  const xiiHashTable<xiiUuid, xiiSubAsset>& AllAssets       = *(AllAssetsLocked.operator->());

  auto allFiles   = xiiFileSystemModel::GetSingleton()->GetFiles();
  auto allFolders = xiiFileSystemModel::GetSingleton()->GetFolders();

  for (const auto& folder : *allFolders)
  {
    if (m_pFilter->IsAssetFiltered(folder.Key().GetDataDirParentRelativePath(), true, nullptr))
      continue;

    auto& entry          = m_EntriesToDisplay.ExpandAndGetRef();
    entry.m_Flags        = folder.Key().GetDataDirRelativePath().IsEmpty() ? xiiAssetBrowserItemFlags::DataDirectory : xiiAssetBrowserItemFlags::Folder;
    entry.m_sAbsFilePath = folder.Key();
  }

  for (const auto& file : *allFiles)
  {
    if (file.Value().m_DocumentID.IsValid())
    {
      auto mainAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(file.Value().m_DocumentID);

      if (!mainAsset)
        continue;

      if (!m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*mainAsset)))
      {
        auto& entry          = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid         = file.Value().m_DocumentID;
        entry.m_Flags        = xiiAssetBrowserItemFlags::File | xiiAssetBrowserItemFlags::Asset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }

      for (const auto& subAssetGuid : mainAsset->m_pAssetInfo->m_SubAssets)
      {
        auto subAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(subAssetGuid);

        if (subAsset && m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*subAsset)))
          continue;

        auto& entry          = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid         = subAssetGuid;
        entry.m_Flags |= xiiAssetBrowserItemFlags::SubAsset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }
    }
    else
    {
      if (m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, nullptr))
        continue;

      auto& entry          = m_EntriesToDisplay.ExpandAndGetRef();
      entry.m_sAbsFilePath = file.Key();
      entry.m_Flags        = xiiAssetBrowserItemFlags::File;
    }
  }

  FileComparer cmp(this, AllAssets);
  m_EntriesToDisplay.Sort(cmp);

  endResetModel();
}

void xiiQtAssetBrowserModel::HandleEntry(const VisibleEntry& entry, AssetOp op)
{
  auto subAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(entry.m_Guid);

  if (m_pFilter->IsAssetFiltered(entry.m_sAbsFilePath.GetDataDirParentRelativePath(), entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory), subAsset.Borrow()))
  {
    if (!m_DisplayedEntries.Contains(entry.m_Guid))
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  xiiAssetCurator::xiiLockedSubAssetTable   AllAssetsLocked = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  const xiiHashTable<xiiUuid, xiiSubAsset>& AllAssets       = *AllAssetsLocked.Borrow();

  FileComparer  cmp(this, AllAssets);
  VisibleEntry* pLB           = std::lower_bound(begin(m_EntriesToDisplay), end(m_EntriesToDisplay), entry, cmp);
  xiiUInt32     uiInsertIndex = pLB - m_EntriesToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model on top of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_EntriesToDisplay.GetCount())
  {
    for (xiiUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); i++)
    {
      VisibleEntry& displayEntry = m_EntriesToDisplay[i];
      if (!cmp.Less(displayEntry, entry) && !cmp.Less(entry, displayEntry))
      {
        uiInsertIndex = i;
        pLB           = &displayEntry;
        break;
      }
    }
  }

  if (op == AssetOp::Add)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_EntriesToDisplay.InsertAt(uiInsertIndex, entry);
    if (entry.m_Guid.IsValid())
      m_DisplayedEntries.Insert(entry.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_EntriesToDisplay.RemoveAtAndCopy(uiInsertIndex);
      if (entry.m_Guid.IsValid())
        m_DisplayedEntries.Remove(entry.m_Guid);
      endRemoveRows();
    }
  }
  else // Updated.
  {
    // Updated entries can cause the filter function `IsAssetFiltered` to change its result, e.g. the transform issues list in the curator panel shows assets that were updated from a healthy state to an error state.
    // Thus, updated entries could be missing in the list at this point, so we need to first check if the item already exists:
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      // Already exists.
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT dataChanged(idx, idx);
    }
    else
    {
      // Item not found. Do an exhaustive search in case the name was changed in which the order is no longer the same.
      xiiInt32 oldIndex = FindAssetIndex(entry.m_Guid);
      if (oldIndex != -1)
      {
        // Order (most likely name) has changed, remove old entry and insert new one.
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_EntriesToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(entry.m_Guid);
        endRemoveRows();
      }
      // Reinsert the updated entry.
      HandleEntry(entry, AssetOp::Add);
    }
  }
}


////////////////////////////////////////////////////////////////////////
// xiiQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void xiiQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2)
{
  const xiiUuid guid(userData1.toULongLong(), userData2.toULongLong());

  for (xiiUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void xiiQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID)
{
  for (xiiUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void xiiQtAssetBrowserModel::OnFileSystemUpdate()
{
  xiiDynamicArray<FsEvent> events;

  {
    XII_LOCK(m_Mutex);
    events.Swap(m_QueuedFileSystemEvents);
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type == xiiFileChangedEvent::Type::ModelReset)
    {
      resetModel();
      return;
    }
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type != xiiFileChangedEvent::Type::None)
      HandleFile(e.m_FileEvent);
    else
      HandleFolder(e.m_FolderEvent);
  }
}

QVariant xiiQtAssetBrowserModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_EntriesToDisplay.GetCount())
    return QVariant();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  // Common properties shared among all item types.
  switch (iRole)
  {
    case xiiQtAssetBrowserModel::UserRoles::ItemFlags:
      return (int)entry.m_Flags.GetValue();
    case xiiQtAssetBrowserModel::UserRoles::Importable:
    {
      if (entry.m_Flags.IsSet(xiiAssetBrowserItemFlags::File) && !entry.m_Flags.IsSet(xiiAssetBrowserItemFlags::Asset))
      {
        xiiStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        return bImportable;
      }
      return false;
    }
    case xiiQtAssetBrowserModel::UserRoles::RelativePath:
      return xiiMakeQString(entry.m_sAbsFilePath.GetDataDirParentRelativePath());
    case xiiQtAssetBrowserModel::UserRoles::AbsolutePath:
      return xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
  }

  if (entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory))
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        xiiStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();

        return xiiMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        xiiStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
        return xiiMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case xiiQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        return xiiQtUiServices::GetCachedIconResource(entry.m_Flags.IsSet(xiiAssetBrowserItemFlags::DataDirectory) ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg");
      }
    }
  }
  else if (!entry.m_Guid.IsValid()) // Normal file
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension());
      }
      break;

      case Qt::EditRole:
      {
        xiiStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName(); // remove the file extension
        return xiiMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case xiiQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        xiiStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable   = m_ImportExtensions.Contains(sExt);
        const bool bIsReferenced = xiiAssetCurator::GetSingleton()->IsReferenced(entry.m_sAbsFilePath.GetAbsolutePath());
        if (bImportable)
        {
          return xiiQtUiServices::GetCachedIconResource(bIsReferenced ? ":/EditorFramework/Icons/ImportedFile.svg" : ":/EditorFramework/Icons/ImportableFile.svg");
        }
        return xiiQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Document.svg");
      }

      case Qt::DecorationRole:
      {
        QFileInfo fi(xiiMakeQString(entry.m_sAbsFilePath));
        return m_IconProvider.icon(fi);
      }
    }
  }
  else if (entry.m_Guid.IsValid()) // Asset or sub-asset
  {
    const xiiUuid                            AssetGuid = entry.m_Guid;
    const xiiAssetCurator::xiiLockedSubAsset pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

    // this can happen when a file was just changed on disk, e.g. got deleted
    if (pSubAsset == nullptr)
      return QVariant();

    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        xiiStringBuilder sFilename = pSubAsset->GetName();
        return xiiMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        if (entry.m_Flags.IsSet(xiiAssetBrowserItemFlags::Asset))
        {
          // Don't allow changing extensions of assets
          xiiStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName();
          return xiiMakeQString(sFilename);
        }
      }
      break;

      case Qt::ToolTipRole:
      {
        xiiStringBuilder sToolTip = pSubAsset->GetName();
        sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
        sToolTip.Append("\nTransform State: ");
        switch (pSubAsset->m_pAssetInfo->m_TransformState)
        {
          case xiiAssetInfo::Unknown:
            sToolTip.Append("Unknown");
            break;
          case xiiAssetInfo::UpToDate:
            sToolTip.Append("Up To Date");
            break;
          case xiiAssetInfo::NeedsTransform:
            sToolTip.Append("Needs Transform");
            break;
          case xiiAssetInfo::NeedsThumbnail:
            sToolTip.Append("Needs Thumbnail");
            break;
          case xiiAssetInfo::TransformError:
            sToolTip.Append("Transform Error");
            break;
          case xiiAssetInfo::MissingTransformDependency:
            sToolTip.Append("Missing Transform Dependency");
            break;
          case xiiAssetInfo::MissingPackageDependency:
            sToolTip.Append("Missing Package Dependency");
            break;
          case xiiAssetInfo::MissingThumbnailDependency:
            sToolTip.Append("Missing Thumbnail Dependency");
            break;
          case xiiAssetInfo::CircularDependency:
            sToolTip.Append("Circular Dependency");
            break;
          default:
            break;
        }

        return QString::fromUtf8(sToolTip, sToolTip.GetElementCount());
      }
      case Qt::DecorationRole:
      {
        if (m_bIconMode)
        {
          xiiString sThumbnailPath = pSubAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);

          xiiUInt64 uiUserData1, uiUserData2;
          AssetGuid.GetValues(uiUserData1, uiUserData2);

          const QPixmap* pThumbnailPixmap = xiiQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName, sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &entry.m_uiThumbnailID);

          return *pThumbnailPixmap;
        }
        else
        {
          return xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, xiiColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::OverlayIcon));
        }
      }
      break;

      case UserRoles::SubAssetGuid:
        return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
      case UserRoles::AssetGuid:
        return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
      case UserRoles::AssetIcon:
        return xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, xiiColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::OverlayIcon));
      case UserRoles::TransformState:
        return (int)pSubAsset->m_pAssetInfo->m_TransformState;
    }
  }
  else
  {
    XII_ASSERT_NOT_IMPLEMENTED;
  }
  return QVariant();
}

bool xiiQtAssetBrowserModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  if (!index.isValid())
    return false;

  if (iRole != Qt::EditRole)
    return false;

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_EntriesToDisplay.GetCount())
    return false;

  const VisibleEntry& entry    = m_EntriesToDisplay[iRow];
  const bool          bIsAsset = entry.m_Guid.IsValid();
  if (entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::File))
  {
    const xiiString& sAbsPath = entry.m_sAbsFilePath.GetAbsolutePath();
    emit             editingFinished(xiiMakeQString(sAbsPath), value.toString(), bIsAsset);

    return true;
  }

  return false;
}

Qt::ItemFlags xiiQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_EntriesToDisplay.GetCount())
    return Qt::ItemFlags();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::File | xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::Asset))
  {
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
  }

  if (entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::SubAsset))
  {
    flags |= Qt::ItemIsDragEnabled;
  }

  return flags;
}

QVariant xiiQtAssetBrowserModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Files");
    }
  }
  return QVariant();
}

QModelIndex xiiQtAssetBrowserModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn);
}

QModelIndex xiiQtAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int xiiQtAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_EntriesToDisplay.GetCount();
}

int xiiQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList xiiQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/xiiEditor.AssetGuid";
  types << "application/xiiEditor.files";
  return types;
}

QMimeData* xiiQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QString                    sGuids;
  QList<QUrl>                urls;
  xiiHybridArray<QString, 1> guids;
  xiiHybridArray<QString, 1> files;

  xiiStringBuilder tmp;

  for (xiiUInt32 i = 0; i < (xiiUInt32)indexes.size(); ++i)
  {
    QString sGuid(xiiConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<xiiUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();
    guids.PushBack(sGuid);
    if (i == 0)
      sGuids += sPath;
    else
      sGuids += "\n" + sPath;

    files.PushBack(sPath);
    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  QByteArray  encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << guids;

  QByteArray  encodedData2;
  QDataStream stream2(&encodedData2, QIODevice::WriteOnly);
  stream2 << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/xiiEditor.AssetGuid", encodedData);
  mimeData->setData("application/xiiEditor.files", encodedData2);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

Qt::DropActions xiiQtAssetBrowserModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::LinkAction;
}

void xiiQtAssetBrowserModel::FileSystemFileEventHandler(const xiiFileChangedEvent& e)
{
  bool bFire = false;

  {
    XII_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res       = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FileEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void xiiQtAssetBrowserModel::FileSystemFolderEventHandler(const xiiFolderChangedEvent& e)
{
  bool bFire = false;

  {
    XII_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res         = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FolderEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void xiiQtAssetBrowserModel::HandleFile(const xiiFileChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Guid         = e.m_Status.m_DocumentID;
  ve.m_sAbsFilePath = e.m_Path;
  ve.m_Flags        = xiiAssetBrowserItemFlags::File;
  if (ve.m_Guid.IsValid())
  {
    ve.m_Flags |= xiiAssetBrowserItemFlags::Asset;
  }

  switch (e.m_Type)
  {
    case xiiFileChangedEvent::Type::ModelReset:
      resetModel();
      return;

    case xiiFileChangedEvent::Type::FileAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case xiiFileChangedEvent::Type::DocumentLinked:
    {
      ve.m_Guid = xiiUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case xiiFileChangedEvent::Type::DocumentUnlinked:
    {
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = xiiUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case xiiFileChangedEvent::Type::FileRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}

void xiiQtAssetBrowserModel::HandleFolder(const xiiFolderChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Flags        = xiiAssetBrowserItemFlags::Folder;
  ve.m_sAbsFilePath = e.m_Path;

  switch (e.m_Type)
  {
    case xiiFolderChangedEvent::Type::FolderAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case xiiFolderChangedEvent::Type::FolderRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}
