/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

struct xiiVisualScriptVariable;

class xiiVisualScriptPin : public xiiPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVisualScriptPin, xiiPin);

public:
  xiiVisualScriptPin(Type type, xiiStringView sName, const xiiVisualScriptNodeRegistry::PinDesc& pinDesc, const xiiDocumentObject* pObject, xiiUInt32 uiDataPinIndex, xiiUInt32 uiElementIndex);
  ~xiiVisualScriptPin();

  XII_ALWAYS_INLINE bool IsExecutionPin() const { return m_pDesc->IsExecutionPin(); }
  XII_ALWAYS_INLINE bool IsDataPin() const { return m_pDesc->IsDataPin(); }

  XII_ALWAYS_INLINE const xiiRTTI* GetDataType() const { return m_pDesc->m_pDataType; }
  XII_ALWAYS_INLINE xiiVisualScriptDataType::Enum GetScriptDataType() const { return m_pDesc->m_ScriptDataType; }
  xiiVisualScriptDataType::Enum                   GetResolvedScriptDataType() const;
  xiiStringView                                   GetDataTypeName() const;
  XII_ALWAYS_INLINE xiiUInt32                     GetDataPinIndex() const { return m_uiDataPinIndex; }
  XII_ALWAYS_INLINE xiiUInt32                     GetElementIndex() const { return m_uiElementIndex; }
  XII_ALWAYS_INLINE bool                          IsRequired() const { return m_pDesc->m_bRequired; }
  XII_ALWAYS_INLINE bool                          HasDynamicPinProperty() const { return m_pDesc->m_sDynamicPinProperty.IsEmpty() == false; }
  XII_ALWAYS_INLINE bool                          SplitExecution() const { return m_pDesc->m_bSplitExecution; }
  XII_ALWAYS_INLINE bool                          ReplaceWithArray() const { return m_pDesc->m_bReplaceWithArray; }
  XII_ALWAYS_INLINE bool                          NeedsTypeDeduction() const { return m_pDesc->m_DeductTypeFunc != nullptr; }

  XII_ALWAYS_INLINE const xiiHashedString& GetDynamicPinProperty() const { return m_pDesc->m_sDynamicPinProperty; }
  XII_ALWAYS_INLINE xiiVisualScriptNodeRegistry::PinDesc::DeductTypeFunc GetDeductTypeFunc() const { return m_pDesc->m_DeductTypeFunc; }

  bool CanConvertTo(const xiiVisualScriptPin& targetPin, bool bUseResolvedDataTypes = true) const;

private:
  const xiiVisualScriptNodeRegistry::PinDesc* m_pDesc          = nullptr;
  xiiUInt32                                   m_uiDataPinIndex = 0;
  xiiUInt32                                   m_uiElementIndex = 0;
};

class xiiVisualScriptNodeManager : public xiiDocumentNodeManager
{
public:
  xiiVisualScriptNodeManager();
  ~xiiVisualScriptNodeManager();

  xiiHashedString GetScriptBaseClass() const;
  bool            IsFilteredByBaseClass(const xiiRTTI* pNodeType, const xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, const xiiHashedString& sBaseClass, bool bLogWarning = false) const;

  xiiVisualScriptDataType::Enum GetVariableType(xiiTempHashedString sName) const;
  xiiResult                     GetVariable(xiiTempHashedString sName, xiiVisualScriptVariable& out_variable) const;
  void                          GetAllVariables(xiiDynamicArray<xiiVisualScriptVariable>& out_variables) const;

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

  virtual void GetNodeCreationTemplates(xiiDynamicArray<xiiNodeCreationTemplate>& out_templates) const override;

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

  mutable xiiDynamicArray<xiiNodePropertyValue> m_PropertyValues;
  mutable xiiDeque<xiiString>                   m_VariableNodeTypeNames;
};
