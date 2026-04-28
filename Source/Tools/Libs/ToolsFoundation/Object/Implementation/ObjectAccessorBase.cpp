/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectAccessorBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

void xiiObjectAccessorBase::StartTransaction(xiiStringView sDisplayString) {}


void xiiObjectAccessorBase::CancelTransaction() {}


void xiiObjectAccessorBase::FinishTransaction() {}


void xiiObjectAccessorBase::BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/) {}


void xiiObjectAccessorBase::CancelTemporaryCommands() {}


void xiiObjectAccessorBase::FinishTemporaryCommands() {}


xiiStatus xiiObjectAccessorBase::GetValueByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant& out_value, xiiVariant index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


xiiStatus xiiObjectAccessorBase::SetValueByName(const xiiDocumentObject* pObject, xiiStringView sProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


xiiStatus xiiObjectAccessorBase::InsertValueByName(const xiiDocumentObject* pObject, xiiStringView sProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


xiiStatus xiiObjectAccessorBase::RemoveValueByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant index /*= xiiVariant()*/)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


xiiStatus xiiObjectAccessorBase::MoveValueByName(const xiiDocumentObject* pObject, xiiStringView sProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


xiiStatus xiiObjectAccessorBase::GetCountByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiInt32& out_iCount)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


xiiStatus xiiObjectAccessorBase::AddObjectByName(const xiiDocumentObject* pParent, xiiStringView sParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid)
{
  const xiiAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

xiiStatus xiiObjectAccessorBase::MoveObjectByName(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, xiiStringView sParentProp, const xiiVariant& index)
{
  const xiiAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(sParentProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


xiiStatus xiiObjectAccessorBase::GetKeysByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


xiiStatus xiiObjectAccessorBase::GetValuesByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiDynamicArray<xiiVariant>& out_values)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
}

const xiiDocumentObject* xiiObjectAccessorBase::GetChildObjectByName(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant index)
{
  xiiVariant value;
  if (GetValueByName(pObject, sProp, value, index).Succeeded() && value.IsA<xiiUuid>())
  {
    return GetObject(value.Get<xiiUuid>());
  }
  return nullptr;
}

xiiStatus xiiObjectAccessorBase::ClearByName(const xiiDocumentObject* pObject, xiiStringView sProp)
{
  const xiiAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sProp);
  if (!pProp)
    return xiiStatus(xiiFmt("The property '{0}' does not exist in type '{1}'.", sProp, pObject->GetType()->GetTypeName()));

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
  return XII_SUCCESS;
}

const xiiAbstractProperty* xiiObjectAccessorBase::FindPropertyByName(const xiiDocumentObject* pObject, xiiStringView sProp)
{
  return pObject->GetType()->FindPropertyByName(sProp);
}

xiiObjectAccessorBase::xiiObjectAccessorBase(const xiiDocumentObjectManager* pManager) :
  m_pConstManager(pManager)
{
}

xiiObjectAccessorBase::~xiiObjectAccessorBase() = default;

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
