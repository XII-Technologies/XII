#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

xiiQtDocumentTreeModelAdapter::xiiQtDocumentTreeModelAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty) :
  m_pTree(pTree), m_pType(pType), m_sChildProperty(szChildProperty)
{
  if (!m_sChildProperty.IsEmpty())
  {
    auto pProp = pType->FindPropertyByName(m_sChildProperty);
    XII_ASSERT_DEV(pProp != nullptr && (pProp->GetCategory() == xiiPropertyCategory::Array || pProp->GetCategory() == xiiPropertyCategory::Set), "The visualized object property tree must either be a set or array!");
    XII_ASSERT_DEV(!pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) || pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner), "The visualized object must have ownership of the property objects!");
  }
}

const xiiRTTI* xiiQtDocumentTreeModelAdapter::GetType() const
{
  return m_pType;
}

const xiiString& xiiQtDocumentTreeModelAdapter::GetChildProperty() const
{
  return m_sChildProperty;
}

bool xiiQtDocumentTreeModelAdapter::setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  return false;
}

Qt::ItemFlags xiiQtDocumentTreeModelAdapter::flags(const xiiDocumentObject* pObject, int iRow, int iColumn) const
{
  if (iColumn == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}

xiiQtDummyAdapter::xiiQtDummyAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty) :
  xiiQtDocumentTreeModelAdapter(pTree, pType, szChildProperty)
{
}

QVariant xiiQtDummyAdapter::data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  if (iColumn == 0)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        return xiiMakeQString(pObject->GetTypeAccessor().GetType()->GetTypeName());
      }
      break;
    }
  }
  return QVariant();
}

xiiQtNamedAdapter::xiiQtNamedAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty, const char* szNameProperty) :
  xiiQtDocumentTreeModelAdapter(pTree, pType, szChildProperty), m_sNameProperty(szNameProperty)
{
  auto pProp = pType->FindPropertyByName(m_sNameProperty);
  XII_ASSERT_DEV(pProp != nullptr && pProp->GetCategory() == xiiPropertyCategory::Member && (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::String || pProp->GetSpecificType()->GetVariantType() == xiiVariantType::StringView), "The name property must be a string member property.");

  m_pTree->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtNamedAdapter::TreePropertyEventHandler, this));
}

xiiQtNamedAdapter::~xiiQtNamedAdapter()
{
  m_pTree->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtNamedAdapter::TreePropertyEventHandler, this));
}

QVariant xiiQtNamedAdapter::data(const xiiDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  if (iColumn == 0)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetTypeAccessor().GetValue(m_sNameProperty).ConvertTo<xiiString>().GetData());
      }
      break;
    }
  }
  return QVariant();
}

void xiiQtNamedAdapter::TreePropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == m_sNameProperty)
  {
    QVector<int> v;
    v.push_back(Qt::DisplayRole);
    v.push_back(Qt::EditRole);
    Q_EMIT dataChanged(e.m_pObject, v);
  }
}

xiiQtNameableAdapter::xiiQtNameableAdapter(const xiiDocumentObjectManager* pTree, const xiiRTTI* pType, const char* szChildProperty, const char* szNameProperty) :
  xiiQtNamedAdapter(pTree, pType, szChildProperty, szNameProperty)
{
}

xiiQtNameableAdapter::~xiiQtNameableAdapter() = default;

bool xiiQtNameableAdapter::setData(const xiiDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  if (iColumn == 0 && iRole == Qt::EditRole)
  {
    auto pHistory = m_pTree->GetDocument()->GetCommandHistory();

    pHistory->StartTransaction(xiiFmt("Rename to '{0}'", value.toString().toUtf8().data()));

    xiiSetObjectPropertyCommand cmd;
    cmd.m_NewValue  = value.toString().toUtf8().data();
    cmd.m_Object    = pObject->GetGuid();
    cmd.m_sProperty = m_sNameProperty;

    pHistory->AddCommand(cmd).AssertSuccess();

    pHistory->FinishTransaction();

    return true;
  }
  return false;
}

Qt::ItemFlags xiiQtNameableAdapter::flags(const xiiDocumentObject* pObject, int iRow, int iColumn) const
{
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

  if (iColumn == 0)
  {
    return flags | Qt::ItemIsEditable;
  }

  return flags;
}

//////////////////////////////////////////////////////////////////////////

xiiQtDocumentTreeModel::xiiQtDocumentTreeModel(const xiiDocumentObjectManager* pTree, const xiiUuid& root) :
  QAbstractItemModel(nullptr), m_pDocumentTree(pTree), m_Root(root)
{
  m_pDocumentTree->m_StructureEvents.AddEventHandler(xiiMakeDelegate(&xiiQtDocumentTreeModel::TreeEventHandler, this));
}

xiiQtDocumentTreeModel::~xiiQtDocumentTreeModel()
{
  m_pDocumentTree->m_StructureEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtDocumentTreeModel::TreeEventHandler, this));
}

void xiiQtDocumentTreeModel::AddAdapter(xiiQtDocumentTreeModelAdapter* pAdapter)
{
  XII_ASSERT_DEV(!m_Adapters.Contains(pAdapter->GetType()), "An adapter for the given type was already registered.");

  pAdapter->setParent(this);
  connect(pAdapter, &xiiQtDocumentTreeModelAdapter::dataChanged, this, [this](const xiiDocumentObject* pObject, QVector<int> roles) {
    if (!pObject)
      return;
    auto index = ComputeModelIndex(pObject);
    if (!index.isValid())
      return;

    QModelIndex idx2 = index.siblingAtColumn(columnCount() - 1); // mark the entire row as modified
    Q_EMIT dataChanged(index, idx2, roles);
  });
  m_Adapters.Insert(pAdapter->GetType(), pAdapter);
  beginResetModel();
  endResetModel();
}

const xiiQtDocumentTreeModelAdapter* xiiQtDocumentTreeModel::GetAdapter(const xiiRTTI* pType) const
{
  while (pType != nullptr)
  {
    if (const xiiQtDocumentTreeModelAdapter* const* adapter = m_Adapters.GetValue(pType))
    {
      return *adapter;
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}

void xiiQtDocumentTreeModel::TreeEventHandler(const xiiDocumentObjectStructureEvent& e)
{
  const xiiDocumentObject* pParent = nullptr;
  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::BeforeReset:
      beginResetModel();
      return;
    case xiiDocumentObjectStructureEvent::Type::AfterReset:
      endResetModel();
      return;
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      pParent = e.m_pPreviousParent;
      break;
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      pParent = e.m_pNewParent;
      break;
  }
  XII_ASSERT_DEV(pParent != nullptr, "Each structure event should have a parent set.");
  if (!IsUnderRoot(pParent))
    return;
  auto pType    = pParent->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return;

  if (pAdapter->GetChildProperty() != e.m_sParentProperty)
    return;

  // TODO: BLA root object could have other objects instead of m_pBaseClass, in which case indices are broken on root.

  switch (e.m_EventType)
  {
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      xiiInt32 iIndex = (xiiInt32)e.m_NewPropertyIndex.ConvertTo<xiiInt32>();
      if (e.m_pNewParent == GetRoot())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      xiiInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      xiiInt32 iNewIndex = (xiiInt32)e.m_NewPropertyIndex.ConvertTo<xiiInt32>();
      xiiInt32 iIndex    = ComputeIndex(e.m_pObject);
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), iNewIndex);
    }
    break;
    case xiiDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      endMoveRows();
    }
    break;
    default:
      break;
  }
}

QModelIndex xiiQtDocumentTreeModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  const xiiDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const xiiDocumentObject*)parent.internalPointer();
  }

  auto pType    = pObject->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();
  if (iRow >= pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty()))
    return QModelIndex();

  xiiVariant value = pObject->GetTypeAccessor().GetValue(pAdapter->GetChildProperty(), iRow);
  XII_ASSERT_DEV(value.IsValid() && value.IsA<xiiUuid>(), "Tree corruption!");
  const xiiDocumentObject* pChild = m_pDocumentTree->GetObject(value.Get<xiiUuid>());
  return createIndex(iRow, iColumn, const_cast<xiiDocumentObject*>(pChild));
}

xiiInt32 xiiQtDocumentTreeModel::ComputeIndex(const xiiDocumentObject* pObject) const
{
  xiiInt32 iIndex = pObject->GetPropertyIndex().ConvertTo<xiiInt32>();
  return iIndex;
}

const xiiDocumentObject* xiiQtDocumentTreeModel::GetRoot() const
{
  if (m_Root.IsValid())
  {
    return m_pDocumentTree->GetObject(m_Root);
  }
  return m_pDocumentTree->GetRootObject();
}

bool xiiQtDocumentTreeModel::IsUnderRoot(const xiiDocumentObject* pObject) const
{
  const xiiDocumentObject* pRoot = GetRoot();
  while (pObject)
  {
    if (pRoot == pObject)
      return true;

    pObject = pObject->GetParent();
  }
  return false;
}

QModelIndex xiiQtDocumentTreeModel::ComputeModelIndex(const xiiDocumentObject* pObject) const
{
  // Filter out objects that are not under the child property of the
  // parents adapter.
  if (pObject == GetRoot())
    return QModelIndex();

  auto pType    = pObject->GetParent()->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();

  if (pAdapter->GetChildProperty() != pObject->GetParentProperty())
    return QModelIndex();

  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
}

void xiiQtDocumentTreeModel::SetAllowDragDrop(bool bAllow)
{
  m_bAllowDragDrop = bAllow;
}

QModelIndex xiiQtDocumentTreeModel::ComputeParent(const xiiDocumentObject* pObject) const
{
  const xiiDocumentObject* pParent = pObject->GetParent();

  if (pParent == GetRoot())
    return QModelIndex();

  xiiInt32 iIndex = ComputeIndex(pParent);

  return createIndex(iIndex, 0, const_cast<xiiDocumentObject*>(pParent));
}

QModelIndex xiiQtDocumentTreeModel::parent(const QModelIndex& child) const
{
  const xiiDocumentObject* pObject = (const xiiDocumentObject*)child.internalPointer();

  return ComputeParent(pObject);
}

int xiiQtDocumentTreeModel::rowCount(const QModelIndex& parent) const
{
  int                      iCount  = 0;
  const xiiDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const xiiDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    if (!pAdapter->GetChildProperty().IsEmpty())
    {
      iCount = pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty());
    }
  }

  return iCount;
}

int xiiQtDocumentTreeModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant xiiQtDocumentTreeModel::data(const QModelIndex& index, int iRole) const
{
  // if (index.isValid())
  {
    const xiiDocumentObject* pObject = (const xiiDocumentObject*)index.internalPointer();
    auto                     pType   = pObject->GetTypeAccessor().GetType();
    if (auto pAdapter = GetAdapter(pType))
    {
      return pAdapter->data(pObject, index.row(), index.column(), iRole);
    }
  }

  return QVariant();
}

Qt::DropActions xiiQtDocumentTreeModel::supportedDropActions() const
{
  if (m_bAllowDragDrop)
    return Qt::MoveAction | Qt::CopyAction;

  return Qt::IgnoreAction;
}

Qt::ItemFlags xiiQtDocumentTreeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  const xiiDocumentObject* pObject = (const xiiDocumentObject*)index.internalPointer();
  auto                     pType   = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->flags(pObject, index.row(), index.column());
  }

  return Qt::ItemFlag::NoItemFlags;
}

bool xiiQtDocumentTreeModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) const
{
  const xiiDocumentObject* pNewParent = (const xiiDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  xiiDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = iRow;
  info.m_pMimeData                     = pData;
  info.m_sTargetContext                = m_sTargetContext;
  info.m_TargetDocument                = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject                  = pNewParent->GetGuid();
  info.m_bCtrlKeyDown                  = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown                 = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter                      = GetAdapter(pNewParent->GetType());

  if (xiiDragDropHandler::CanDropOnly(&info))
    return true;

  {
    // Test 'CanMove' of the target object manager.
    QByteArray                             encodedData = pData->data("application/xiiEditor.ObjectSelection");
    QDataStream                            stream(&encodedData, QIODevice::ReadOnly);
    xiiHybridArray<xiiDocumentObject*, 32> Dragged;
    stream >> Dragged;

    auto             pType     = pNewParent->GetTypeAccessor().GetType();
    auto             pAdapter  = GetAdapter(pType);
    const xiiString& sProperty = pAdapter->GetChildProperty();
    for (const xiiDocumentObject* pItem : Dragged)
    {
      // If the item's and the target tree's document don't match we can't operate via this code.
      if (pItem->GetDocumentObjectManager()->GetDocument() != m_pDocumentTree->GetDocument())
        return false;
      if (m_pDocumentTree->CanMove(pItem, pNewParent, sProperty, info.m_iTargetObjectInsertChildIndex).Failed())
        return false;
    }
    return QAbstractItemModel::canDropMimeData(pData, action, iRow, iColumn, parent);
  }
  return false;
}

bool xiiQtDocumentTreeModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent)
{
  if (!m_bAllowDragDrop)
    return false;

  if (iColumn > 0)
    return false;

  const xiiDocumentObject* pNewParent = (const xiiDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  xiiDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = iRow;
  info.m_pMimeData                     = pData;
  info.m_sTargetContext                = m_sTargetContext;
  info.m_TargetDocument                = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject                  = pNewParent->GetGuid();
  info.m_bCtrlKeyDown                  = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown                 = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter                      = GetAdapter(pNewParent->GetType());
  if (xiiDragDropHandler::DropOnly(&info))
    return true;

  return xiiQtDocumentTreeModel::MoveObjects(info);
}

bool xiiQtDocumentTreeModel::MoveObjects(const xiiDragDropInfo& info)
{
  if (info.m_pMimeData->hasFormat("application/xiiEditor.ObjectSelection"))
  {
    auto                     pDoc    = xiiDocumentManager::GetDocumentByGuid(info.m_TargetDocument);
    const xiiDocumentObject* pTarget = pDoc->GetObjectManager()->GetObject(info.m_TargetObject);
    XII_ASSERT_DEBUG(pTarget != nullptr, "object from info should always be valid");

    QByteArray                             encodedData = info.m_pMimeData->data("application/xiiEditor.ObjectSelection");
    QDataStream                            stream(&encodedData, QIODevice::ReadOnly);
    xiiHybridArray<xiiDocumentObject*, 32> Dragged;
    stream >> Dragged;

    for (const xiiDocumentObject* pDocObject : Dragged)
    {
      // if (action != Qt::DropAction::MoveAction)
      {
        bool                     bCanMove   = true;
        const xiiDocumentObject* pCurParent = pTarget;

        while (pCurParent)
        {
          if (pCurParent == pDocObject)
          {
            bCanMove = false;
            break;
          }

          pCurParent = pCurParent->GetParent();
        }

        if (!bCanMove)
        {
          xiiQtUiServices::MessageBoxInformation("Cannot move an object to one of its own children");
          return false;
        }
      }
    }

    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Reparent Object");

    xiiStatus res(XII_SUCCESS);
    for (xiiUInt32 i = 0; i < Dragged.GetCount(); ++i)
    {
      xiiMoveObjectCommand cmd;
      cmd.m_Object          = Dragged[i]->GetGuid();
      cmd.m_Index           = info.m_iTargetObjectInsertChildIndex;
      cmd.m_sParentProperty = info.m_pAdapter->GetChildProperty();
      cmd.m_NewParent       = pTarget->GetGuid();

      res = pHistory->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }

    if (res.m_Result.Failed())
      pHistory->CancelTransaction();
    else
      pHistory->FinishTransaction();

    xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node move failed.");
    return true;
  }

  return false;
}

QStringList xiiQtDocumentTreeModel::mimeTypes() const
{
  QStringList types;
  if (m_bAllowDragDrop)
  {
    types << "application/xiiEditor.ObjectSelection";
  }

  return types;
}

QMimeData* xiiQtDocumentTreeModel::mimeData(const QModelIndexList& indexes) const
{
  if (!m_bAllowDragDrop)
    return nullptr;

  xiiHybridArray<void*, 1> ptrs;
  for (const QModelIndex& index : indexes)
  {
    if (index.isValid())
    {
      void* pObject = index.internalPointer();
      ptrs.PushBack(pObject);
    }
  }

  QByteArray  encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << ptrs;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/xiiEditor.ObjectSelection", encodedData);
  return mimeData;
}

bool xiiQtDocumentTreeModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  const xiiDocumentObject* pObject = (const xiiDocumentObject*)index.internalPointer();
  auto                     pType   = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->setData(pObject, index.row(), index.column(), value, iRole);
  }

  return false;
}
