#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>

xiiObjectDirectAccessor::xiiObjectDirectAccessor(xiiDocumentObjectManager* pManager) :
  xiiObjectAccessorBase(pManager), m_pManager(pManager)
{
}

const xiiDocumentObject* xiiObjectDirectAccessor::GetObject(const xiiUuid& object)
{
  return m_pManager->GetObject(object);
}

xiiStatus xiiObjectDirectAccessor::GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index)
{
  if (pProp == nullptr)
    return xiiStatus("Property is null.");

  xiiStatus res(XII_SUCCESS);
  out_value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index, &res);
  return res;
}

xiiStatus xiiObjectDirectAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  xiiDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  XII_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().SetValue(pProp->GetPropertyName(), newValue, index);
  return xiiStatus(bRes ? XII_SUCCESS : XII_FAILURE);
}

xiiStatus xiiObjectDirectAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  xiiDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  XII_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), index, newValue);
  return xiiStatus(bRes ? XII_SUCCESS : XII_FAILURE);
}

xiiStatus xiiObjectDirectAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  XII_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), index);
  return xiiStatus(bRes ? XII_SUCCESS : XII_FAILURE);
}

xiiStatus xiiObjectDirectAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  xiiDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  XII_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().MoveValue(pProp->GetPropertyName(), oldIndex, newIndex);
  return xiiStatus(bRes ? XII_SUCCESS : XII_FAILURE);
}

xiiStatus xiiObjectDirectAccessor::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount)
{
  out_iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
  return XII_SUCCESS;
}

xiiStatus xiiObjectDirectAccessor::AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid)
{
  XII_SUCCEED_OR_RETURN(m_pManager->CanAdd(pType, pParent, pParentProp->GetPropertyName(), index));

  xiiDocumentObject* pPar = m_pManager->GetObject(pParent->GetGuid());
  XII_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  if (!inout_objectGuid.IsValid())
    inout_objectGuid = xiiUuid::MakeUuid();

  xiiDocumentObject* pObj = m_pManager->CreateObject(pType, inout_objectGuid);
  m_pManager->AddObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return XII_SUCCESS;
}

xiiStatus xiiObjectDirectAccessor::RemoveObject(const xiiDocumentObject* pObject)
{
  XII_SUCCEED_OR_RETURN(m_pManager->CanRemove(pObject));

  xiiDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  XII_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  m_pManager->RemoveObject(pObj);
  return XII_SUCCESS;
}

xiiStatus xiiObjectDirectAccessor::MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index)
{
  XII_SUCCEED_OR_RETURN(m_pManager->CanMove(pObject, pNewParent, pParentProp->GetPropertyName(), index));

  xiiDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  XII_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  xiiDocumentObject* pPar = m_pManager->GetObject(pNewParent->GetGuid());
  XII_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  m_pManager->MoveObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return XII_SUCCESS;
}

xiiStatus xiiObjectDirectAccessor::GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  bool bRes = pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), out_keys);
  return xiiStatus(bRes ? XII_SUCCESS : XII_FAILURE);
}

xiiStatus xiiObjectDirectAccessor::GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values)
{
  bool bRes = pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), out_values);
  return xiiStatus(bRes ? XII_SUCCESS : XII_FAILURE);
}
