#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class xiiVisualScriptPin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptPin, xiiPin);

public:
  xiiVisualScriptPin(Type type, xiiStringView sName, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, const xiiDocumentObject* pObject, xiiUInt32 uiDataPinIndex);
  ~xiiVisualScriptPin();

  XII_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == xiiVisualScriptDataType::Invalid; }
  XII_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != xiiVisualScriptDataType::Invalid; }

  XII_ALWAYS_INLINE const xiiRTTI* GetDataType() const { return m_pDataType; }
  XII_ALWAYS_INLINE xiiVisualScriptDataType::Enum GetScriptDataType() const { return m_ScriptDataType; }
  xiiVisualScriptDataType::Enum                   GetResolvedScriptDataType() const;
  xiiStringView                                   GetDataTypeName() const;
  XII_ALWAYS_INLINE xiiUInt32                     GetDataPinIndex() const { return m_uiDataPinIndex; }
  XII_ALWAYS_INLINE bool                          IsRequired() const { return m_bRequired; }
  XII_ALWAYS_INLINE bool                          HasDynamicPinProperty() const { return m_bHasDynamicPinProperty; }
  XII_ALWAYS_INLINE bool                          SplitExecution() const { return m_bSplitExecution; }
  XII_ALWAYS_INLINE bool                          NeedsTypeDeduction() const { return m_DeductTypeFunc != nullptr; }

  XII_ALWAYS_INLINE xiiVisualScriptNodeRegistry::PinDesc::DeductTypeFunc GetDeductTypeFunc() const { return m_DeductTypeFunc; }

  bool CanConvertTo(const xiiVisualScriptPin& targetPin, bool bUseResolvedDataTypes = true) const;

private:
  const xiiRTTI*                                       m_pDataType      = nullptr;
  xiiVisualScriptNodeRegistry::PinDesc::DeductTypeFunc m_DeductTypeFunc = nullptr;
  xiiUInt32                                            m_uiDataPinIndex = 0;
  xiiEnum<xiiVisualScriptDataType>                     m_ScriptDataType;
  bool                                                 m_bRequired              = false;
  bool                                                 m_bHasDynamicPinProperty = false;
  bool                                                 m_bSplitExecution        = false;
};

class xiiVisualScriptNodeManager : public xiiDocumentNodeManager
{
public:
  xiiVisualScriptNodeManager();
  ~xiiVisualScriptNodeManager();

  xiiHashedString GetScriptBaseClass() const;
  bool            IsFilteredByBaseClass(const xiiRTTI* pNodeType, const xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, const xiiHashedString& sBaseClass, bool bLogWarning = false) const;

  xiiVisualScriptDataType::Enum GetVariableType(xiiTempHashedString sName) const;
  xiiResult                     GetVariableDefaultValue(xiiTempHashedString sName, xiiVariant& out_value) const;

  void GetInputExecutionPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;
  void GetOutputExecutionPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;

  void GetInputDataPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;
  void GetOutputDataPins(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiVisualScriptPin*>& out_pins) const;

  void GetEntryNodes(const xiiDocumentObject* pObject, xiiDynamicArray<const xiiDocumentObject*>& out_entryNodes) const;

  static xiiStringView GetNiceTypeName(const xiiDocumentObject* pObject);
  static xiiStringView GetNiceFunctionName(const xiiDocumentObject* pObject);

  xiiVisualScriptDataType::Enum GetDeductedType(const xiiVisualScriptPin& pin) const;
  xiiVisualScriptDataType::Enum GetDeductedType(const xiiDocumentObject* pObject) const;

  bool IsCoroutine(const xiiDocumentObject* pObject) const;
  bool IsLoop(const xiiDocumentObject* pObject) const;

  xiiEvent<const xiiDocumentObject*> m_NodeChangedEvent;

private:
  virtual bool      InternalIsNode(const xiiDocumentObject* pObject) const override;
  virtual bool      InternalIsDynamicPinProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) const override;
  virtual xiiStatus InternalCanConnect(const xiiPin& source, const xiiPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const xiiDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(xiiHybridArray<const xiiRTTI*, 32>& Types) const override;

  void NodeEventsHandler(const xiiDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const xiiDocumentObjectPropertyEvent& e);

  friend class xiiVisualScriptPin;
  void RemoveDeductedPinType(const xiiVisualScriptPin& pin);
  void DeductNodeTypeAndAllPinTypes(const xiiDocumentObject* pObject, const xiiPin* pDisconnectedPin = nullptr);
  void UpdateCoroutine(const xiiDocumentObject* pTargetNode, const xiiConnection& changedConnection, bool bIsAboutToDisconnect = false);
  bool IsConnectedToCoroutine(const xiiDocumentObject* pEntryNode, const xiiConnection& changedConnection, bool bIsAboutToDisconnect = false) const;

  xiiHashTable<const xiiDocumentObject*, xiiEnum<xiiVisualScriptDataType>>  m_ObjectToDeductedType;
  xiiHashTable<const xiiVisualScriptPin*, xiiEnum<xiiVisualScriptDataType>> m_PinToDeductedType;
  xiiHashSet<const xiiDocumentObject*>                                      m_CoroutineObjects;
};
