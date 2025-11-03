#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiObjectProxyAccessor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiObjectProxyAccessor::xiiObjectProxyAccessor(xiiObjectAccessorBase* pSource) :
  xiiObjectAccessorBase(pSource->GetObjectManager()), m_pSource(pSource)
{
}

xiiObjectProxyAccessor::~xiiObjectProxyAccessor() = default;

void xiiObjectProxyAccessor::StartTransaction(xiiStringView sDisplayString)
{
  m_pSource->StartTransaction(sDisplayString);
}

void xiiObjectProxyAccessor::CancelTransaction()
{
  m_pSource->CancelTransaction();
}

void xiiObjectProxyAccessor::FinishTransaction()
{
  m_pSource->FinishTransaction();
}

void xiiObjectProxyAccessor::BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pSource->BeginTemporaryCommands(sDisplayString, bFireEventsWhenUndoingTempCommands);
}

void xiiObjectProxyAccessor::CancelTemporaryCommands()
{
  m_pSource->CancelTemporaryCommands();
}

void xiiObjectProxyAccessor::FinishTemporaryCommands()
{
  m_pSource->FinishTemporaryCommands();
}

const xiiDocumentObject* xiiObjectProxyAccessor::GetObject(const xiiUuid& object)
{
  return m_pSource->GetObject(object);
}

xiiStatus xiiObjectProxyAccessor::GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index /*= xiiVariant()*/)
{
  return m_pSource->GetValue(pObject, pProp, out_value, index);
}

xiiStatus xiiObjectProxyAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  return m_pSource->SetValue(pObject, pProp, newValue, index);
}

xiiStatus xiiObjectProxyAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  return m_pSource->InsertValue(pObject, pProp, newValue, index);
}

xiiStatus xiiObjectProxyAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  return m_pSource->RemoveValue(pObject, pProp, index);
}

xiiStatus xiiObjectProxyAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  return m_pSource->MoveValue(pObject, pProp, oldIndex, newIndex);
}

xiiStatus xiiObjectProxyAccessor::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount)
{
  return m_pSource->GetCount(pObject, pProp, out_iCount);
}

xiiStatus xiiObjectProxyAccessor::AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid)
{
  return m_pSource->AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
}

xiiStatus xiiObjectProxyAccessor::RemoveObject(const xiiDocumentObject* pObject)
{
  return m_pSource->RemoveObject(pObject);
}

xiiStatus xiiObjectProxyAccessor::MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index)
{
  return m_pSource->MoveObject(pObject, pNewParent, pParentProp, index);
}

xiiStatus xiiObjectProxyAccessor::GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  return m_pSource->GetKeys(pObject, pProp, out_keys);
}

xiiStatus xiiObjectProxyAccessor::GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values)
{
  return m_pSource->GetValues(pObject, pProp, out_values);
}
