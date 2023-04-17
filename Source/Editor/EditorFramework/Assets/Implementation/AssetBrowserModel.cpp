#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

xiiQtAssetFilter::xiiQtAssetFilter(QObject* pParent) :
  QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// xiiQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct AssetComparer
{
  AssetComparer(xiiQtAssetBrowserModel* pModel, const xiiHashTable<xiiUuid, xiiSubAsset>& allAssets) :
    m_Model(pModel), m_AllAssets(allAssets)
  {
  }

  XII_ALWAYS_INLINE bool Less(const xiiQtAssetBrowserModel::AssetEntry& a, const xiiQtAssetBrowserModel::AssetEntry& b) const
  {
    const xiiSubAsset* pInfoA = &m_AllAssets.Find(a.m_Guid).Value();
    const xiiSubAsset* pInfoB = &m_AllAssets.Find(b.m_Guid).Value();

    return m_Model->m_pFilter->Less(pInfoA, pInfoB);
  }

  XII_ALWAYS_INLINE bool operator()(const xiiQtAssetBrowserModel::AssetEntry& a, const xiiQtAssetBrowserModel::AssetEntry& b) const
  {
    return Less(a, b);
  }

  xiiQtAssetBrowserModel*                   m_Model;
  const xiiHashTable<xiiUuid, xiiSubAsset>& m_AllAssets;
};

xiiQtAssetBrowserModel::xiiQtAssetBrowserModel(QObject* pParent, xiiQtAssetFilter* pFilter) :
  QAbstractItemModel(pParent), m_pFilter(pFilter)
{
  XII_ASSERT_DEBUG(pFilter != nullptr, "xiiQtAssetBrowserModel requires a valid filter.");
  connect(pFilter, &xiiQtAssetFilter::FilterChanged, this, [this]() { resetModel(); });

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  XII_VERIFY(connect(xiiQtImageCache::GetSingleton(), &xiiQtImageCache::ImageLoaded, this, &xiiQtAssetBrowserModel::ThumbnailLoaded) != nullptr,
             "signal/slot connection failed");
  XII_VERIFY(connect(xiiQtImageCache::GetSingleton(), &xiiQtImageCache::ImageInvalidated, this, &xiiQtAssetBrowserModel::ThumbnailInvalidated) != nullptr,
             "signal/slot connection failed");
}

xiiQtAssetBrowserModel::~xiiQtAssetBrowserModel()
{
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void xiiQtAssetBrowserModel::AssetCuratorEventHandler(const xiiAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAssetCuratorEvent::Type::AssetAdded:
      HandleAsset(e.m_pInfo, AssetOp::Add);
      break;
    case xiiAssetCuratorEvent::Type::AssetRemoved:
      HandleAsset(e.m_pInfo, AssetOp::Remove);
      break;
    case xiiAssetCuratorEvent::Type::AssetListReset:
      resetModel();
      break;
    case xiiAssetCuratorEvent::Type::AssetUpdated:
      HandleAsset(e.m_pInfo, AssetOp::Updated);
      break;
    default:
      break;
  }
}


xiiInt32 xiiQtAssetBrowserModel::FindAssetIndex(const xiiUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (xiiUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}

void xiiQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  xiiAssetCurator::xiiLockedSubAssetTable   AllAssetsLocked = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  const xiiHashTable<xiiUuid, xiiSubAsset>& AllAssets       = *(AllAssetsLocked.operator->());

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());
  m_DisplayedEntries.Clear();

  AssetEntry ae;
  // last access > filename
  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    const xiiSubAsset* pSub = &it.Value();
    if (m_pFilter->IsAssetFiltered(pSub))
      continue;

    Init(ae, pSub);

    m_AssetsToDisplay.PushBack(ae);
    m_DisplayedEntries.Insert(ae.m_Guid);
  }

  AssetComparer cmp(this, AllAssets);
  m_AssetsToDisplay.Sort(cmp);

  endResetModel();
  XII_ASSERT_DEBUG(m_AssetsToDisplay.GetCount() == m_DisplayedEntries.GetCount(), "Implementation error: Set and sorted list diverged");
}

void xiiQtAssetBrowserModel::HandleAsset(const xiiSubAsset* pInfo, AssetOp op)
{
  if (m_pFilter->IsAssetFiltered(pInfo))
  {
    // TODO: Due to file system watcher weirdness the m_sDataDirRelativePath can be empty at this point when renaming files
    // really rare haven't reproed it yet but that case crashes when getting the name so early out that.
    if (!m_DisplayedEntries.Contains(pInfo->m_Data.m_Guid) || pInfo->m_pAssetInfo->m_sDataDirParentRelativePath.IsEmpty())
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  xiiAssetCurator::xiiLockedSubAssetTable   AllAssetsLocked = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
  const xiiHashTable<xiiUuid, xiiSubAsset>& AllAssets       = *(AllAssetsLocked.operator->());

  AssetEntry ae;
  Init(ae, pInfo);

  AssetComparer cmp(this, AllAssets);
  AssetEntry*   pLB           = std::lower_bound(begin(m_AssetsToDisplay), end(m_AssetsToDisplay), ae, cmp);
  xiiUInt32     uiInsertIndex = pLB - m_AssetsToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model ontop of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_AssetsToDisplay.GetCount())
  {
    for (xiiUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); i++)
    {
      AssetEntry& displayEntry = m_AssetsToDisplay[i];
      if (!cmp.Less(displayEntry, ae) && !cmp.Less(ae, displayEntry))
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
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_AssetsToDisplay.Insert(ae, uiInsertIndex);
    m_DisplayedEntries.Insert(pInfo->m_Data.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_AssetsToDisplay.RemoveAtAndCopy(uiInsertIndex);
      m_DisplayedEntries.Remove(pInfo->m_Data.m_Guid);
      endRemoveRows();
    }
  }
  else
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT      dataChanged(idx, idx);
    }
    else
    {
      xiiInt32 oldIndex = FindAssetIndex(pInfo->m_Data.m_Guid);
      if (oldIndex != -1)
      {
        // Name has changed, remove old entry
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_AssetsToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(pInfo->m_Data.m_Guid);
        endRemoveRows();
      }
      HandleAsset(pInfo, AssetOp::Add);
    }
  }
  XII_ASSERT_DEBUG(m_AssetsToDisplay.GetCount() == m_DisplayedEntries.GetCount(), "Implementation error: Set and sorted list diverged");
}


////////////////////////////////////////////////////////////////////////
// xiiQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void xiiQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const xiiUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  for (xiiUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT      dataChanged(idx, idx);
      return;
    }
  }
}

void xiiQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID)
{
  for (xiiUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT      dataChanged(idx, idx);
      return;
    }
  }
}

QVariant xiiQtAssetBrowserModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const xiiInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (xiiInt32)m_AssetsToDisplay.GetCount())
    return QVariant();

  const auto&                              asset     = m_AssetsToDisplay[iRow];
  const xiiUuid                            AssetGuid = asset.m_Guid;
  const xiiAssetCurator::xiiLockedSubAsset pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

  // this can happen when a file was just changed on disk, e.g. got deleted
  if (pSubAsset == nullptr)
    return QVariant();

  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      xiiStringBuilder sFilename = pSubAsset->GetName();
      return QString::fromUtf8(sFilename, sFilename.GetElementCount());
    }
    break;

    case Qt::ToolTipRole:
    {
      xiiStringBuilder sToolTip = pSubAsset->GetName();
      sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_sDataDirParentRelativePath);
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
        xiiStringBuilder sThumbnailPath = xiiAssetDocumentManager::GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_sAbsolutePath);

        xiiUInt64 uiUserData1, uiUserData2;
        AssetGuid.GetValues(uiUserData1, uiUserData2);

        const QPixmap* pThumbnailPixmap = xiiQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName,
                                                                                              sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &asset.m_uiThumbnailID);

        return *pThumbnailPixmap;
      }
      else
      {
        return xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon);
      }
    }
    break;

    case UserRoles::SubAssetGuid:
    {
      return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
    }
    case UserRoles::AssetGuid:
    {
      return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
    }
    case UserRoles::AbsolutePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sAbsolutePath, pSubAsset->m_pAssetInfo->m_sAbsolutePath.GetElementCount());

    case UserRoles::RelativePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sDataDirParentRelativePath, pSubAsset->m_pAssetInfo->m_sDataDirParentRelativePath.GetElementCount());

    case UserRoles::AssetIconPath:
      return xiiQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon);

    case UserRoles::TransformState:
      return (int)pSubAsset->m_pAssetInfo->m_TransformState;
  }

  return QVariant();
}

Qt::ItemFlags xiiQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant xiiQtAssetBrowserModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Asset");
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

  return (int)m_AssetsToDisplay.GetCount();
}

int xiiQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList xiiQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/xiiEditor.AssetGuid";
  return types;
}

QMimeData* xiiQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData*  mimeData = new QMimeData();
  QByteArray  encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString     sGuids;
  QList<QUrl> urls;

  xiiStringBuilder tmp;

  stream << (int)indexes.size();
  for (int i = 0; i < indexes.size(); ++i)
  {
    QString sGuid(xiiConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<xiiUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();

    stream << sGuid;
    sGuids += sPath + "\n";

    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  mimeData->setData("application/xiiEditor.AssetGuid", encodedData);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

void xiiQtAssetBrowserModel::Init(AssetEntry& ae, const xiiSubAsset* pInfo)
{
  ae.m_Guid          = pInfo->m_Data.m_Guid;
  ae.m_uiThumbnailID = (xiiUInt32)-1;
}
