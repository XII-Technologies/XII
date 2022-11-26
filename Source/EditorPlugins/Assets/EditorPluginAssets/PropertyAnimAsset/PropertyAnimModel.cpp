#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>

xiiQtPropertyAnimModel::xiiQtPropertyAnimModel(xiiPropertyAnimAssetDocument* pDocument, QObject* pParent) :
  QAbstractItemModel(pParent), m_pAssetDoc(pDocument)
{
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimModel::DocumentStructureEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimModel::DocumentPropertyEventHandler, this));

  TriggerBuildMapping();
}

xiiQtPropertyAnimModel::~xiiQtPropertyAnimModel()
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimModel::DocumentPropertyEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtPropertyAnimModel::DocumentStructureEventHandler, this));
}

QVariant xiiQtPropertyAnimModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  xiiQtPropertyAnimModelTreeEntry* pItem = static_cast<xiiQtPropertyAnimModelTreeEntry*>(index.internalPointer());
  XII_ASSERT_DEBUG(pItem != nullptr, "Invalid model index");

  switch (role)
  {
    case Qt::DisplayRole:
      return QString(pItem->m_sDisplay.GetData());

    case Qt::DecorationRole:
      return pItem->m_Icon;

    case UserRoles::TrackPtr:
      return QVariant::fromValue((void*)pItem->m_pTrack);

    case UserRoles::TreeItem:
      return QVariant::fromValue((void*)pItem);

    case UserRoles::TrackIdx:
      return pItem->m_iTrackIdx;

    case UserRoles::Path:
      return QString(pItem->m_sPathToItem.GetData());
  }

  return QVariant();
}

Qt::ItemFlags xiiQtPropertyAnimModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex xiiQtPropertyAnimModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (column != 0)
    return QModelIndex();

  xiiQtPropertyAnimModelTreeEntry* pParentItem = static_cast<xiiQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  if (pParentItem != nullptr)
  {
    return createIndex(row, column, (void*)&m_AllEntries[m_iInUse][pParentItem->m_Children[row]]);
  }
  else
  {
    if (row >= (int)m_TopLevelEntries[m_iInUse].GetCount())
      return QModelIndex();

    return createIndex(row, column, (void*)&m_AllEntries[m_iInUse][m_TopLevelEntries[m_iInUse][row]]);
  }
}

QModelIndex xiiQtPropertyAnimModel::parent(const QModelIndex& index) const
{
  if (!index.isValid() || index.column() != 0)
    return QModelIndex();

  xiiQtPropertyAnimModelTreeEntry* pItem = static_cast<xiiQtPropertyAnimModelTreeEntry*>(index.internalPointer());

  if (pItem->m_iParent < 0)
    return QModelIndex();

  return createIndex(m_AllEntries[m_iInUse][pItem->m_iParent].m_uiOwnRowIndex, index.column(), (void*)&m_AllEntries[m_iInUse][pItem->m_iParent]);
}

int xiiQtPropertyAnimModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (!parent.isValid())
    return m_TopLevelEntries[m_iInUse].GetCount();

  xiiQtPropertyAnimModelTreeEntry* pItem = static_cast<xiiQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  return pItem->m_Children.GetCount();
}

int xiiQtPropertyAnimModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 1;
}

void xiiQtPropertyAnimModel::DocumentStructureEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      TriggerBuildMapping();
      break;

    default:
      break;
  }
}

void xiiQtPropertyAnimModel::DocumentPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_sProperty == "ObjectPath")
    {
      TriggerBuildMapping();
      return;
    }
  }
}

void xiiQtPropertyAnimModel::TriggerBuildMapping()
{
  if (m_bBuildMappingQueued)
    return;

  m_bBuildMappingQueued = true;
  QTimer::singleShot(100, this, SLOT(onBuildMappingTriggered()));
}

void xiiQtPropertyAnimModel::onBuildMappingTriggered()
{
  BuildMapping();
  m_bBuildMappingQueued = false;
}

void xiiQtPropertyAnimModel::BuildMapping()
{
  const xiiInt32 iToUse = (m_iInUse + 1) % 2;
  BuildMapping(iToUse);

  if (m_AllEntries[0] != m_AllEntries[1])
  {
    beginResetModel();
    m_iInUse = iToUse;
    endResetModel();
  }
}

void xiiQtPropertyAnimModel::BuildMapping(xiiInt32 iToUse)
{
  m_TopLevelEntries[iToUse].Clear();
  m_AllEntries[iToUse].Clear();

  const xiiPropertyAnimationTrackGroup& group = *m_pAssetDoc->GetProperties();

  xiiStringBuilder tmp;

  for (xiiUInt32 tIdx = 0; tIdx < group.m_Tracks.GetCount(); ++tIdx)
  {
    xiiPropertyAnimationTrack* pTrack = group.m_Tracks[tIdx];

    tmp = pTrack->m_sObjectSearchSequence;
    if (!pTrack->m_sComponentType.IsEmpty())
    {
      tmp.AppendPath(":");
      tmp.Append(pTrack->m_sComponentType.GetData());
    }
    tmp.AppendPath(pTrack->m_sPropertyPath);

    BuildMapping(iToUse, tIdx, pTrack, m_TopLevelEntries[iToUse], -1, tmp);
  }
}

void xiiQtPropertyAnimModel::BuildMapping(
  xiiInt32                   iToUse,
  xiiInt32                   iTrackIdx,
  xiiPropertyAnimationTrack* pTrack,
  xiiDynamicArray<xiiInt32>& treeItems,
  xiiInt32                   iParentEntry,
  const char*                szPath)
{
  const char* szSubPath = xiiStringUtils::FindSubString(szPath, "/");

  xiiStringBuilder name, sDisplayString;

  bool bIsComponent = false;
  if (szPath[0] == ':')
  {
    ++szPath;
    bIsComponent = true;
  }

  if (szSubPath != nullptr)
    name.SetSubString_FromTo(szPath, szSubPath);
  else
    name = szPath;

  if (bIsComponent)
    sDisplayString = xiiTranslate(name);
  else
    sDisplayString = name;

  xiiInt32 iThisEntry = -1;

  for (xiiUInt32 i = 0; i < treeItems.GetCount(); ++i)
  {
    if (m_AllEntries[iToUse][treeItems[i]].m_sDisplay.IsEqual_NoCase(sDisplayString))
    {
      iThisEntry = treeItems[i];
      break;
    }
  }

  xiiQtPropertyAnimModelTreeEntry* pThisEntry = nullptr;

  if (iThisEntry < 0)
  {
    pThisEntry = &m_AllEntries[iToUse].ExpandAndGetRef();
    iThisEntry = m_AllEntries[iToUse].GetCount() - 1;
    treeItems.PushBack(iThisEntry);

    pThisEntry->m_iParent       = iParentEntry;
    pThisEntry->m_uiOwnRowIndex = treeItems.GetCount() - 1;
    pThisEntry->m_sDisplay      = sDisplayString;

    if (bIsComponent)
    {
      sDisplayString.Set(":/TypeIcons/", name);
      pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(sDisplayString);
    }

    if (iParentEntry >= 0)
    {
      xiiStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
  else
  {
    pThisEntry = &m_AllEntries[iToUse][iThisEntry];
  }

  if (szSubPath != nullptr)
  {
    szSubPath += 1;
    BuildMapping(iToUse, iTrackIdx, pTrack, pThisEntry->m_Children, iThisEntry, szSubPath);
  }
  else
  {
    pThisEntry->m_iTrackIdx = iTrackIdx;
    pThisEntry->m_pTrack    = pTrack;

    switch (pTrack->m_Target)
    {
      case xiiPropertyAnimTarget::Color:
        pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/ColorGradient.png");
        break;
      case xiiPropertyAnimTarget::Number:
        pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/Curve1D.png");
        break;
      case xiiPropertyAnimTarget::VectorX:
      case xiiPropertyAnimTarget::RotationX:
        pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveX.png");
        name.Append(".x");
        break;
      case xiiPropertyAnimTarget::VectorY:
      case xiiPropertyAnimTarget::RotationY:
        pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.png");
        name.Append(".y");
        break;
      case xiiPropertyAnimTarget::VectorZ:
      case xiiPropertyAnimTarget::RotationZ:
        pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveZ.png");
        name.Append(".z");
        break;
      case xiiPropertyAnimTarget::VectorW:
        pThisEntry->m_Icon = xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveW.png");
        name.Append(".w");
        break;
    }

    pThisEntry->m_sDisplay = name;

    if (iParentEntry >= 0)
    {
      xiiStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
}
