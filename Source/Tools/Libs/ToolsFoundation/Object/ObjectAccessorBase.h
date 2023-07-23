#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class xiiDocumentObject;

class XII_TOOLSFOUNDATION_DLL xiiObjectAccessorBase
{
public:
  virtual ~xiiObjectAccessorBase();
  const xiiDocumentObjectManager* GetObjectManager() const;

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(xiiStringView sDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(xiiStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  virtual void CancelTemporaryCommands();
  virtual void FinishTemporaryCommands();

  ///@}
  /// \name Object Access Interface
  ///@{

  virtual const xiiDocumentObject* GetObject(const xiiUuid& object)                                                                                                             = 0;
  virtual xiiStatus                GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index = xiiVariant())         = 0;
  virtual xiiStatus                SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant())    = 0;
  virtual xiiStatus                InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) = 0;
  virtual xiiStatus                RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant())                             = 0;
  virtual xiiStatus                MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)        = 0;
  virtual xiiStatus                GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount)                                           = 0;

  virtual xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) = 0;
  virtual xiiStatus RemoveObject(const xiiDocumentObject* pObject)                                                                                                                = 0;
  virtual xiiStatus MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index)            = 0;

  virtual xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)     = 0;
  virtual xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) = 0;

  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  xiiStatus GetValue(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant& out_value, xiiVariant index = xiiVariant());
  xiiStatus SetValue(const xiiDocumentObject* pObject, xiiStringView sProp, const xiiVariant& newValue, xiiVariant index = xiiVariant());
  xiiStatus InsertValue(const xiiDocumentObject* pObject, xiiStringView sProp, const xiiVariant& newValue, xiiVariant index = xiiVariant());
  xiiStatus RemoveValue(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant index = xiiVariant());
  xiiStatus MoveValue(const xiiDocumentObject* pObject, xiiStringView sProp, const xiiVariant& oldIndex, const xiiVariant& newIndex);
  xiiStatus GetCount(const xiiDocumentObject* pObject, xiiStringView sProp, xiiInt32& out_iCount);

  xiiStatus AddObject(const xiiDocumentObject* pParent, xiiStringView sParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid);
  xiiStatus MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, xiiStringView sParentProp, const xiiVariant& index);

  xiiStatus                GetKeys(const xiiDocumentObject* pObject, xiiStringView sProp, xiiDynamicArray<xiiVariant>& out_keys);
  xiiStatus                GetValues(const xiiDocumentObject* pObject, xiiStringView sProp, xiiDynamicArray<xiiVariant>& out_values);
  const xiiDocumentObject* GetChildObject(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant index);

  xiiStatus Clear(const xiiDocumentObject* pObject, xiiStringView sProp);

  template <typename T>
  T Get(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant());
  template <typename T>
  T        Get(const xiiDocumentObject* pObject, xiiStringView sProp, xiiVariant index = xiiVariant());
  xiiInt32 GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);
  xiiInt32 GetCount(const xiiDocumentObject* pObject, xiiStringView sProp);

  ///@}

protected:
  xiiObjectAccessorBase(const xiiDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const xiiDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const xiiDocumentObjectPropertyEvent& e);

protected:
  const xiiDocumentObjectManager* m_pConstManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>
