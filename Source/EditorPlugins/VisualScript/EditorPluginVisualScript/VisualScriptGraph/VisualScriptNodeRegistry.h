#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

struct xiiScriptBaseClassAttribute_Function;

class xiiVisualScriptNodeRegistry
{
  XII_DECLARE_SINGLETON(xiiVisualScriptNodeRegistry);

public:
  struct PinDesc
  {
    xiiHashedString                  m_sName;
    xiiHashedString                  m_sDynamicPinProperty;
    const xiiRTTI*                   m_pDataType = nullptr;
    xiiEnum<xiiVisualScriptDataType> m_ScriptDataType;
    bool                             m_bRequired = false;

    XII_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == xiiVisualScriptDataType::Invalid; }
    XII_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != xiiVisualScriptDataType::Invalid; }

    static xiiColor GetColorForScriptDataType(xiiVisualScriptDataType::Enum dataType);
    xiiColor        GetColor() const;
  };

  struct NodeDesc
  {
    xiiSmallArray<PinDesc, 4>                     m_InputPins;
    xiiSmallArray<PinDesc, 4>                     m_OutputPins;
    xiiHashedString                               m_sFilterByBaseClass;
    const xiiRTTI*                                m_pTargetType     = nullptr;
    const xiiAbstractProperty*                    m_pTargetProperty = nullptr;
    xiiEnum<xiiVisualScriptNodeDescription::Type> m_Type;
    bool                                          m_bImplicitExecution      = false;
    bool                                          m_bNeedsDataTypeDeduction = false;
    bool                                          m_bHasDynamicPins         = false;

    void AddInputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty = xiiHashedString());
    void AddOutputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty = xiiHashedString());

    void AddInputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, const xiiHashedString& sDynamicPinProperty = xiiHashedString());
    void AddOutputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, const xiiHashedString& sDynamicPinProperty = xiiHashedString());
  };

  xiiVisualScriptNodeRegistry();
  ~xiiVisualScriptNodeRegistry();

  const xiiRTTI*  GetNodeBaseType() const { return m_pBaseType; }
  const NodeDesc* GetNodeDescForType(const xiiRTTI* pRtti) const { return m_TypeToNodeDescs.GetValue(pRtti); }

  const xiiMap<const xiiRTTI*, NodeDesc>& GetAllNodeTypes() const { return m_TypeToNodeDescs; }

  static constexpr const char* s_szTypeNamePrefix       = "VisualScriptNode_";
  static constexpr xiiUInt32   s_uiTypeNamePrefixLength = xiiStringUtils::GetStringElementCount(s_szTypeNamePrefix);

private:
  void PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e);
  void UpdateNodeTypes();
  void UpdateNodeType(const xiiRTTI* pRtti);

  void CreateBuiltinTypes();
  void CreateGetOwnerNodeType(const xiiRTTI* pRtti);
  void CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiAbstractFunctionProperty* pFunction, bool bIsEntryFunction);

  void FillDesc(xiiReflectedTypeDescriptor& desc, const xiiRTTI* pRtti, xiiStringView sCategoryOverride = xiiStringView(), xiiColorGammaUB* pColorOverride = nullptr);
  void FillDesc(xiiReflectedTypeDescriptor& desc, xiiStringView sTypeName, xiiStringView sCategory, const xiiColorGammaUB& color);

  const xiiRTTI*                   m_pBaseType            = nullptr;
  bool                             m_bBuiltinTypesCreated = false;
  xiiMap<const xiiRTTI*, NodeDesc> m_TypeToNodeDescs;
};
