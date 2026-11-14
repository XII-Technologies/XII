/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class xiiDocumentObject;

class XII_TOOLSFOUNDATION_DLL xiiObjectProxyAccessor : public xiiObjectAccessorBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectProxyAccessor, xiiObjectAccessorBase);

public:
  xiiObjectProxyAccessor(xiiObjectAccessorBase* pSource);
  ~xiiObjectProxyAccessor() override;

  xiiObjectAccessorBase* GetSourceAccessor() const { return m_pSource; }

  /// \name Transaction Operations
  ///@{

  void StartTransaction(xiiStringView sDisplayString) override;
  void CancelTransaction() override;
  void FinishTransaction() override;
  void BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  void CancelTemporaryCommands() override;
  void FinishTemporaryCommands() override;

  ///@}
  /// \name xiiObjectAccessorBase overrides
  ///@{

  const xiiDocumentObject* GetObject(const xiiUuid& object) override;
  xiiStatus                GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index = xiiVariant()) override;
  xiiStatus                SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  xiiStatus                InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  xiiStatus                RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  xiiStatus                MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex) override;
  xiiStatus                GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount) override;

  xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) override;
  xiiStatus RemoveObject(const xiiDocumentObject* pObject) override;
  xiiStatus MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index) override;

  xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys) override;
  xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) override;

  ///@}

protected:
  xiiObjectAccessorBase* m_pSource = nullptr;
};
