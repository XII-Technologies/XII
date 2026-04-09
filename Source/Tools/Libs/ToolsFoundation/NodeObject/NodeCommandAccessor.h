#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class XII_TOOLSFOUNDATION_DLL xiiNodeCommandAccessor : public xiiObjectCommandAccessor
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNodeCommandAccessor, xiiObjectCommandAccessor);

public:
  xiiNodeCommandAccessor(xiiCommandHistory* pHistory);
  ~xiiNodeCommandAccessor();

  virtual xiiStatus SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;

  virtual xiiStatus InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index = xiiVariant()) override;
  virtual xiiStatus MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex) override;

  virtual xiiStatus AddObject(const xiiDocumentObject* pParent, const xiiAbstractProperty* pParentProp, const xiiVariant& index, const xiiRTTI* pType, xiiUuid& inout_objectGuid) override;
  virtual xiiStatus RemoveObject(const xiiDocumentObject* pObject) override;

private:
  bool IsNode(const xiiDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const;

  struct ConnectionInfo
  {
    const xiiDocumentObject* m_pSource = nullptr;
    const xiiDocumentObject* m_pTarget = nullptr;
    xiiString                m_sSourcePin;
    xiiString                m_sTargetPin;
  };

  xiiStatus DisconnectAllPins(const xiiDocumentObject* pObject, xiiDynamicArray<ConnectionInfo>& out_oldConnections);
  xiiStatus TryReconnectAllPins(const xiiDocumentObject* pObject, const xiiDynamicArray<ConnectionInfo>& oldConnections);
};
