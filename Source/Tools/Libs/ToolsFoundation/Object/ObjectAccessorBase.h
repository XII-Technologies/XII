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

  virtual void StartTransaction(const char* szDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  virtual void CancelTemporaryCommands();
  virtual void FinishTemporaryCommands();

  ///@}
  /// \name Object Access Interface
  ///@{

  virtual const xiiDocumentObject* GetObject(const xiiUuid& object) = 0;
  virtual xiiStatus                GetValue(
                   const xiiDocumentObject*   pObject,
                   const xiiAbstractProperty* pProp,
                   xiiVariant&                out_value,
                   xiiVariant                 index = xiiVariant()) = 0;
  virtual xiiStatus SetValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    const xiiVariant&          newValue,
    xiiVariant                 index = xiiVariant()) = 0;
  virtual xiiStatus InsertValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    const xiiVariant&          newValue,
    xiiVariant                 index = xiiVariant())                                                                                                 = 0;
  virtual xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) = 0;
  virtual xiiStatus MoveValue(
    const xiiDocumentObject*   pObject,
    const xiiAbstractProperty* pProp,
    const xiiVariant&          oldIndex,
    const xiiVariant&          newIndex)                                                                                        = 0;
  virtual xiiStatus GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount) = 0;

  virtual xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) = 0;
  virtual xiiStatus RemoveObject(const xiiDocumentObject* pObject)                                                                                                                = 0;
  virtual xiiStatus MoveObject(
    const xiiDocumentObject*   pObject,
    const xiiDocumentObject*   pNewParent,
    const xiiAbstractProperty* pParentProp,
    const xiiVariant&          index) = 0;

  virtual xiiStatus GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)     = 0;
  virtual xiiStatus GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values) = 0;

  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  xiiStatus GetValue(const xiiDocumentObject* pObject, const char* szProp, xiiVariant& out_value, xiiVariant index = xiiVariant());
  xiiStatus SetValue(const xiiDocumentObject* pObject, const char* szProp, const xiiVariant& newValue, xiiVariant index = xiiVariant());
  xiiStatus InsertValue(const xiiDocumentObject* pObject, const char* szProp, const xiiVariant& newValue, xiiVariant index = xiiVariant());
  xiiStatus RemoveValue(const xiiDocumentObject* pObject, const char* szProp, xiiVariant index = xiiVariant());
  xiiStatus MoveValue(const xiiDocumentObject* pObject, const char* szProp, const xiiVariant& oldIndex, const xiiVariant& newIndex);
  xiiStatus GetCount(const xiiDocumentObject* pObject, const char* szProp, xiiInt32& out_iCount);

  xiiStatus AddObject(
    const xiiDocumentObject* pParent,
    const char*              szParentProp,
    const xiiVariant&        index,
    const xiiRTTI*           pType,
    xiiUuid&                 inout_objectGuid);
  xiiStatus MoveObject(const xiiDocumentObject* pObject, const xiiDocumentObject* pNewParent, const char* szParentProp, const xiiVariant& index);

  xiiStatus                GetKeys(const xiiDocumentObject* pObject, const char* szProp, xiiDynamicArray<xiiVariant>& out_keys);
  xiiStatus                GetValues(const xiiDocumentObject* pObject, const char* szProp, xiiDynamicArray<xiiVariant>& out_values);
  const xiiDocumentObject* GetChildObject(const xiiDocumentObject* pObject, const char* szProp, xiiVariant index);

  xiiStatus Clear(const xiiDocumentObject* pObject, const char* szProp);

  template <typename T>
  T Get(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant());
  template <typename T>
  T        Get(const xiiDocumentObject* pObject, const char* szProp, xiiVariant index = xiiVariant());
  xiiInt32 GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp);

  ///@}

protected:
  xiiObjectAccessorBase(const xiiDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const xiiDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const xiiDocumentObjectPropertyEvent& e);

protected:
  const xiiDocumentObjectManager* m_pConstManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>
