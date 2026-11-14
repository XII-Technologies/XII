/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QDateTime>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLocale>
#include <QPromise>
#include <QThreadPool>
#include <QTimer>

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

    xiiInt32 iColumnOrder = 0;
    switch (m_pModel->m_iSortColumn)
    {
      case xiiQtAssetBrowserModel::TypeColumn:
        iColumnOrder = (pInfoA ? xiiStringView(pInfoA->m_Data.m_sSubAssetsDocumentTypeName) : a.m_sAbsFilePath.GetAbsolutePath().GetFileExtension())
                         .Compare_NoCase(pInfoB ? xiiStringView(pInfoB->m_Data.m_sSubAssetsDocumentTypeName) : b.m_sAbsFilePath.GetAbsolutePath().GetFileExtension());
        break;
      case xiiQtAssetBrowserModel::ModifiedColumn:
        if (a.m_LastModified.IsValid() && b.m_LastModified.IsValid())
        {
          if (!a.m_LastModified.Compare(b.m_LastModified, xiiTimestamp::CompareMode::Identical))
            iColumnOrder = a.m_LastModified.Compare(b.m_LastModified, xiiTimestamp::CompareMode::Newer) ? 1 : -1;
        }
        else
          iColumnOrder = static_cast<int>(a.m_LastModified.IsValid()) - static_cast<int>(b.m_LastModified.IsValid());
        break;
      case xiiQtAssetBrowserModel::PathColumn:
        iColumnOrder = a.m_sAbsFilePath.GetDataDirParentRelativePath().Compare_NoCase(b.m_sAbsFilePath.GetDataDirParentRelativePath());
        break;
    }
    if (iColumnOrder != 0)
      return m_pModel->m_SortOrder == Qt::AscendingOrder ? iColumnOrder < 0 : iColumnOrder > 0;

    if (m_bSortByRecentlyUsed && m_pModel->m_iSortColumn == xiiQtAssetBrowserModel::NameColumn)
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
    return m_pModel->m_SortOrder == Qt::AscendingOrder ? iValue < 0 : iValue > 0;
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
  connect(m_pFilter, &xiiQtAssetFilter::FilterChanged, this, &xiiQtAssetBrowserModel::resetModel);
  connect(m_pFilter, &xiiQtAssetFilter::FilterNarrowed, this, &xiiQtAssetBrowserModel::NarrowFilter);
  m_pFileSizes = new QFutureWatcher<QList<qint64>>(this);
  connect(m_pFileSizes, &QFutureWatcher<QList<qint64>>::finished, this, &xiiQtAssetBrowserModel::FinishFileSizeQueries);

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
  if (e.m_Type == xiiAssetCuratorEvent::Type::AssetListReset)
  {
    m_ImportExtensions.Clear();
    xiiAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
    return;
  }

  if ((e.m_Type != xiiAssetCuratorEvent::Type::AssetAdded &&
       e.m_Type != xiiAssetCuratorEvent::Type::AssetMoved &&
       e.m_Type != xiiAssetCuratorEvent::Type::AssetRemoved &&
       e.m_Type != xiiAssetCuratorEvent::Type::AssetUpdated) ||
      !e.m_pInfo)
    return;

  VisibleEntry entry;
  entry.m_Guid         = e.m_AssetGuid;
  entry.m_sAbsFilePath = e.m_pInfo->m_pAssetInfo->m_Path;
  entry.m_Flags        = xiiAssetBrowserItemFlags::SubAsset;
  if (e.m_pInfo->m_bMainAsset)
    entry.m_Flags = xiiAssetBrowserItemFlags::File | xiiAssetBrowserItemFlags::Asset;
  xiiFileStatus stat;
  if (xiiFileSystemModel::GetSingleton()->FindFile(entry.m_sAbsFilePath.GetAbsolutePath(), stat).Succeeded())
    entry.m_LastModified = stat.m_LastModified;

  if (e.m_Type == xiiAssetCuratorEvent::Type::AssetRemoved)
  {
    HandleEntry(entry, AssetOp::Remove);
    for (const auto& subAssetGuid : e.m_pInfo->m_pAssetInfo->m_SubAssets)
    {
      entry.m_Guid  = subAssetGuid;
      entry.m_Flags = xiiAssetBrowserItemFlags::SubAsset;
      HandleEntry(entry, AssetOp::Remove);
    }
  }
  else
  {
    HandleEntry(entry, AssetOp::Updated);
    if (e.m_pInfo->m_bMainAsset)
    {
      for (const auto& subAssetGuid : e.m_pInfo->m_pAssetInfo->m_SubAssets)
      {
        entry.m_Guid  = subAssetGuid;
        entry.m_Flags = xiiAssetBrowserItemFlags::SubAsset;
        HandleEntry(entry, AssetOp::Updated);
      }
    }
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

  m_FileSizePending.clear();
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
        entry.m_LastModified = file.Value().m_LastModified;
        entry.m_Guid         = file.Value().m_DocumentID;
        entry.m_Flags        = xiiAssetBrowserItemFlags::File | xiiAssetBrowserItemFlags::Asset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }

      for (const auto& subAssetGuid : mainAsset->m_pAssetInfo->m_SubAssets)
      {
        auto subAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(subAssetGuid);

        if (!subAsset || m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*subAsset)))
          continue;

        auto& entry          = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_LastModified = file.Value().m_LastModified;
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
      entry.m_LastModified = file.Value().m_LastModified;
      entry.m_Flags        = xiiAssetBrowserItemFlags::File;
    }
  }

  FileComparer cmp(this, AllAssets);
  m_EntriesToDisplay.Sort(cmp);

  endResetModel();
}

void xiiQtAssetBrowserModel::HandleEntry(const VisibleEntry& entry, AssetOp op)
{
  const auto         allAssets = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  const xiiSubAsset* pAsset    = nullptr;
  allAssets->TryGetValue(entry.m_Guid, pAsset);
  const bool bFolder = entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory);

  if (op != AssetOp::Remove && m_pFilter->IsAssetFiltered(entry.m_sAbsFilePath.GetDataDirParentRelativePath(), bFolder, pAsset))
    op = AssetOp::Remove;

  if (op == AssetOp::Remove && entry.m_Guid.IsValid() && !m_DisplayedEntries.Contains(entry.m_Guid))
    return;

  const FileComparer comparer(this, *allAssets.Borrow());
  const auto         findInsertion = [&]() {
    if (m_EntriesToDisplay.IsEmpty())
      return 0;
    return static_cast<int>(std::lower_bound(begin(m_EntriesToDisplay), end(m_EntriesToDisplay), entry, comparer) - begin(m_EntriesToDisplay));
  };
  auto sameIdentity = [&entry](const VisibleEntry& existing) {
    if (entry.m_Guid.IsValid())
      return existing.m_Guid == entry.m_Guid;
    return !existing.m_Guid.IsValid() && existing.m_sAbsFilePath.GetAbsolutePath() == entry.m_sAbsFilePath.GetAbsolutePath();
  };

  int iInsert   = findInsertion();
  int iExisting = -1;
  if (iInsert < rowCount() && sameIdentity(m_EntriesToDisplay[iInsert]))
    iExisting = iInsert;
  else if (entry.m_Guid.IsValid() && m_DisplayedEntries.Contains(entry.m_Guid))
    iExisting = FindAssetIndex(entry.m_Guid);
  else if (!entry.m_Guid.IsValid() && op != AssetOp::Add)
  {
    for (int i = 0; i < rowCount(); ++i)
    {
      if (sameIdentity(m_EntriesToDisplay[i]))
      {
        iExisting = i;
        break;
      }
    }
  }

  if (op == AssetOp::Remove)
  {
    if (iExisting < 0)
      return;
    beginRemoveRows({}, iExisting, iExisting);
    m_EntriesToDisplay.RemoveAtAndCopy(iExisting);
    m_DisplayedEntries.Remove(entry.m_Guid);
    endRemoveRows();
    return;
  }

  if (iExisting >= 0)
  {
    if (op == AssetOp::Add)
      return;

    VisibleEntry updated    = entry;
    const auto&  previous   = m_EntriesToDisplay[iExisting];
    updated.m_uiThumbnailID = previous.m_uiThumbnailID;
    if (updated.m_sAbsFilePath.GetAbsolutePath() == previous.m_sAbsFilePath.GetAbsolutePath() &&
        updated.m_LastModified.Compare(previous.m_LastModified, xiiTimestamp::CompareMode::Identical))
      updated.m_iFileSize = previous.m_iFileSize;
    const bool bFitsBefore = iExisting == 0 || !comparer(updated, m_EntriesToDisplay[iExisting - 1]);
    const bool bFitsAfter  = iExisting + 1 == rowCount() || !comparer(m_EntriesToDisplay[iExisting + 1], updated);
    if (bFitsBefore && bFitsAfter)
    {
      m_EntriesToDisplay[iExisting] = updated;
      Q_EMIT dataChanged(index(iExisting, NameColumn), index(iExisting, ColumnCount - 1));
      return;
    }

    if (iInsert > iExisting)
      --iInsert;
    if (iInsert != iExisting)
    {
      beginMoveRows({}, iExisting, iExisting, {}, iInsert > iExisting ? iInsert + 1 : iInsert);
      m_EntriesToDisplay.RemoveAtAndCopy(iExisting);
      m_EntriesToDisplay.InsertAt(iInsert, updated);
      endMoveRows();
    }
    else
    {
      m_EntriesToDisplay[iExisting] = updated;
    }
    Q_EMIT dataChanged(index(iInsert, NameColumn), index(iInsert, ColumnCount - 1));
    return;
  }

  beginInsertRows({}, iInsert, iInsert);
  m_EntriesToDisplay.InsertAt(iInsert, entry);
  if (entry.m_Guid.IsValid())
    m_DisplayedEntries.Insert(entry.m_Guid);
  endInsertRows();
}


void xiiQtAssetBrowserModel::NarrowFilter()
{
  const auto assets = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  int        iLast  = -1;
  for (int i = rowCount() - 1; i >= -1; --i)
  {
    if (i >= 0)
    {
      const auto&        entry  = m_EntriesToDisplay[i];
      const xiiSubAsset* pAsset = nullptr;
      assets->TryGetValue(entry.m_Guid, pAsset);
      const bool bFolder = entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::Folder | xiiAssetBrowserItemFlags::DataDirectory);
      if (m_pFilter->IsAssetFiltered(entry.m_sAbsFilePath.GetDataDirParentRelativePath(), bFolder, pAsset))
      {
        if (iLast < 0)
          iLast = i;
        continue;
      }
    }
    if (iLast >= 0)
    {
      beginRemoveRows({}, i + 1, iLast);
      for (int iRemove = i + 1; iRemove <= iLast; ++iRemove)
        m_DisplayedEntries.Remove(m_EntriesToDisplay[iRemove].m_Guid);
      m_EntriesToDisplay.RemoveAtAndCopy(i + 1, iLast - i);
      endRemoveRows();
      iLast = -1;
    }
  }
}

void xiiQtAssetBrowserModel::QueueFileSize(const QModelIndex& index) const
{
  const bool bSchedule = m_FileSizePending.isEmpty();
  m_FileSizePending.insert(index.siblingAtColumn(NameColumn));
  if (bSchedule)
  {
    auto* pThis = const_cast<xiiQtAssetBrowserModel*>(this);
    QTimer::singleShot(0, pThis, &xiiQtAssetBrowserModel::StartFileSizeQueries);
  }
}

void xiiQtAssetBrowserModel::StartFileSizeQueries()
{
  if (!m_FileSizeBatch.isEmpty() || m_FileSizePending.isEmpty())
    return;

  QStringList paths;
  for (const auto& index : std::exchange(m_FileSizePending, {}))
  {
    if (!index.isValid() || m_EntriesToDisplay[index.row()].m_iFileSize != -2)
      continue;

    const auto&    entry    = m_EntriesToDisplay[index.row()];
    const auto     sPath    = xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
    const QVariant modified = entry.m_LastModified.IsValid() ? QVariant(entry.m_LastModified.GetInt64(xiiSIUnitOfTime::Microsecond)) : QVariant();
    m_FileSizeBatch.push_back({index, sPath, modified});
    paths.push_back(sPath);
  }
  if (paths.isEmpty())
    return;

  QPromise<QList<qint64>> results;
  m_pFileSizes->setFuture(results.future());
  QThreadPool::globalInstance()->start([paths = std::move(paths), results = std::move(results)]() mutable {
    results.start();
    QList<qint64> sizes;
    sizes.reserve(paths.size());
    for (const auto& sPath : paths)
    {
      const QFileInfo info(sPath);
      sizes.push_back(info.isFile() ? info.size() : -1);
    }
    results.addResult(std::move(sizes));
    results.finish();
  });
}

void xiiQtAssetBrowserModel::FinishFileSizeQueries()
{
  const auto results = m_pFileSizes->result();
  const auto batch   = std::exchange(m_FileSizeBatch, {});
  for (qsizetype i = 0; i < batch.size(); ++i)
  {
    const auto& request = batch[i];
    if (!request.m_Index.isValid())
      continue;

    auto&          entry    = m_EntriesToDisplay[request.m_Index.row()];
    const QVariant modified = entry.m_LastModified.IsValid() ? QVariant(entry.m_LastModified.GetInt64(xiiSIUnitOfTime::Microsecond)) : QVariant();
    if (modified != request.m_Modified || xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath()) != request.m_sPath)
      continue;

    entry.m_iFileSize = results[i];
    const auto size   = request.m_Index.sibling(request.m_Index.row(), SizeColumn);
    Q_EMIT dataChanged(size, size, {Qt::DisplayRole, FileSize});
  }

  StartFileSizeQueries();
}

////////////////////////////////////////////////////////////////////////
// xiiQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void xiiQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2)
{
  const xiiUuid guid(userData1.toULongLong(), userData2.toULongLong());

  if (!m_DisplayedEntries.Contains(guid))
    return;

  int iRow = index.model() == this ? index.row() : -1;
  if (iRow < 0 || iRow >= rowCount() || m_EntriesToDisplay[iRow].m_Guid != guid)
    iRow = FindAssetIndex(guid);
  if (iRow >= 0)
  {
    const auto item = createIndex(iRow, NameColumn);
    Q_EMIT dataChanged(item, item, {Qt::DecorationRole, Thumbnail});
  }
}

void xiiQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID)
{
  for (xiiUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx, {Qt::DecorationRole, Thumbnail});
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

namespace
{
  struct TransformStateDisplay
  {
    const char* m_szLabel;
    const char* m_szIcon;
  };

  constexpr TransformStateDisplay s_TransformStates[] = {
    {QT_TRANSLATE_NOOP("AssetBrowser", "Pending"), ""},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Up to date"), ":/EditorFramework/Icons/AssetOk.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Needs import"), ":/EditorFramework/Icons/AssetNeedsImport.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Needs transform"), ":/EditorFramework/Icons/AssetNeedsTransform.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Needs thumbnail"), ":/EditorFramework/Icons/AssetNeedsThumbnail.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Transform failed"), ":/EditorFramework/Icons/AssetFailedTransform.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Missing transform dependency"), ":/EditorFramework/Icons/AssetMissingDependency.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Missing thumbnail dependency"), ":/EditorFramework/Icons/AssetMissingReference.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Missing package dependency"), ":/EditorFramework/Icons/AssetMissingDependency.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Circular dependency"), ":/EditorFramework/Icons/AssetFailedTransform.svg"},
    {QT_TRANSLATE_NOOP("AssetBrowser", "Processing"), ":/EditorFramework/Icons/TransformAsset.svg"},
  };
  static_assert(std::size(s_TransformStates) == xiiAssetInfo::COUNT + 1);
} // namespace

QString xiiQtAssetBrowserModel::TransformStateLabel(int iState)
{
  return iState >= 0 && iState <= xiiAssetInfo::COUNT ? QCoreApplication::translate("AssetBrowser", s_TransformStates[iState].m_szLabel) : QString();
}

QIcon xiiQtAssetBrowserModel::TransformStateIcon(int iState)
{
  if (iState >= 0 && iState <= xiiAssetInfo::COUNT && s_TransformStates[iState].m_szIcon[0] != '\0')
    return xiiQtUiServices::GetCachedIconResource(s_TransformStates[iState].m_szIcon);

  return {};
}

QVariant xiiQtAssetBrowserModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid())
    return QVariant();

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_EntriesToDisplay.GetCount())
    return QVariant();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  if (index.column() != NameColumn && (iRole == Qt::DisplayRole || iRole == Qt::DecorationRole))
  {
    const auto name = index.siblingAtColumn(NameColumn);
    if (iRole == Qt::DecorationRole)
      return index.column() == StatusColumn && entry.m_Guid.IsValid() ? QVariant(TransformStateIcon(data(name, TransformState).toInt())) : QVariant();

    switch (index.column())
    {
      case TypeColumn:
        if (const auto pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(entry.m_Guid))
          return xiiMakeQString(xiiTranslate(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName));
        return xiiMakeQString(entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension());
      case StatusColumn:
        return entry.m_Guid.IsValid() ? TransformStateLabel(data(name, TransformState).toInt()) : QString();
      case SizeColumn:
      {
        const auto bytes = data(name, FileSize);
        return bytes.isValid() && bytes.toLongLong() >= 0 ? QLocale().formattedDataSize(bytes.toLongLong()) : QString();
      }
      case ModifiedColumn:
        if (entry.m_LastModified.IsValid())
          return QLocale().toString(QDateTime::fromMSecsSinceEpoch(entry.m_LastModified.GetInt64(xiiSIUnitOfTime::Millisecond)), QLocale::ShortFormat);
        return {};
      case PathColumn:
        return data(name, RelativePath);
    }
    return {};
  }

  // Common properties shared among all item types.
  switch (iRole)
  {
    case FileSize:
      if (!entry.m_Flags.IsAnySet(xiiAssetBrowserItemFlags::File | xiiAssetBrowserItemFlags::SubAsset))
        return {};
      if (entry.m_iFileSize == -2)
      {
        QueueFileSize(index);
        return {};
      }
      return entry.m_iFileSize;
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
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        if (bImportable)
        {
          const bool bIsReferenced = xiiAssetCurator::GetSingleton()->IsReferenced(entry.m_sAbsFilePath.GetAbsolutePath());
          return xiiQtUiServices::GetCachedIconResource(bIsReferenced ? ":/EditorFramework/Icons/ImportedFile.svg" : ":/EditorFramework/Icons/ImportableFile.svg");
        }
        return xiiQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Document.svg");
      }

      case Qt::DecorationRole:
        return data(index, AssetIcon);
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
        return xiiMakeQString(sToolTip) + QLatin1Char('\n') + TransformStateLabel(pSubAsset->m_pAssetInfo->m_TransformState);
      }
      case Qt::DecorationRole:
        return data(index, m_bIconMode ? Thumbnail : AssetIcon);
      case UserRoles::Thumbnail:
      {
        xiiString sThumbnailPath = pSubAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);
        xiiUInt64 uiUserData1, uiUserData2;
        AssetGuid.GetValues(uiUserData1, uiUserData2);
        auto*         pCache = xiiQtImageCache::GetSingleton();
        const QPixmap pixmap = *pCache->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName, sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &entry.m_uiThumbnailID);
        if (pCache->IsUnavailablePixmap(pixmap))
          return data(index, AssetIcon).value<QIcon>().pixmap(96);
        return pixmap;
      }

      case UserRoles::SubAssetGuid:
        return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
      case UserRoles::AssetGuid:
        return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
      case UserRoles::AssetIcon:
        return xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, xiiColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, xiiColorScheme::CategoryColorUsage::OverlayIcon));
      case UserRoles::TransformState:
        if (xiiAssetCurator::GetSingleton()->IsAssetBeingTransformed(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID))
          return static_cast<int>(xiiAssetInfo::COUNT);
        return static_cast<int>(pSubAsset->m_pAssetInfo->m_TransformState);
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

  if (iRole != Qt::EditRole || index.column() != NameColumn)
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
    flags |= Qt::ItemIsDragEnabled;
    if (index.column() == NameColumn)
      flags |= Qt::ItemIsEditable;
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
      case NameColumn:
        return tr("Name");
      case TypeColumn:
        return tr("Type");
      case StatusColumn:
        return tr("Status");
      case SizeColumn:
        return tr("Size");
      case ModifiedColumn:
        return tr("Modified");
      case PathColumn:
        return tr("Path");
    }
  }
  return QVariant();
}

QModelIndex xiiQtAssetBrowserModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (!hasIndex(iRow, iColumn, parent))
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
  return parent.isValid() ? 0 : ColumnCount;
}

void xiiQtAssetBrowserModel::sort(int iColumn, Qt::SortOrder order)
{
  if (iColumn < NameColumn || iColumn >= ColumnCount || iColumn == StatusColumn || iColumn == SizeColumn ||
      (m_iSortColumn == iColumn && m_SortOrder == order))
    return;

  m_iSortColumn = iColumn;
  m_SortOrder   = order;

  xiiDynamicArray<xiiUInt32> rows;
  rows.SetCountUninitialized(m_EntriesToDisplay.GetCount());
  for (xiiUInt32 i = 0; i < rows.GetCount(); ++i)
    rows[i] = i;
  {
    const auto         assets = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
    const FileComparer comparer(this, *assets.Borrow());
    rows.Sort([&](xiiUInt32 a, xiiUInt32 b) { return comparer(m_EntriesToDisplay[a], m_EntriesToDisplay[b]); });
  }

  Q_EMIT layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
  const auto                 oldIndexes = persistentIndexList();
  xiiDynamicArray<xiiUInt32> newRows;
  newRows.SetCountUninitialized(rows.GetCount());
  xiiDynamicArray<VisibleEntry> sorted;
  sorted.Reserve(rows.GetCount());
  for (xiiUInt32 i = 0; i < rows.GetCount(); ++i)
  {
    newRows[rows[i]] = i;
    sorted.PushBack(std::move(m_EntriesToDisplay[rows[i]]));
  }
  m_EntriesToDisplay.Swap(sorted);

  QModelIndexList newIndexes;
  newIndexes.reserve(oldIndexes.size());
  for (const auto& item : oldIndexes)
    newIndexes.push_back(index(newRows[item.row()], item.column()));

  changePersistentIndexList(oldIndexes, newIndexes);
  Q_EMIT layoutChanged({}, QAbstractItemModel::VerticalSortHint);
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
    if (indexes[i].column() != NameColumn)
      continue;

    QString sGuid(xiiConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<xiiUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();
    guids.PushBack(sGuid);
    if (sGuids.isEmpty())
      sGuids += sPath;
    else
      sGuids += "\n" + sPath;

    const xiiBitflags<xiiAssetBrowserItemFlags> itemFlags = static_cast<xiiAssetBrowserItemFlags::Enum>(data(indexes[i], ItemFlags).toInt());
    if (!itemFlags.IsAnySet(xiiAssetBrowserItemFlags::SubAsset | xiiAssetBrowserItemFlags::DataDirectory) && !files.Contains(sPath))
    {
      files.PushBack(sPath);
      urls.push_back(QUrl::fromLocalFile(sPath));
    }
  }

  QByteArray  encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << guids;

  QByteArray  encodedData2;
  QDataStream stream2(&encodedData2, QIODevice::WriteOnly);
  stream2 << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/xiiEditor.AssetGuid", encodedData);
  if (!files.IsEmpty())
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
  ve.m_LastModified = e.m_Status.m_LastModified;
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
      ve.m_Guid  = xiiUuid::MakeInvalid();
      ve.m_Flags = xiiAssetBrowserItemFlags::File;
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid  = e.m_Status.m_DocumentID;
      ve.m_Flags = xiiAssetBrowserItemFlags::File | xiiAssetBrowserItemFlags::Asset;
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case xiiFileChangedEvent::Type::DocumentUnlinked:
    {
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid  = xiiUuid::MakeInvalid();
      ve.m_Flags = xiiAssetBrowserItemFlags::File;
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case xiiFileChangedEvent::Type::FileChanged:
      HandleEntry(ve, AssetOp::Updated);
      return;
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
  ve.m_Flags        = e.m_Path.GetDataDirRelativePath().IsEmpty() ? xiiAssetBrowserItemFlags::DataDirectory : xiiAssetBrowserItemFlags::Folder;
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
