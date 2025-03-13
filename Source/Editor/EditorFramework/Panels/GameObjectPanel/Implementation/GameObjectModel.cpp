#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>

xiiQtGameObjectAdapter::xiiQtGameObjectAdapter(xiiDocumentObjectManager* pObjectManager, xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>* pObjectMetaData, xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>* pGameObjectMetaData) :
  xiiQtNameableAdapter(pObjectManager, xiiGetStaticRTTI<xiiGameObject>(), "Children", "Name")
{
  m_pObjectManager      = pObjectManager;
  m_pGameObjectDocument = xiiDynamicCast<xiiGameObjectDocument*>(pObjectManager->GetDocument());

  m_pObjectMetaData = pObjectMetaData;
  if (!m_pObjectMetaData)
    m_pObjectMetaData = m_pGameObjectDocument->m_DocumentObjectMetaData.Borrow();

  m_pGameObjectMetaData = pGameObjectMetaData;
  if (!m_pGameObjectMetaData)
    m_pGameObjectMetaData = m_pGameObjectDocument->m_GameObjectMetaData.Borrow();

  m_GameObjectMetaDataSubscription     = m_pGameObjectMetaData->m_DataModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiQtGameObjectAdapter::GameObjectMetaDataEventHandler, this));
  m_DocumentObjectMetaDataSubscription = m_pObjectMetaData->m_DataModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiQtGameObjectAdapter::DocumentObjectMetaDataEventHandler, this));
}

xiiQtGameObjectAdapter::~xiiQtGameObjectAdapter()
{
  m_pGameObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(m_GameObjectMetaDataSubscription);
  m_pObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(m_DocumentObjectMetaDataSubscription);
}

QVariant xiiQtGameObjectAdapter::data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      xiiStringBuilder sName;
      xiiUuid          prefabGuid;
      QIcon            icon;

      m_pGameObjectDocument->QueryCachedNodeName(pObject, sName, &prefabGuid, &icon);

      const QString sQtName = QString::fromUtf8(sName.GetData());

      if (prefabGuid.IsValid())
        return QStringLiteral("[") + sQtName + QStringLiteral("]");

      return sQtName;
    }
    break;

    case Qt::DecorationRole:
    {
      xiiStringBuilder sName;
      xiiUuid          prefabGuid;
      QIcon            icon;

      m_pGameObjectDocument->QueryCachedNodeName(pObject, sName, &prefabGuid, &icon);
      return icon;
    }
    break;

    case Qt::EditRole:
    {
      xiiStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();

      if (sName.IsEmpty())
      {
        auto pMeta = m_pGameObjectMetaData->BeginReadMetaData(pObject->GetGuid());
        sName      = pMeta->m_CachedNodeName;
        m_pGameObjectMetaData->EndReadMetaData();
      }

      return QString::fromUtf8(sName.GetData());
    }
    break;

    case Qt::ToolTipRole:
    {
      auto          pMeta  = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const xiiUuid prefab = pMeta->m_CreateFromPrefab;
      m_pObjectMetaData->EndReadMetaData();

      if (prefab.IsValid())
      {
        auto pInfo = xiiAssetCurator::GetSingleton()->GetSubAsset(prefab);

        if (pInfo)
          return xiiMakeQString(pInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

        return QString::fromUtf8("Prefab asset could not be found");
      }
    }
    break;

    case Qt::FontRole:
    {
      auto       pMeta   = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const bool bHidden = pMeta->m_bHidden;
      m_pObjectMetaData->EndReadMetaData();

      const bool bHasName = !pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>().IsEmpty();

      if (bHidden || bHasName)
      {
        QFont font;

        if (bHidden)
          font.setStrikeOut(true);
        if (bHasName)
          font.setBold(true);

        return font;
      }
    }
    break;

    case Qt::ForegroundRole:
    {
      xiiStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<xiiString>();

      auto       pMeta   = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const bool bPrefab = pMeta->m_CreateFromPrefab.IsValid();
      m_pObjectMetaData->EndReadMetaData();

      bool bActive = pObject->GetTypeAccessor().GetValue("Active").ConvertTo<bool>();

      const QPalette palette        = QApplication::palette();
      const QColor   qtDefaultColor = palette.color(QPalette::Text);

      xiiColor color = qtToXIIColor(qtDefaultColor);

      if (bPrefab)
      {
        color = xiiColorScheme::LightUI(xiiColorScheme::Blue);
      }

      if (!bActive)
      {
        return xiiToQtColor(color.GetDarker(1.85f));
      }

      return xiiToQtColor(color);
    }
    break;

    case UserRoles::HiddenRole:
    {
      auto       pMeta   = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const bool bHidden = pMeta->m_bHidden;
      m_pObjectMetaData->EndReadMetaData();

      return bHidden;
    }
    break;

    case UserRoles::ActiveParentRole:
    {
      return (pObject->GetGuid() == m_pGameObjectDocument->GetActiveParent());
    }
    break;
  }

  return xiiQtNameableAdapter::data(pObject, iRow, iColumn, iRole);
}

bool xiiQtGameObjectAdapter::setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  if (iRole == Qt::EditRole)
  {
    auto pMetaWrite = m_pGameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());

    xiiStringBuilder sNewValue = value.toString().toUtf8().data();

    const xiiStringBuilder sOldValue = pMetaWrite->m_CachedNodeName;

    // pMetaWrite->m_CachedNodeName.Clear();
    m_pGameObjectMetaData->EndModifyMetaData(0); // no need to broadcast this change

    if (sOldValue == sNewValue && !sOldValue.IsEmpty())
      return false;

    sNewValue.Trim("[]{}() \t\r"); // forbid these

    return xiiQtNameableAdapter::setData(pObject, iRow, iColumn, QString::fromUtf8(sNewValue.GetData()), iRole);
  }

  return false;
}

void xiiQtGameObjectAdapter::DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & (xiiDocumentObjectMetaData::HiddenFlag | xiiDocumentObjectMetaData::PrefabFlag | xiiDocumentObjectMetaData::ActiveParentFlag)) == 0)
    return;

  auto pObject = m_pObjectManager->GetObject(e.m_ObjectKey);

  if (pObject == nullptr)
  {
    // The object was destroyed due to a clear of the redo queue, i.e. it is not contained in the scene anymore.
    // So we of course won't find it in the model and thus can skip it.
    return;
  }

  // ignore all components etc.
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  QVector<int> v;
  v.push_back(Qt::FontRole);
  v.push_back(Qt::DecorationRole);
  v.push_back(Qt::ForegroundRole);
  dataChanged(pObject, v);
}

void xiiQtGameObjectAdapter::GameObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiGameObjectMetaData>::EventData& e)
{
  if (e.m_uiModifiedFlags == 0)
    return;

  auto pObject = m_pObjectManager->GetObject(e.m_ObjectKey);

  if (pObject == nullptr)
  {
    // The object was destroyed due to a clear of the redo queue, i.e. it is not contained in the scene anymore.
    // So we of course won't find it in the model and thus can skip it.
    return;
  }

  // ignore all components etc.
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<xiiGameObject>())
    return;

  QVector<int> v;
  v.push_back(Qt::FontRole);
  dataChanged(pObject, v);
}

xiiQtGameObjectModel::xiiQtGameObjectModel(const xiiDocumentObjectManager* pObjectManager, const xiiUuid& root) :
  xiiQtDocumentTreeModel(pObjectManager, root)
{
}

xiiQtGameObjectModel::~xiiQtGameObjectModel() = default;

//////////////////////////////////////////////////////////////////////////


xiiQtGameObjectDelegate::xiiQtGameObjectDelegate(QObject* pParent, xiiGameObjectDocument* pDocument) :
  xiiQtItemDelegate(pParent), m_pDocument(pDocument)
{
}

void xiiQtGameObjectDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  xiiQtItemDelegate::paint(pPainter, option, index);

  QPoint mousePos;
  if (QWidget* pParent = qobject_cast<QWidget*>(parent()))
  {
    mousePos = pParent->mapFromGlobal(QCursor::pos());
  }

  {
    const bool  bIsHidden = index.data(xiiQtGameObjectAdapter::UserRoles::HiddenRole).value<bool>();
    const QRect iconRect  = GetHiddenIconRect(option);

    if (bIsHidden)
    {
      xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ObjectsHidden.svg").paint(pPainter, iconRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
    }
  }

  {
    const bool  bIsActiveParent = index.data(xiiQtGameObjectAdapter::UserRoles::ActiveParentRole).value<bool>();
    const QRect iconRect        = GetActiveParentIconRect(option);

    if (bIsActiveParent)
    {
      xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ActiveParent.svg").paint(pPainter, iconRect, Qt::AlignmentFlag::AlignCenter, QIcon::Mode::Normal);
    }
  }
}

bool xiiQtGameObjectDelegate::helpEvent(QHelpEvent* pEvent, QAbstractItemView* pView, const QStyleOptionViewItem& option, const QModelIndex& index)
{
  const QRect hiddenRect       = GetHiddenIconRect(option);
  const QRect activeParentRect = GetActiveParentIconRect(option);

  if (hiddenRect.contains(pEvent->pos()))
  {
    const bool bIsHidden = index.data(xiiQtGameObjectAdapter::UserRoles::HiddenRole).value<bool>();

    if (bIsHidden)
    {
      QToolTip::showText(pEvent->globalPos(), "Object is hidden. It is not rendered in the viewport.");
      return true;
    }
  }
  else if (activeParentRect.contains(pEvent->pos()))
  {
    const bool bIsActiveParent = index.data(xiiQtGameObjectAdapter::UserRoles::ActiveParentRole).value<bool>();

    if (bIsActiveParent)
    {
      QToolTip::showText(pEvent->globalPos(), "This is the 'active parent' object. All new objects will be created below this.");
      return true;
    }
  }

  return xiiQtItemDelegate::helpEvent(pEvent, pView, option, index);
}

QRect xiiQtGameObjectDelegate::GetHiddenIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height(), 0, 0, 0);
}

QRect xiiQtGameObjectDelegate::GetActiveParentIconRect(const QStyleOptionViewItem& opt)
{
  return opt.rect.adjusted(opt.rect.width() - opt.rect.height() * 2, 0, -opt.rect.height(), 0);
}
