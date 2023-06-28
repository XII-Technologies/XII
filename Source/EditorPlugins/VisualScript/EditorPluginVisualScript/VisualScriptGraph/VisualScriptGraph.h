#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiVisualScriptPin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptPin, xiiPin);

public:
  xiiVisualScriptPin(Type type, xiiStringView sName, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, const xiiDocumentObject* pObject, xiiUInt32 uiPinIndex);

  XII_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == xiiVisualScriptDataType::Invalid; }
  XII_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != xiiVisualScriptDataType::Invalid; }

  XII_ALWAYS_INLINE const xiiRTTI* GetDataType() const { return m_pDataType; }
  XII_ALWAYS_INLINE xiiVisualScriptDataType::Enum GetScriptDataType() const { return m_ScriptDataType; }
  xiiStringView GetDataTypeName(xiiVisualScriptDataType::Enum deductedType = xiiVisualScriptDataType::Invalid) const;
  XII_ALWAYS_INLINE xiiUInt32 GetPinIndex() const { return m_uiPinIndex; }
  XII_ALWAYS_INLINE bool IsRequired() const { return m_bRequired; }
  XII_ALWAYS_INLINE bool HasDynamicPinProperty() const { return m_bHasDynamicPinProperty; }

  bool CanConvertTo(const xiiVisualScriptPin& targetPin, xiiVisualScriptDataType::Enum deductedSourceDataType = xiiVisualScriptDataType::Invalid, xiiVisualScriptDataType::Enum deductedTargetDataType = xiiVisualScriptDataType::Invalid) const;

private:
  const xiiRTTI* m_pDataType = nullptr;
  xiiUInt32 m_uiPinIndex = 0;
  xiiEnum<xiiVisualScriptDataType> m_ScriptDataType;
  bool m_bRequired = false;
  bool m_bHasDynamicPinProperty = false;
};

class xiiVisualScriptNodeManager : public xiiDocumentNodeManager
{
public:
  xiiVisualScriptNodeManager();
  ~xiiVisualScriptNodeManager();

  xiiHashedString GetScriptBaseClass() const;
  bool IsFilteredByBaseClass(const xiiRTTI* pNodeType, const xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, const xiiHashedString& sBaseClass, bool bLogWarning = false) const;

  void GetInputExecutionPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;
  void GetOutputExecutionPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;

  void GetInputDataPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;
  void GetOutputDataPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;

  static xiiStringView GetNiceTypeName(const xiiDocumentObject* pObject);

  void DeductType(const xiiDocumentObject* pObject, const xiiPin* pChangedPin = nullptr, bool bConnected = true);
  xiiVisualScriptDataType::Enum GetDeductedType(const xiiVisualScriptPin& pin) const;
  xiiVisualScriptDataType::Enum GetDeductedType(const xiiDocumentObject* pObject) const;

  xiiEvent<const xiiDocumentObject*> m_DeductedTypeChangedEvent;

private:
  virtual bool InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual bool InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const override;
  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const override;

  void NodeEventsHandler(const xiiDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e);

  xiiHashTable<const xiiDocumentObject*, xiiEnum<xiiVisualScriptDataType>> m_ObjectToDeductedType;
};
