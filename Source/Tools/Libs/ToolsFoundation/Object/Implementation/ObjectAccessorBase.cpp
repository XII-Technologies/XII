#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void xiiObjectAccessorBase::StartTransaction(const char* szDisplayString) {}


void xiiObjectAccessorBase::CancelTransaction() {}


void xiiObjectAccessorBase::FinishTransaction() {}


void xiiObjectAccessorBase::BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/) {}


void xiiObjectAccessorBase::CancelTemporaryCommands() {}


void xiiObjectAccessorBase::FinishTemporaryCommands() {}


xiiStatus xiiObjectAccessorBase::GetValue(const xiiDocumentObject* pObject, const char* szProp, xiiVariant& out_value, xiiVariant index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


xiiStatus xiiObjectAccessorBase::SetValue(
  const xiiDocumentObject* pObject,
  const char*              szProp,
  const xiiVariant&        newValue,
  xiiVariant               index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


xiiStatus xiiObjectAccessorBase::InsertValue(
  const xiiDocumentObject* pObject,
  const char*              szProp,
  const xiiVariant&        newValue,
  xiiVariant               index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


xiiStatus xiiObjectAccessorBase::RemoveValue(const xiiDocumentObject* pObject, const char* szProp, xiiVariant index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


xiiStatus xiiObjectAccessorBase::MoveValue(const xiiDocumentObject* pObject, const char* szProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


xiiStatus xiiObjectAccessorBase::GetCount(const xiiDocumentObject* pObject, const char* szProp, xiiInt32& out_iCount)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


xiiStatus xiiObjectAccessorBase::AddObject(
  const xiiDocumentObject* pParent,
  const char*              szParentProp,
  const xiiVariant&        index,
  const xiiRTTI*           pType,
  xiiUuid&                 inout_objectGuid)
{
  const xiiAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(szParentProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

xiiStatus xiiObjectAccessorBase::MoveObject(
  const xiiDocumentObject* pObject,
  const xiiDocumentObject* pNewParent,
  const char*              szParentProp,
  const xiiVariant&        index)
{
  const xiiAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(szParentProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


xiiStatus xiiObjectAccessorBase::GetKeys(const xiiDocumentObject* pObject, const char* szProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


xiiStatus xiiObjectAccessorBase::GetValues(const xiiDocumentObject* pObject, const char* szProp, xiiDynamicArray<xiiVariant>& out_values)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
}

const xiiDocumentObject* xiiObjectAccessorBase::GetChildObject(const xiiDocumentObject* pObject, const char* szProp, xiiVariant index)
{
  xiiVariant value;
  if (GetValue(pObject, szProp, value, index).Succeeded() && value.IsA<xiiUuid>())
  {
    return GetObject(value.Get<xiiUuid>());
  }
  return nullptr;
}

xiiStatus xiiObjectAccessorBase::Clear(const xiiDocumentObject* pObject, const char* szProp)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));

  xiiHybridArray<xiiVariant, 8> keys;
  xiiStatus                     res = GetKeys(pObject, pProp, keys);
  if (res.Failed())
    return res;

  for (xiiInt32 i = keys.GetCount() - 1; i >= 0; --i)
  {
    res = RemoveValue(pObject, pProp, keys[i]);
    if (res.Failed())
      return res;
  }
  return xiiStatus(XII_SUCCESS);
}

xiiObjectAccessorBase::xiiObjectAccessorBase(const xiiDocumentObjectManager* pManager) :
  m_pConstManager(pManager)
{
}

xiiObjectAccessorBase::~xiiObjectAccessorBase() {}

const xiiDocumentObjectManager* xiiObjectAccessorBase::GetObjectManager() const
{
  return m_pConstManager;
}

void xiiObjectAccessorBase::FireDocumentObjectStructureEvent(const xiiDocumentObjectStructureEvent& e)
{
  m_pConstManager->m_StructureEvents.Broadcast(e);
}

void xiiObjectAccessorBase::FireDocumentObjectPropertyEvent(const xiiDocumentObjectPropertyEvent& e)
{
  m_pConstManager->m_PropertyEvents.Broadcast(e);
}
