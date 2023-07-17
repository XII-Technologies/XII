#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

xiiObjectCommandAccessor::xiiObjectCommandAccessor(xiiCommandHistory* pHistory) :
  xiiObjectDirectAccessor(const_cast<xiiDocumentObjectManager*>(pHistory->GetDocument()->GetObjectManager())), m_pHistory(pHistory)
{
}

void xiiObjectCommandAccessor::StartTransaction(xiiStringView sDisplayString)
{
  m_pHistory->StartTransaction(sDisplayString);
}

void xiiObjectCommandAccessor::CancelTransaction()
{
  m_pHistory->CancelTransaction();
}

void xiiObjectCommandAccessor::FinishTransaction()
{
  m_pHistory->FinishTransaction();
}

void xiiObjectCommandAccessor::BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pHistory->BeginTemporaryCommands(sDisplayString, bFireEventsWhenUndoingTempCommands);
}

void xiiObjectCommandAccessor::CancelTemporaryCommands()
{
  m_pHistory->CancelTemporaryCommands();
}

void xiiObjectCommandAccessor::FinishTemporaryCommands()
{
  m_pHistory->FinishTemporaryCommands();
}

xiiStatus xiiObjectCommandAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  xiiSetObjectPropertyCommand cmd;
  cmd.m_Object    = pObject->GetGuid();
  cmd.m_NewValue  = newValue;
  cmd.m_Index     = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

xiiStatus xiiObjectCommandAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  xiiInsertObjectPropertyCommand cmd;
  cmd.m_Object    = pObject->GetGuid();
  cmd.m_NewValue  = newValue;
  cmd.m_Index     = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

xiiStatus xiiObjectCommandAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiRemoveObjectPropertyCommand cmd;
  cmd.m_Object    = pObject->GetGuid();
  cmd.m_Index     = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

xiiStatus xiiObjectCommandAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  xiiMoveObjectPropertyCommand cmd;
  cmd.m_Object    = pObject->GetGuid();
  cmd.m_OldIndex  = oldIndex;
  cmd.m_NewIndex  = newIndex;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

xiiStatus xiiObjectCommandAccessor::AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid)
{
  xiiAddObjectCommand cmd;
  cmd.m_Parent          = pParent ? pParent->GetGuid() : xiiUuid();
  cmd.m_Index           = index;
  cmd.m_pType           = pType;
  cmd.m_NewObjectGuid   = inout_objectGuid;
  cmd.m_sParentProperty = pParentProp ? pParentProp->GetPropertyName() : "Children";
  xiiStatus res         = m_pHistory->AddCommand(cmd);
  if (res.m_Result.Succeeded())
    inout_objectGuid = cmd.m_NewObjectGuid;
  return res;
}

xiiStatus xiiObjectCommandAccessor::RemoveObject(const xiiDocumentObject* pObject)
{
  xiiRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  return m_pHistory->AddCommand(cmd);
}

xiiStatus xiiObjectCommandAccessor::MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index)
{
  xiiMoveObjectCommand cmd;
  cmd.m_NewParent       = pNewParent ? pNewParent->GetGuid() : xiiUuid();
  cmd.m_Object          = pObject->GetGuid();
  cmd.m_Index           = index;
  cmd.m_sParentProperty = pParentProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}
