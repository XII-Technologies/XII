/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class xiiDocumentObject;

class XII_TOOLSFOUNDATION_DLL xiiObjectProxyAccessor : public xiiObjectAccessorBase
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectProxyAccessor, xiiObjectAccessorBase);

public:
  xiiObjectProxyAccessor(xiiObjectAccessorBase* pSource);
  virtual ~xiiObjectProxyAccessor();

  xiiObjectAccessorBase* GetSourceAccessor() const { return m_pSource; }

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(xiiStringView sDisplayString) override;
  virtual void CancelTransaction() override;
  virtual void FinishTransaction() override;
  virtual void BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  virtual void CancelTemporaryCommands() override;
  virtual void FinishTemporaryCommands() override;

  ///@}
  /// \name xiiObjectAccessorBase overrides
  ///@{

  virtual const xiiDocumentObject* GetObject(const xiiUuid& object) override;
  virtual xiiStatus                GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus                SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus                InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus                RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus                MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex) override;
  virtual xiiStatus                GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount) override;

  virtual xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) override;
  virtual xiiStatus RemoveObject(const xiiDocumentObject* pObject) override;
  virtual xiiStatus MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index) override;

  virtual xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys) override;
  virtual xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) override;

  ///@}

protected:
  xiiObjectAccessorBase* m_pSource = nullptr;
};
