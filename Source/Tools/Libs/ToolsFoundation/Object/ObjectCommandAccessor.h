/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Object/ObjectDirectAccessor.h>

class xiiDocumentObject;
class xiiCommandHistory;

class XII_TOOLSFOUNDATION_DLL xiiObjectCommandAccessor : public xiiObjectDirectAccessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectCommandAccessor, xiiObjectDirectAccessor);

public:
  xiiObjectCommandAccessor(xiiCommandHistory* pHistory);

  void StartTransaction(xiiStringView sDisplayString) override;
  void CancelTransaction() override;
  void FinishTransaction() override;
  void BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  void CancelTemporaryCommands() override;
  void FinishTemporaryCommands() override;

  xiiStatus SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  xiiStatus InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  xiiStatus MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex) override;

  xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) override;
  xiiStatus RemoveObject(const xiiDocumentObject* pObject) override;
  xiiStatus MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index) override;

protected:
  xiiCommandHistory* m_pHistory;
};
