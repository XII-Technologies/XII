#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

namespace
{
  constexpr const char* szPluginName           = "EditorPluginVisualScript";
  constexpr const char* szEventHandlerCategory = "Add Event Handler/";
  constexpr const char* szCoroutinesCategory   = "Coroutines";
  constexpr const char* szEnumsCategory        = "Enums";

  const xiiRTTI* FindTopMostBaseClass(const xiiRTTI* pRtti)
  {
    const xiiRTTI* pReflectedClass = xiiGetStaticRTTI<xiiReflectedClass>();
    while (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != pReflectedClass)
    {
      pRtti = pRtti->GetParentType();
    }
    return pRtti;
  }

  void CollectFunctionArgumentAttributes(const xiiAbstractFunctionProperty* pFuncProp, xiiDynamicArray<const xiiFunctionArgumentAttributes*>& out_attributes)
  {
    for (auto pAttr : pFuncProp->GetAttributes())
    {
      if (auto pFuncArgAttr = xiiDynamicCast<const xiiFunctionArgumentAttributes*>(pAttr))
      {
        xiiUInt32 uiArgIndex = pFuncArgAttr->GetArgumentIndex();
        out_attributes.EnsureCount(uiArgIndex + 1);
        out_attributes[uiArgIndex] = pFuncArgAttr;
      }
    }
  }

  void AddInputProperty(xiiReflectedTypeDescriptor& ref_typeDesc, xiiStringView sName, const xiiRTTI* pRtti, xiiVisualScriptDataType::Enum scriptDataType, xiiArrayPtr<const xiiPropertyAttribute* const> attributes = {})
  {
    auto& propDesc   = ref_typeDesc.m_Properties.ExpandAndGetRef();
    propDesc.m_sName = sName;
    propDesc.m_Flags = xiiPropertyFlags::StandardType;

    for (auto pAttr : attributes)
    {
      propDesc.m_Attributes.PushBack(pAttr->GetDynamicRTTI()->GetAllocator()->Clone<xiiPropertyAttribute>(pAttr));
    }

    if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum))
    {
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sType    = pRtti->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::IsEnum;
    }
    else
    {
      if (scriptDataType == xiiVisualScriptDataType::Variant)
      {
        propDesc.m_Category = xiiPropertyCategory::Member;
        propDesc.m_sType    = xiiGetStaticRTTI<xiiVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiVisualScriptVariableAttribute));
      }
      else if (scriptDataType == xiiVisualScriptDataType::Array)
      {
        propDesc.m_Category = xiiPropertyCategory::Array;
        propDesc.m_sType    = xiiGetStaticRTTI<xiiVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiVisualScriptVariableAttribute));
      }
      else if (scriptDataType == xiiVisualScriptDataType::Map)
      {
        propDesc.m_Category = xiiPropertyCategory::Map;
        propDesc.m_sType    = xiiGetStaticRTTI<xiiVariant>()->GetTypeName();
        propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiVisualScriptVariableAttribute));
      }
      else
      {
        propDesc.m_Category = xiiPropertyCategory::Member;
        propDesc.m_sType    = xiiVisualScriptDataType::GetRtti(scriptDataType)->GetTypeName();
      }
    }
  }

  xiiStringView StripTypeName(xiiStringView sTypeName)
  {
    sTypeName.TrimWordStart("xii");
    return sTypeName;
  }

  xiiStringView GetTypeName(const xiiRTTI* pRtti)
  {
    xiiStringView sTypeName = pRtti->GetTypeName();
    if (auto pScriptExtension = pRtti->GetAttributeByType<xiiScriptExtensionAttribute>())
    {
      sTypeName = pScriptExtension->GetTypeName();
    }
    return StripTypeName(sTypeName);
  }

  xiiColorGammaUB NiceColorFromName(xiiStringView sTypeName, xiiStringView sCategory = xiiStringView())
  {
    float typeX = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(sTypeName))).x();

    float x = typeX;
    if (sCategory.IsEmpty() == false)
    {
      x = xiiSimdRandom::FloatZeroToOne(xiiSimdVec4i(xiiHashingUtils::StringHash(sCategory))).x();
      x += typeX * xiiColorScheme::s_fIndexNormalizer;
    }

    return xiiColorScheme::DarkUI(x);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

static xiiColorScheme::Enum s_scriptDataTypeToPinColor[] = {
  xiiColorScheme::Gray,   // Invalid
  xiiColorScheme::Red,    // Bool,
  xiiColorScheme::Cyan,   // Byte,
  xiiColorScheme::Teal,   // Int,
  xiiColorScheme::Teal,   // Int64,
  xiiColorScheme::Green,  // Float,
  xiiColorScheme::Green,  // Double,
  xiiColorScheme::Lime,   // Color,
  xiiColorScheme::Orange, // Vector3,
  xiiColorScheme::Orange, // Quaternion,
  xiiColorScheme::Orange, // Transform,
  xiiColorScheme::Violet, // Time,
  xiiColorScheme::Green,  // Angle,
  xiiColorScheme::Grape,  // String,
  xiiColorScheme::Grape,  // HashedString,
  xiiColorScheme::Blue,   // GameObject,
  xiiColorScheme::Blue,   // Component,
  xiiColorScheme::Blue,   // TypedPointer,
  xiiColorScheme::Pink,   // Variant,
  xiiColorScheme::Pink,   // VariantArray,
  xiiColorScheme::Pink,   // VariantDictionary,
  xiiColorScheme::Cyan,   // Coroutine,
};

static_assert(XII_ARRAY_SIZE(s_scriptDataTypeToPinColor) == xiiVisualScriptDataType::Count);

// static
xiiColor xiiVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Enum dataType)
{
  if (dataType == xiiVisualScriptDataType::EnumValue)
  {
    return xiiColorScheme::DarkUI(xiiColorScheme::Teal);
  }

  XII_ASSERT_DEBUG(dataType >= 0 && dataType < XII_ARRAY_SIZE(s_scriptDataTypeToPinColor), "Out of bounds access");
  return xiiColorScheme::DarkUI(s_scriptDataTypeToPinColor[dataType]);
}

xiiColor xiiVisualScriptNodeRegistry::PinDesc::GetColor() const
{
  if (IsExecutionPin())
  {
    return xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  }

  if (m_ScriptDataType > xiiVisualScriptDataType::Invalid && m_ScriptDataType < xiiVisualScriptDataType::Count)
  {
    return GetColorForScriptDataType(m_ScriptDataType);
  }

  if (m_ScriptDataType == xiiVisualScriptDataType::EnumValue)
  {
    return xiiColorScheme::DarkUI(xiiColorScheme::Teal);
  }

  if (m_ScriptDataType == xiiVisualScriptDataType::Any)
  {
    return xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  }

  return xiiColorScheme::DarkUI(xiiColorScheme::Blue);
}

//////////////////////////////////////////////////////////////////////////

void AddExecutionPin(xiiVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, xiiStringView sName, xiiHashedString sDynamicPinProperty, bool bSplitExecution, xiiSmallArray<xiiVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType           = nullptr;
  pin.m_ScriptDataType      = xiiVisualScriptDataType::Invalid;
  pin.m_bSplitExecution     = bSplitExecution;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddInputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, false, m_InputPins);

  m_bImplicitExecution = false;
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddOutputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/, bool bSplitExecution /*= false*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, bSplitExecution, m_OutputPins);

  m_bImplicitExecution = false;
}

void AddDataPin(xiiVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, xiiHashedString sDynamicPinProperty, xiiVisualScriptNodeRegistry::PinDesc::DeductTypeFunc deductTypeFunc, xiiSmallArray<xiiVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
{
  if ((scriptDataType == xiiVisualScriptDataType::AnyPointer || scriptDataType == xiiVisualScriptDataType::Any) && deductTypeFunc == nullptr)
  {
    deductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromNodeDataType;
  }

  auto& pin = inout_pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_DeductTypeFunc      = deductTypeFunc;
  pin.m_pDataType           = pDataType;
  pin.m_ScriptDataType      = scriptDataType;
  pin.m_bRequired           = bRequired;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, deductTypeFunc, m_InputPins);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, deductTypeFunc, m_OutputPins);
}

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SINGLETON(xiiVisualScriptNodeRegistry);

xiiVisualScriptNodeRegistry::xiiVisualScriptNodeRegistry() :
  m_SingletonRegistrar(this)
{
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));

  UpdateNodeTypes();
}

xiiVisualScriptNodeRegistry::~xiiVisualScriptNodeRegistry()
{
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler, this));
}

void xiiVisualScriptNodeRegistry::PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  if (e.m_pChangedType->GetPluginName() == "EditorPluginVisualScript")
    return;

  if ((e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeAdded && m_TypeToNodeDescs.Contains(e.m_pChangedType) == false) ||
      e.m_Type == xiiPhantomRttiManagerEvent::Type::TypeChanged)
  {
    UpdateNodeType(e.m_pChangedType);
  }
}

void xiiVisualScriptNodeRegistry::UpdateNodeTypes()
{
  XII_PROFILE_SCOPE("Update VS Node Types");

  // Base Node Type
  if (m_pBaseType == nullptr)
  {
    xiiReflectedTypeDescriptor desc;
    desc.m_sTypeName       = "xiiVisualScriptNodeBase";
    desc.m_sPluginName     = szPluginName;
    desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
    desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;

    m_pBaseType = xiiPhantomRttiManager::RegisterType(desc);
  }

  if (m_bBuiltinTypesCreated == false)
  {
    CreateBuiltinTypes();
    m_bBuiltinTypesCreated = true;
  }

  auto& componentTypesDynEnum    = xiiDynamicStringEnum::CreateDynamicEnum("ComponentTypes");
  auto& scriptBaseClassesDynEnum = xiiDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  xiiRTTI::ForEachType([this](const xiiRTTI* pRtti) { UpdateNodeType(pRtti); });
}

void xiiVisualScriptNodeRegistry::UpdateNodeType(const xiiRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<xiiHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<xiiExcludeFromScript>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    auto& componentTypesDynEnum = xiiDynamicStringEnum::GetDynamicEnum("ComponentTypes");
    componentTypesDynEnum.AddValidValue(pRtti->GetTypeName(), true);
  }

  if (pRtti->IsDerivedFrom<xiiScriptCoroutine>())
  {
    CreateCoroutineNodeType(pRtti);
  }
  else if (pRtti->IsDerivedFrom<xiiMessage>())
  {
    CreateMessageNodeTypes(pRtti);
  }
  else
  {
    // expose reflected functions and properties to visual scripts
    {
      bool bExposeToVisualScript  = false;
      bool bHasBaseClassFunctions = false;

      for (const xiiAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
      {
        auto pScriptableFunctionAttribute = pFuncProp->GetAttributeByType<xiiScriptableFunctionAttribute>();
        if (pScriptableFunctionAttribute == nullptr)
          continue;

        bExposeToVisualScript = true;

        bool bIsBaseClassFunction = pFuncProp->GetAttributeByType<xiiScriptBaseClassFunctionAttribute>() != nullptr;
        if (bIsBaseClassFunction)
        {
          bHasBaseClassFunctions = true;
        }

        CreateFunctionCallNodeType(pRtti, pFuncProp, pScriptableFunctionAttribute, bIsBaseClassFunction);
      }

      if (bExposeToVisualScript)
      {
        for (const xiiAbstractProperty* pProp : pRtti->GetProperties())
        {
        }
      }

      if (bHasBaseClassFunctions)
      {
        auto& scriptBaseClassesDynEnum = xiiDynamicStringEnum::GetDynamicEnum("ScriptBaseClasses");
        scriptBaseClassesDynEnum.AddValidValue(StripTypeName(pRtti->GetTypeName()));

        CreateGetOwnerNodeType(pRtti);
      }
    }
  }
}

xiiResult xiiVisualScriptNodeRegistry::GetScriptDataType(const xiiRTTI* pRtti, xiiVisualScriptDataType::Enum& out_scriptDataType, xiiStringView sFunctionName /*= xiiStringView()*/, xiiStringView sArgName /*= xiiStringView()*/)
{
  if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum))
  {
    CreateEnumNodeTypes(pRtti);
  }

  xiiVisualScriptDataType::Enum scriptDataType = xiiVisualScriptDataType::FromRtti(pRtti);
  if (scriptDataType == xiiVisualScriptDataType::Invalid)
  {
    xiiLog::Warning("The script function '{}' uses an argument '{}' of type '{}' which is not a valid script data type, therefore this function will not be available in visual scripts", sFunctionName, sArgName, pRtti->GetTypeName());
    return XII_FAILURE;
  }

  out_scriptDataType = scriptDataType;
  return XII_SUCCESS;
}

xiiVisualScriptDataType::Enum xiiVisualScriptNodeRegistry::GetScriptDataType(const xiiAbstractProperty* pProp)
{
  if (pProp->GetCategory() == xiiPropertyCategory::Member)
  {
    xiiVisualScriptDataType::Enum result = xiiVisualScriptDataType::Invalid;
    GetScriptDataType(pProp->GetSpecificType(), result).IgnoreResult();
    return result;
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Array)
  {
    return xiiVisualScriptDataType::Array;
  }
  else if (pProp->GetCategory() == xiiPropertyCategory::Map)
  {
    return xiiVisualScriptDataType::Map;
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiVisualScriptDataType::Invalid;
}

template <typename T>
void xiiVisualScriptNodeRegistry::AddInputDataPin(xiiReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, xiiStringView sName)
{
  const xiiRTTI* pDataType = xiiGetStaticRTTI<T>();

  xiiVisualScriptDataType::Enum scriptDataType;
  XII_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  AddInputProperty(ref_typeDesc, sName, pDataType, scriptDataType);

  ref_nodeDesc.AddInputDataPin(sName, pDataType, scriptDataType, false);
};

void xiiVisualScriptNodeRegistry::AddInputDataPin_Any(xiiReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, xiiStringView sName, bool bRequired, bool bAddVariantProperty /*= false*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  if (bAddVariantProperty)
  {
    AddInputProperty(ref_typeDesc, sName, xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant);
  }

  ref_nodeDesc.AddInputDataPin(sName, nullptr, xiiVisualScriptDataType::Any, bRequired, xiiHashedString(), deductTypeFunc);
}

template <typename T>
void xiiVisualScriptNodeRegistry::AddOutputDataPin(NodeDesc& ref_nodeDesc, xiiStringView sName)
{
  const xiiRTTI* pDataType = xiiGetStaticRTTI<T>();

  xiiVisualScriptDataType::Enum scriptDataType;
  XII_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", sName).Succeeded(), "Invalid script data type");

  ref_nodeDesc.AddOutputDataPin(sName, pDataType, scriptDataType);
};

void xiiVisualScriptNodeRegistry::CreateBuiltinTypes()
{
  const xiiColorGammaUB logicColor      = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Invalid);
  const xiiColorGammaUB mathColor       = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Int);
  const xiiColorGammaUB stringColor     = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::String);
  const xiiColorGammaUB gameObjectColor = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::GameObject);
  const xiiColorGammaUB variantColor    = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Variant);
  const xiiColorGammaUB coroutineColor  = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Coroutine);

  // GetReflectedProperty
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "GetProperty", "Properties", logicColor);

    AddInputProperty(typeDesc, "Type", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{Type}::Get {Property}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::GetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputDataPin("Object", nullptr, xiiVisualScriptDataType::AnyPointer, true, xiiHashedString(), &xiiVisualScriptTypeDeduction::DeductFromTypeProperty);
    nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // SetReflectedProperty
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "SetProperty", "Properties", logicColor);

    AddInputProperty(typeDesc, "Type", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{Type}::Set {Property} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::SetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Object", nullptr, xiiVisualScriptDataType::AnyPointer, true, xiiHashedString(), &xiiVisualScriptTypeDeduction::DeductFromTypeProperty);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_GetVariable
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_GetVariable", "Variables", logicColor);

    AddInputProperty(typeDesc, "Name", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Get {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_GetVariable;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_SetVariable
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_SetVariable", "Variables", logicColor);

    AddInputProperty(typeDesc, "Name", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Set {Name} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_SetVariable;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);
    nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IncVariable, Builtin_DecVariable
  {
    xiiVisualScriptNodeDescription::Type::Enum nodeTypes[] = {
      xiiVisualScriptNodeDescription::Type::Builtin_IncVariable,
      xiiVisualScriptNodeDescription::Type::Builtin_DecVariable,
    };

    const char* szNodeTitles[] = {
      "++ {Name}",
      "-- {Name}",
    };

    static_assert(XII_ARRAY_SIZE(nodeTypes) == XII_ARRAY_SIZE(szNodeTitles));

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(nodeTypes); ++i)
    {
      xiiReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(nodeTypes[i]), "Variables", logicColor);

      AddInputProperty(typeDesc, "Name", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, szNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type           = nodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromVariableNameProperty;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Branch
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Branch", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Branch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Switch
  {
    xiiVisualScriptDataType::Enum switchDataTypes[] = {
      xiiVisualScriptDataType::Int64,
      xiiVisualScriptDataType::HashedString,
    };

    const char* szSwitchTypeNames[] = {
      "Builtin_SwitchInt64",
      "Builtin_SwitchString",
    };

    const char* szSwitchTitles[] = {
      "Int64::Switch",
      "HashedString::Switch",
    };

    static_assert(XII_ARRAY_SIZE(switchDataTypes) == XII_ARRAY_SIZE(szSwitchTypeNames));
    static_assert(XII_ARRAY_SIZE(switchDataTypes) == XII_ARRAY_SIZE(szSwitchTitles));

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(switchDataTypes); ++i)
    {
      const xiiRTTI* pValueType = xiiVisualScriptDataType::GetRtti(switchDataTypes[i]);

      xiiReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, szSwitchTypeNames[i], "Logic", logicColor);

      {
        auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = xiiPropertyCategory::Array;
        propDesc.m_sName    = "Cases";
        propDesc.m_sType    = pValueType->GetTypeName();
        propDesc.m_Flags    = xiiPropertyFlags::StandardType;

        auto pMaxSizeAttr = XII_DEFAULT_NEW(xiiMaxArraySizeAttribute, 16);
        propDesc.m_Attributes.PushBack(pMaxSizeAttr);

        auto pNoTempAttr = XII_DEFAULT_NEW(xiiNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);
      }

      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, szSwitchTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Switch;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("Case", xiiMakeHashedString("Cases"));
      nodeDesc.AddOutputExecutionPin("Default");

      nodeDesc.AddInputDataPin("Value", pValueType, switchDataTypes[i], true);

      m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_WhileLoop
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_WhileLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_WhileLoop;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("LoopBody");
    nodeDesc.AddOutputExecutionPin("Completed");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ForLoop
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ForLoop", "Logic", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "ForLoop [{FirstIndex}..{LastIndex}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_ForLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<int>(typeDesc, nodeDesc, "FirstIndex");
    AddInputDataPin<int>(typeDesc, nodeDesc, "LastIndex");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ForEachLoop
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ForEachLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_ForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<xiiVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<xiiVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_ReverseForEachLoop
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_ReverseForEachLoop", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<xiiVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<xiiVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Break
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Break", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Break;
    nodeDesc.AddInputExecutionPin("");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_And
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_And", "Logic", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_And;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Or
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Or", "Logic", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} OR {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Or;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Not
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Not", "Logic", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "NOT {A}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Not;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Compare
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Compare", "Logic", logicColor);

    AddInputProperty(typeDesc, "Operator", xiiGetStaticRTTI<xiiComparisonOperator>(), xiiVisualScriptDataType::Int64);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_CompareExec
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_CompareExec", "Logic", logicColor);

    AddInputProperty(typeDesc, "Operator", xiiGetStaticRTTI<xiiComparisonOperator>(), xiiVisualScriptDataType::Int64);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_CompareExec;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");
    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IsValid
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_IsValid", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Add, Builtin_Sub, Builtin_Mul, Builtin_Div
  {
    xiiVisualScriptNodeDescription::Type::Enum mathNodeTypes[] = {
      xiiVisualScriptNodeDescription::Type::Builtin_Add,
      xiiVisualScriptNodeDescription::Type::Builtin_Subtract,
      xiiVisualScriptNodeDescription::Type::Builtin_Multiply,
      xiiVisualScriptNodeDescription::Type::Builtin_Divide,
    };

    const char* szMathNodeTitles[] = {
      "{A} + {B}",
      "{A} - {B}",
      "{A} * {B}",
      "{A} / {B}",
    };

    static_assert(XII_ARRAY_SIZE(mathNodeTypes) == XII_ARRAY_SIZE(szMathNodeTitles));

    for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(mathNodeTypes); ++i)
    {
      xiiReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(mathNodeTypes[i]), "Math", mathColor);

      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, szMathNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type           = mathNodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, xiiVisualScriptDataType::Any);

      m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc
    {
      const char*                   m_szCategory;
      xiiColorGammaUB               m_Color;
      xiiVisualScriptDataType::Enum m_DataType;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {"Type Conversion", logicColor, xiiVisualScriptDataType::Bool},
      {"Type Conversion", mathColor, xiiVisualScriptDataType::Byte},
      {"Type Conversion", mathColor, xiiVisualScriptDataType::Int},
      {"Type Conversion", mathColor, xiiVisualScriptDataType::Int64},
      {"Type Conversion", mathColor, xiiVisualScriptDataType::Float},
      {"Type Conversion", mathColor, xiiVisualScriptDataType::Double},
      {"Type Conversion", stringColor, xiiVisualScriptDataType::String},
      {"Type Conversion", variantColor, xiiVisualScriptDataType::Variant},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = xiiVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      xiiReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_szCategory, conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type           = nodeType;
      nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", xiiVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_String_Format,
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_String_Format", "String", stringColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "String::Format {Text}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_String_Format;

    AddInputDataPin<xiiString>(typeDesc, nodeDesc, "Text");
    AddInputProperty(typeDesc, "Params", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array);
    nodeDesc.AddInputDataPin("Params", xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, false, xiiMakeHashedString("Params"));
    AddOutputDataPin<xiiString>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Variant_ConvertTo
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", "Type Conversion", variantColor);

    {
      auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName    = "Type";
      propDesc.m_sType    = xiiGetStaticRTTI<xiiVisualScriptDataType>()->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::IsEnum;

      auto pAttr = XII_DEFAULT_NEW(xiiDefaultValueAttribute, xiiVisualScriptDataType::Bool);
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Variant::ConvertTo {Type}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_Variant_ConvertTo;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("Succeeded");
    nodeDesc.AddOutputExecutionPin("Failed");
    nodeDesc.AddInputDataPin("Variant", xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, true);
    nodeDesc.AddOutputDataPin("Result", nullptr, xiiVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_MakeArray
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_MakeArray", "Array", variantColor);

    xiiHashedString sElements = xiiMakeHashedString("Elements");
    AddInputProperty(typeDesc, sElements, xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Array);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_MakeArray;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin(sElements, xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, false, sElements);
    nodeDesc.AddOutputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_TryGetComponentOfBaseType
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", "GameObject", gameObjectColor);

    {
      auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName    = "TypeName";
      propDesc.m_sType    = xiiGetStaticRTTI<xiiString>()->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::StandardType;

      auto pAttr = XII_DEFAULT_NEW(xiiDynamicStringEnumAttribute, "ComponentTypes");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "GameObject::TryGetComponentOfBaseType {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;

    nodeDesc.AddInputDataPin("GameObject", xiiGetStaticRTTI<xiiGameObject>(), xiiVisualScriptDataType::GameObject, false);
    AddOutputDataPin<xiiComponent>(nodeDesc, "Component");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StartCoroutine
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StartCoroutine", szCoroutinesCategory, coroutineColor);

    AddInputProperty(typeDesc, "CoroutineMode", xiiGetStaticRTTI<xiiScriptCoroutineCreationMode>(), xiiVisualScriptDataType::Int64);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "StartCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_StartCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("CoroutineBody", xiiHashedString(), true);
    AddInputDataPin<xiiString>(typeDesc, nodeDesc, "Name");
    nodeDesc.AddOutputDataPin("CoroutineID", xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), xiiVisualScriptDataType::Coroutine);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StopCoroutine
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StopCoroutine", szCoroutinesCategory, coroutineColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "StopCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_StopCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("CoroutineID", xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), xiiVisualScriptDataType::Coroutine, false);
    AddInputDataPin<xiiString>(typeDesc, nodeDesc, "Name");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_StopAllCoroutines
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_StopAllCoroutines", szCoroutinesCategory, coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_StopAllCoroutines;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_WaitForAll
  {
    xiiVisualScriptNodeDescription::Type::Enum waitTypes[] = {
      xiiVisualScriptNodeDescription::Type::Builtin_WaitForAll,
      xiiVisualScriptNodeDescription::Type::Builtin_WaitForAny,
    };

    for (auto waitType : waitTypes)
    {
      xiiReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(waitType), szCoroutinesCategory, coroutineColor);

      xiiHashedString sCount = xiiMakeHashedString("Count");
      {
        auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
        propDesc.m_Category = xiiPropertyCategory::Member;
        propDesc.m_sName    = sCount.GetView();
        propDesc.m_sType    = xiiGetStaticRTTI<xiiUInt32>()->GetTypeName();
        propDesc.m_Flags    = xiiPropertyFlags::StandardType;

        auto pNoTempAttr = XII_DEFAULT_NEW(xiiNoTemporaryTransactionsAttribute);
        propDesc.m_Attributes.PushBack(pNoTempAttr);

        auto pDefaultAttr = XII_DEFAULT_NEW(xiiDefaultValueAttribute, 1);
        propDesc.m_Attributes.PushBack(pDefaultAttr);

        auto pClampAttr = XII_DEFAULT_NEW(xiiClampValueAttribute, 1, 16);
        propDesc.m_Attributes.PushBack(pClampAttr);
      }

      NodeDesc nodeDesc;
      nodeDesc.m_Type = waitType;

      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddInputDataPin("", xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), xiiVisualScriptDataType::Coroutine, false, sCount);

      m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Yield
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Yield", szCoroutinesCategory, coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Yield;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void xiiVisualScriptNodeRegistry::CreateGetOwnerNodeType(const xiiRTTI* pRtti)
{
  xiiStringView sBaseClass = StripTypeName(pRtti->GetTypeName());

  xiiReflectedTypeDescriptor typeDesc;
  {
    xiiStringBuilder sTypeName;
    sTypeName.Set(sBaseClass, "::GetScriptOwner");

    xiiStringBuilder sCategory;
    sCategory.Set(sBaseClass);

    xiiColorGammaUB color = NiceColorFromName(sBaseClass);

    FillDesc(typeDesc, sTypeName, sCategory, color);
  }

  NodeDesc nodeDesc;
  nodeDesc.m_sFilterByBaseClass.Assign(sBaseClass);
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_Type        = xiiVisualScriptNodeDescription::Type::GetScriptOwner;

  xiiVisualScriptDataType::Enum scriptDataType;
  if (GetScriptDataType(pRtti, scriptDataType, "GetScriptOwner", "").Failed())
    return;

  if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    nodeDesc.AddOutputDataPin("World", xiiGetStaticRTTI<xiiWorld>(), xiiVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("GameObject", xiiGetStaticRTTI<xiiGameObject>(), xiiVisualScriptDataType::GameObject);
    nodeDesc.AddOutputDataPin("Component", xiiGetStaticRTTI<xiiComponent>(), xiiVisualScriptDataType::Component);
  }
  else
  {
    nodeDesc.AddOutputDataPin("World", xiiGetStaticRTTI<xiiWorld>(), xiiVisualScriptDataType::TypedPointer);
    nodeDesc.AddOutputDataPin("Owner", pRtti, scriptDataType);
  }

  m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void xiiVisualScriptNodeRegistry::CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiAbstractFunctionProperty* pFunction, const xiiScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction)
{
  xiiHashSet<xiiStringView> dynamicPins;
  for (auto pAttribute : pFunction->GetAttributes())
  {
    if (auto pDynamicPinAttribute = xiiDynamicCast<const xiiDynamicPinAttribute*>(pAttribute))
    {
      dynamicPins.Insert(pDynamicPinAttribute->GetProperty());
    }
  }

  xiiHybridArray<const xiiFunctionArgumentAttributes*, 8> argumentAttributes;
  CollectFunctionArgumentAttributes(pFunction, argumentAttributes);

  xiiStringView sTypeName = StripTypeName(pRtti->GetTypeName());

  xiiStringView sFunctionName = pFunction->GetPropertyName();
  sFunctionName.TrimWordStart("Reflection_");

  xiiReflectedTypeDescriptor typeDesc;
  bool                       bHasTitle = false;
  {
    if (bIsEntryFunction)
    {
      xiiStringBuilder sCategory;
      sCategory.Set(szEventHandlerCategory, sTypeName);

      xiiColorGammaUB color = NiceColorFromName(sTypeName);

      FillDesc(typeDesc, pRtti, sCategory, &color);
    }
    else
    {
      FillDesc(typeDesc, pRtti);
    }

    xiiStringBuilder temp;
    temp.Set(typeDesc.m_sTypeName, "::", sFunctionName);
    typeDesc.m_sTypeName = temp;

    if (bIsEntryFunction)
    {
      AddInputProperty(typeDesc, "CoroutineMode", xiiGetStaticRTTI<xiiScriptCoroutineCreationMode>(), xiiVisualScriptDataType::Int64);
    }

    if (auto pTitleAttribute = pFunction->GetAttributeByType<xiiTitleAttribute>())
    {
      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);

      bHasTitle = true;
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pFunction);
  if (bIsEntryFunction)
  {
    nodeDesc.m_sFilterByBaseClass.Assign(sTypeName);
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::EntryCall;
  }
  else
  {
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::ReflectedFunction;
  }

  {
    if (pFunction->GetFlags().IsSet(xiiPropertyFlags::Const) == false)
    {
      if (bIsEntryFunction == false)
      {
        nodeDesc.AddInputExecutionPin("");
      }
      nodeDesc.AddOutputExecutionPin("");
    }

    if (bIsEntryFunction == false)
    {
      if (pFunction->GetFunctionType() == xiiFunctionType::Member)
      {
        // GameObject and World pins will default to the script owner's game object/world thus they are not required
        const bool bRequired = pRtti->IsDerivedFrom<xiiGameObject>() == false && pRtti->IsDerivedFrom<xiiWorld>() == false;
        nodeDesc.AddInputDataPin(sTypeName, pRtti, xiiVisualScriptDataType::FromRtti(pRtti), bRequired);
      }

      if (const xiiRTTI* pReturnRtti = pFunction->GetReturnType())
      {
        xiiVisualScriptDataType::Enum scriptDataType;
        if (GetScriptDataType(pReturnRtti, scriptDataType, pFunction->GetPropertyName(), "return value").Failed())
        {
          return;
        }

        nodeDesc.AddOutputDataPin("Result", pReturnRtti, scriptDataType);
      }
    }

    xiiUInt32 titleArgIdx = xiiInvalidIndex;

    xiiStringBuilder sArgName;
    for (xiiUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
    {
      sArgName = pScriptableFunctionAttribute->GetArgumentName(argIdx);
      if (sArgName.IsEmpty())
        sArgName.Format("Arg{}", argIdx);

      auto       pArgRtti              = pFunction->GetArgumentType(argIdx);
      auto       argType               = pScriptableFunctionAttribute->GetArgumentType(argIdx);
      const bool bIsDynamicPinProperty = dynamicPins.Contains(sArgName);

      xiiHashedString sDynamicPinProperty;
      if (bIsDynamicPinProperty)
      {
        sDynamicPinProperty.Assign(sArgName);
      }

      xiiVisualScriptDataType::Enum scriptDataType;
      if (GetScriptDataType(pArgRtti, scriptDataType, pFunction->GetPropertyName(), sArgName).Failed())
      {
        return;
      }

      xiiVisualScriptDataType::Enum pinScriptDataType = scriptDataType;
      if (bIsDynamicPinProperty && scriptDataType == xiiVisualScriptDataType::Array)
      {
        pArgRtti          = xiiGetStaticRTTI<xiiVariant>();
        pinScriptDataType = xiiVisualScriptDataType::Variant;
      }

      if (bIsEntryFunction)
      {
        nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType);
      }
      else
      {
        if (argType == xiiScriptableFunctionAttribute::In || argType == xiiScriptableFunctionAttribute::Inout)
        {
          if (xiiVisualScriptDataType::IsPointer(scriptDataType) == false)
          {
            xiiArrayPtr<const xiiPropertyAttribute* const> attributes;
            if (argIdx < argumentAttributes.GetCount() && argumentAttributes[argIdx] != nullptr)
            {
              attributes = argumentAttributes[argIdx]->GetArgumentAttributes();
            }

            AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType, attributes);
          }

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty);

          if (titleArgIdx == xiiInvalidIndex &&
              (pinScriptDataType == xiiVisualScriptDataType::String || pinScriptDataType == xiiVisualScriptDataType::HashedString))
          {
            titleArgIdx = argIdx;
          }
        }
        else if (argType == xiiScriptableFunctionAttribute::Out || argType == xiiScriptableFunctionAttribute::Inout)
        {
          xiiLog::Error("Script function out parameter are not yet supported");
          return;

#if 0
          if (!pFunction->GetArgumentFlags(argIdx).IsSet(xiiPropertyFlags::Reference))
          {
            // TODO: xiiPropertyFlags::Reference is also set for const-ref parameters, should we change that ?

            xiiLog::Error("Script function '{}' argument {} is marked 'out' but is not a non-const reference value", pRtti->GetTypeName(), argIdx);
            return;
          }

          nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType, sDynamicPinProperty);
#endif
        }
      }
    }

    if (bIsEntryFunction)
    {
      nodeDesc.AddOutputDataPin("CoroutineID", xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), xiiVisualScriptDataType::Coroutine);
    }

    if (bHasTitle == false && titleArgIdx != xiiInvalidIndex)
    {
      xiiStringBuilder sTitle;
      sTitle.Set(GetTypeName(pRtti), "::", sFunctionName, " {", pScriptableFunctionAttribute->GetArgumentName(titleArgIdx), "}");

      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, sTitle);
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void xiiVisualScriptNodeRegistry::CreateCoroutineNodeType(const xiiRTTI* pRtti)
{
  if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
    return;

  const xiiAbstractFunctionProperty*    pStartFunc               = nullptr;
  const xiiScriptableFunctionAttribute* pScriptableFuncAttribute = nullptr;
  for (auto pFunc : pRtti->GetFunctions())
  {
    if (pFunc->GetPropertyName() == "Start")
    {
      if (auto pAttr = pFunc->GetAttributeByType<xiiScriptableFunctionAttribute>())
      {
        pStartFunc               = pFunc;
        pScriptableFuncAttribute = pAttr;
        break;
      }
    }
  }

  if (pStartFunc == nullptr || pScriptableFuncAttribute == nullptr)
  {
    xiiLog::Warning("The script coroutine '{}' has no reflected script function called 'Start'.", pRtti->GetTypeName());
    return;
  }

  xiiReflectedTypeDescriptor typeDesc;
  {
    const xiiColorGammaUB coroutineColor = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Coroutine);
    FillDesc(typeDesc, pRtti, szCoroutinesCategory, &coroutineColor);

    xiiStringBuilder temp;
    temp.Set("Coroutine::", typeDesc.m_sTypeName);
    typeDesc.m_sTypeName = temp;

    if (auto pTitleAttribute = pRtti->GetAttributeByType<xiiTitleAttribute>())
    {
      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_TargetProperties.PushBack(pStartFunc);
  nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::InplaceCoroutine;

  nodeDesc.AddInputExecutionPin("");
  nodeDesc.AddOutputExecutionPin("Succeeded");
  nodeDesc.AddOutputExecutionPin("Failed");

  xiiStringBuilder sArgName;
  for (xiiUInt32 argIdx = 0; argIdx < pStartFunc->GetArgumentCount(); ++argIdx)
  {
    sArgName = pScriptableFuncAttribute->GetArgumentName(argIdx);
    if (sArgName.IsEmpty())
      sArgName.Format("Arg{}", argIdx);

    auto pArgRtti = pStartFunc->GetArgumentType(argIdx);
    auto argType  = pScriptableFuncAttribute->GetArgumentType(argIdx);
    if (argType != xiiScriptableFunctionAttribute::In)
    {
      xiiLog::Error("Script function out parameter are not yet supported");
      return;
    }

    xiiVisualScriptDataType::Enum scriptDataType = xiiVisualScriptDataType::Invalid;
    if (GetScriptDataType(pArgRtti, scriptDataType, pStartFunc->GetPropertyName(), sArgName).Failed())
    {
      return;
    }

    if (xiiVisualScriptDataType::IsPointer(scriptDataType) == false)
    {
      AddInputProperty(typeDesc, sArgName, pArgRtti, scriptDataType);
    }

    nodeDesc.AddInputDataPin(sArgName, pArgRtti, scriptDataType, false);
  }

  m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
}

void xiiVisualScriptNodeRegistry::CreateMessageNodeTypes(const xiiRTTI* pRtti)
{
  if (pRtti == xiiGetStaticRTTI<xiiMessage>() || pRtti == xiiGetStaticRTTI<xiiEventMessage>() || pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
    return;

  // Message Handler
  {
    xiiReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti, szEventHandlerCategory);

      xiiStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "On", GetTypeName(pRtti));
      typeDesc.m_sTypeName = temp;

      AddInputProperty(typeDesc, "CoroutineMode", xiiGetStaticRTTI<xiiScriptCoroutineCreationMode>(), xiiVisualScriptDataType::Int64);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type        = xiiVisualScriptNodeDescription::Type::MessageHandler;

    nodeDesc.AddOutputExecutionPin("");

    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      auto                          pPropRtti      = pProp->GetSpecificType();
      xiiVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == xiiVisualScriptDataType::Invalid)
        continue;

      nodeDesc.AddOutputDataPin(pProp->GetPropertyName(), pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    nodeDesc.AddOutputDataPin("CoroutineID", xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), xiiVisualScriptDataType::Coroutine);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Message Sender
  {
    xiiReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti, "Messages");

      xiiStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "Send", GetTypeName(pRtti));
      typeDesc.m_sTypeName = temp;

      temp.Set("Send{?SendMode}", GetTypeName(pRtti), " {Delay}");
      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, temp);
      typeDesc.m_Attributes.PushBack(pAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type        = xiiVisualScriptNodeDescription::Type::SendMessage;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("GameObject", xiiGetStaticRTTI<xiiGameObject>(), xiiVisualScriptDataType::GameObject, false);
    nodeDesc.AddInputDataPin("Component", xiiGetStaticRTTI<xiiComponent>(), xiiVisualScriptDataType::Component, false);
    AddInputDataPin<xiiVisualScriptSendMessageMode>(typeDesc, nodeDesc, "SendMode");
    AddInputDataPin<xiiTime>(typeDesc, nodeDesc, "Delay");

    xiiHybridArray<const xiiAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
        continue;

      auto                          szPropName     = pProp->GetPropertyName();
      auto                          pPropRtti      = pProp->GetSpecificType();
      xiiVisualScriptDataType::Enum scriptDataType = GetScriptDataType(pProp);
      if (scriptDataType == xiiVisualScriptDataType::Invalid)
        continue;

      if (xiiVisualScriptDataType::IsPointer(scriptDataType) == false)
      {
        AddInputProperty(typeDesc, szPropName, pPropRtti, scriptDataType);
      }

      nodeDesc.AddInputDataPin(szPropName, pPropRtti, scriptDataType, false);
      nodeDesc.AddOutputDataPin(szPropName, pPropRtti, scriptDataType);

      nodeDesc.m_TargetProperties.PushBack(pProp);
    }

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void xiiVisualScriptNodeRegistry::CreateEnumNodeTypes(const xiiRTTI* pRtti)
{
  if (m_EnumTypes.Insert(pRtti))
    return;

  xiiStringView   sTypeName = GetTypeName(pRtti);
  xiiColorGammaUB enumColor = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::EnumValue);

  // Value
  {
    xiiStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Value");

    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, szEnumsCategory, enumColor);
    AddInputProperty(typeDesc, "Value", pRtti, xiiVisualScriptDataType::EnumValue);

    xiiStringBuilder sTitle;
    sTitle.Set(GetTypeName(pRtti), "::{Value}");

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type        = xiiVisualScriptNodeDescription::Type::Builtin_Constant;
    nodeDesc.AddOutputDataPin("Value", pRtti, xiiVisualScriptDataType::EnumValue);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Switch
  {
    xiiStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Switch");

    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, szEnumsCategory, enumColor);

    xiiStringBuilder sTitle;
    sTitle.Set(GetTypeName(pRtti), "::Switch");

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type        = xiiVisualScriptNodeDescription::Type::Builtin_Switch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddInputDataPin("Value", pRtti, xiiVisualScriptDataType::EnumValue, false);

    xiiHybridArray<xiiReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
    xiiReflectionUtils::GetEnumKeysAndValues(pRtti, enumKeysAndValues, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);
    for (auto& keyAndValue : enumKeysAndValues)
    {
      nodeDesc.AddOutputExecutionPin(keyAndValue.m_sKey);
    }

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }
}

void xiiVisualScriptNodeRegistry::FillDesc(xiiReflectedTypeDescriptor& desc, const xiiRTTI* pRtti, xiiStringView sCategoryOverride /*= xiiStringView()*/, const xiiColorGammaUB* pColorOverride /*= nullptr */)
{
  xiiStringBuilder sTypeName  = GetTypeName(pRtti);
  const xiiRTTI*   pBaseClass = FindTopMostBaseClass(pRtti);

  xiiStringBuilder sCategory;
  if (sCategoryOverride.IsEmpty())
  {
    if (pBaseClass != pRtti)
    {
      sCategory.Set(StripTypeName(pBaseClass->GetTypeName()), "/", sTypeName);
    }
    else
    {
      sCategory = sTypeName;
    }
  }
  else
  {
    sCategory = sCategoryOverride;
  }

  xiiColorGammaUB color;
  if (pColorOverride == nullptr)
  {
    if (pBaseClass != pRtti)
    {
      color = NiceColorFromName(sTypeName, StripTypeName(pBaseClass->GetTypeName()));
    }
    else
    {
      auto scriptDataType = xiiVisualScriptDataType::FromRtti(pRtti);
      if (scriptDataType != xiiVisualScriptDataType::Invalid &&
          scriptDataType != xiiVisualScriptDataType::Component &&
          scriptDataType != xiiVisualScriptDataType::TypedPointer)
      {
        color = PinDesc::GetColorForScriptDataType(scriptDataType);
      }
      else
      {
        color = NiceColorFromName(sTypeName);
      }
    }
  }
  else
  {
    color = *pColorOverride;
  }

  FillDesc(desc, sTypeName, sCategory, color);
}

void xiiVisualScriptNodeRegistry::FillDesc(xiiReflectedTypeDescriptor& desc, xiiStringView sTypeName, xiiStringView sCategory, const xiiColorGammaUB& color)
{
  xiiStringBuilder sTypeNameFull;
  sTypeNameFull.Set(s_szTypeNamePrefix, sTypeName);

  desc.m_sTypeName       = sTypeNameFull;
  desc.m_sPluginName     = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;

  // Category
  {
    xiiStringBuilder tmp;
    auto             pAttr = XII_DEFAULT_NEW(xiiCategoryAttribute, sCategory.GetData(tmp));
    desc.m_Attributes.PushBack(pAttr);
  }

  // Color
  {
    auto pAttr = XII_DEFAULT_NEW(xiiColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}
