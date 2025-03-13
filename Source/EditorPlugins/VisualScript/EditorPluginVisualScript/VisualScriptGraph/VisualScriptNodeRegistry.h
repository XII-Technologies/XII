#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

struct xiiNodePropertyValue;
class xiiVisualScriptPin;

class xiiVisualScriptNodeRegistry
{
  XII_DECLARE_SINGLETON(xiiVisualScriptNodeRegistry);

public:
  struct PinDesc
  {
    xiiHashedString m_sName;
    xiiHashedString m_sDynamicPinProperty;
    const xiiRTTI*  m_pDataType = nullptr;

    using DeductTypeFunc            = xiiVisualScriptDataType::Enum (*)(const xiiVisualScriptPin& pin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    xiiEnum<xiiVisualScriptDataType> m_ScriptDataType;
    bool                             m_bRequired         = false;
    bool                             m_bSplitExecution   = false;
    bool                             m_bReplaceWithArray = false;

    XII_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == xiiVisualScriptDataType::Invalid; }
    XII_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != xiiVisualScriptDataType::Invalid; }

    static xiiColor GetColorForScriptDataType(xiiVisualScriptDataType::Enum dataType);
    xiiColor        GetColor() const;
  };

  struct NodeDesc
  {
    xiiSmallArray<PinDesc, 4>                    m_InputPins;
    xiiSmallArray<PinDesc, 4>                    m_OutputPins;
    xiiHashedString                              m_sFilterByBaseClass;
    const xiiRTTI*                               m_pTargetType = nullptr;
    xiiSmallArray<const xiiAbstractProperty*, 1> m_TargetProperties;

    using DeductTypeFunc            = xiiVisualScriptDataType::Enum (*)(const xiiDocumentObject* pObject, const xiiVisualScriptPin* pDisconnectedPin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    bool                                          m_bImplicitExecution = true;
    bool                                          m_bHasDynamicPins    = false;

    void AddInputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty = xiiHashedString());
    void AddOutputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty = xiiHashedString(), bool bSplitExecution = false);

    void AddInputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, const xiiHashedString& sDynamicPinProperty = xiiHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr, bool bReplaceWithArray = false);
    void AddOutputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, const xiiHashedString& sDynamicPinProperty = xiiHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

    XII_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_DeductTypeFunc != nullptr; }
  };

  xiiVisualScriptNodeRegistry();
  ~xiiVisualScriptNodeRegistry();

  const xiiRTTI*  GetNodeBaseType() const { return m_pBaseType; }
  const xiiRTTI*  GetVariableSetterType() const { return m_pSetVariableType; }
  const xiiRTTI*  GetVariableGetterType() const { return m_pGetVariableType; }
  const NodeDesc* GetNodeDescForType(const xiiRTTI* pRtti) const { return m_TypeToNodeDescs.GetValue(pRtti); }

  struct NodeCreationTemplate
  {
    const xiiRTTI*  m_pType = nullptr;
    xiiStringView   m_sTypeName;
    xiiHashedString m_sCategory;
    xiiUInt32       m_uiPropertyValuesStart;
    xiiUInt32       m_uiPropertyValuesCount;
  };

  const xiiArrayPtr<const NodeCreationTemplate> GetNodeCreationTemplates() const { return m_NodeCreationTemplates; }
  const xiiArrayPtr<const xiiNodePropertyValue> GetPropertyValues() const { return m_PropertyValues; }

  static constexpr const char* s_szTypeNamePrefix       = "VisualScriptNode_";
  static constexpr xiiUInt32   s_uiTypeNamePrefixLength = xiiStringUtils::GetStringElementCount(s_szTypeNamePrefix);

private:
  void PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);
  void UpdateNodeTypes();
  void UpdateNodeType(const xiiRTTI* pRtti, bool bForceExpose = false);

  xiiResult                     GetScriptDataType(const xiiRTTI* pRtti, xiiVisualScriptDataType::Enum& out_scriptDataType, xiiStringView sFunctionName = xiiStringView(), xiiStringView sArgName = xiiStringView());
  xiiVisualScriptDataType::Enum GetScriptDataType(const xiiAbstractProperty* pProp);

  template <typename T>
  void AddInputDataPin(xiiReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, xiiStringView sName);
  void AddInputDataPin_Any(xiiReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, xiiStringView sName, bool bRequired, bool bAddVariantProperty = false, PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

  template <typename T>
  void AddOutputDataPin(NodeDesc& ref_nodeDesc, xiiStringView sName);

  void CreateBuiltinTypes();
  void CreateGetOwnerNodeType(const xiiRTTI* pRtti);
  void CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiHashedString& sCategory, const xiiAbstractFunctionProperty* pFunction, const xiiScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction);
  void CreateCoroutineNodeType(const xiiRTTI* pRtti);
  void CreateMessageNodeTypes(const xiiRTTI* pRtti);
  void CreateEnumNodeTypes(const xiiRTTI* pRtti);

  void FillDesc(xiiReflectedTypeDescriptor& desc, const xiiRTTI* pRtti, const xiiColorGammaUB* pColorOverride = nullptr);
  void FillDesc(xiiReflectedTypeDescriptor& desc, xiiStringView sTypeName, const xiiColorGammaUB& color);

  const xiiRTTI* RegisterNodeType(xiiReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const xiiHashedString& sCategory);

  const xiiRTTI*                   m_pBaseType            = nullptr;
  const xiiRTTI*                   m_pSetPropertyType     = nullptr;
  const xiiRTTI*                   m_pGetPropertyType     = nullptr;
  const xiiRTTI*                   m_pSetVariableType     = nullptr;
  const xiiRTTI*                   m_pGetVariableType     = nullptr;
  bool                             m_bBuiltinTypesCreated = false;
  xiiMap<const xiiRTTI*, NodeDesc> m_TypeToNodeDescs;
  xiiHashSet<const xiiRTTI*>       m_ExposedTypes;

  xiiDynamicArray<NodeCreationTemplate> m_NodeCreationTemplates;
  xiiDynamicArray<xiiNodePropertyValue> m_PropertyValues;
  xiiDeque<xiiString>                   m_PropertyNodeTypeNames;
};
