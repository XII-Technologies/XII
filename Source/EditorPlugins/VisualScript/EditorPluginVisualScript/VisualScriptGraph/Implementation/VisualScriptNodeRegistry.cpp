/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>

namespace
{
  constexpr const char*  szPluginName            = "EditorPluginVisualScript";
  static xiiHashedString sEventHandlerCategory   = xiiMakeHashedString("Add Event Handler/");
  static xiiHashedString sCoroutinesCategory     = xiiMakeHashedString("Coroutines");
  static xiiHashedString sPropertiesCategory     = xiiMakeHashedString("Properties");
  static xiiHashedString sVariablesCategory      = xiiMakeHashedString("Variables");
  static xiiHashedString sLogicCategory          = xiiMakeHashedString("Logic");
  static xiiHashedString sMathCategory           = xiiMakeHashedString("Math");
  static xiiHashedString sTypeConversionCategory = xiiMakeHashedString("Type Conversion");
  static xiiHashedString sArrayCategory          = xiiMakeHashedString("Array");
  static xiiHashedString sMessagesCategory       = xiiMakeHashedString("Messages");
  static xiiHashedString sEnumsCategory          = xiiMakeHashedString("Enums");

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
    else if (pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags))
    {
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sType    = pRtti->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::Bitflags;
    }
    else
    {
      if (scriptDataType == xiiVisualScriptDataType::Color)
      {
        propDesc.m_Category = xiiPropertyCategory::Member;
        propDesc.m_sType    = pRtti->GetTypeName();
        propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiExposeColorAlphaAttribute));
      }
      else if (scriptDataType == xiiVisualScriptDataType::Variant)
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
  if (dataType == xiiVisualScriptDataType::EnumValue || dataType == xiiVisualScriptDataType::BitflagValue)
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

  if (m_ScriptDataType == xiiVisualScriptDataType::EnumValue || m_ScriptDataType == xiiVisualScriptDataType::BitflagValue)
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

void AddDataPin(xiiVisualScriptNodeRegistry::NodeDesc& inout_nodeDesc, xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, xiiHashedString sDynamicPinProperty, xiiVisualScriptNodeRegistry::PinDesc::DeductTypeFunc deductTypeFunc, bool bReplaceWithArray, xiiSmallArray<xiiVisualScriptNodeRegistry::PinDesc, 4>& inout_pins)
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
  pin.m_bReplaceWithArray   = bReplaceWithArray;

  inout_nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/, bool bReplaceWithArray /*= false*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, deductTypeFunc, bReplaceWithArray, m_InputPins);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/, PinDesc::DeductTypeFunc deductTypeFunc /*= nullptr*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, deductTypeFunc, false, m_OutputPins);
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

  auto& scriptBaseClassesDynEnum = xiiDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  xiiRTTI::ForEachType([this](const xiiRTTI* pRtti) { UpdateNodeType(pRtti); });

  for (const xiiRTTI* pRtti : m_TypesToUpdate)
  {
    if (m_ExposedTypes.Contains(pRtti) == false)
      UpdateNodeType(pRtti, true);
  }
  m_TypesToUpdate.Clear();
}

void xiiVisualScriptNodeRegistry::UpdateNodeType(const xiiRTTI* pRtti, bool bForceExpose /*= false*/)
{
  static xiiHashedString sType     = xiiMakeHashedString("Type");
  static xiiHashedString sProperty = xiiMakeHashedString("Property");
  static xiiHashedString sValue    = xiiMakeHashedString("Value");

  if (pRtti->GetAttributeByType<xiiHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<xiiExcludeFromScript>() != nullptr)
    return;

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
      // All components should be exposed to visual scripts, furthermore all classes that have script-able functions are also exposed
      bool bExposeToVisualScript  = pRtti->IsDerivedFrom<xiiComponent>() || bForceExpose;
      bool bHasBaseClassFunctions = false;

      xiiStringBuilder sCategory;
      {
        xiiStringView  sTypeName  = GetTypeName(pRtti);
        const xiiRTTI* pBaseClass = FindTopMostBaseClass(pRtti);
        if (pBaseClass != pRtti)
        {
          sCategory.Set(StripTypeName(pBaseClass->GetTypeName()), "/", sTypeName);
        }
        else
        {
          sCategory = sTypeName;
        }
      }

      xiiHashedString sCategoryHashed;
      sCategoryHashed.Assign(sCategory);

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

        CreateFunctionCallNodeType(pRtti, bIsBaseClassFunction ? sEventHandlerCategory : sCategoryHashed, pFuncProp, pScriptableFunctionAttribute, bIsBaseClassFunction);
      }

      if (bExposeToVisualScript && m_ExposedTypes.Insert(pRtti) == false)
      {
        xiiStringView    sTypeName = GetTypeName(pRtti);
        xiiStringBuilder sPropertyNodeTypeName;

        for (const xiiAbstractProperty* pProp : pRtti->GetProperties())
        {
          if (pProp->GetCategory() != xiiPropertyCategory::Member)
            continue;

          const xiiRTTI* pPropRtti = pProp->GetSpecificType();
          if (pPropRtti->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum))
          {
            CreateEnumNodeTypes(pPropRtti);
          }

          xiiUInt32 uiStart = m_PropertyValues.GetCount();
          m_PropertyValues.PushBack({sType, sTypeName});
          m_PropertyValues.PushBack({sProperty, pProp->GetPropertyName()});
          m_PropertyValues.PushBack({sValue, xiiReflectionUtils::GetDefaultValue(pProp)});

          // Setter
          {
            sPropertyNodeTypeName.Set("Set", pProp->GetPropertyName());
            auto it = m_PropertyNodeTypeNames.Insert(sPropertyNodeTypeName);

            auto& nodeTemplate                   = m_NodeCreationTemplates.ExpandAndGetRef();
            nodeTemplate.m_pType                 = m_pSetPropertyType;
            nodeTemplate.m_sTypeName             = it.Key();
            nodeTemplate.m_sCategory             = sCategoryHashed;
            nodeTemplate.m_uiPropertyValuesStart = uiStart;
            nodeTemplate.m_uiPropertyValuesCount = 3;
          }

          // Getter
          {
            sPropertyNodeTypeName.Set("Get", pProp->GetPropertyName());
            auto it = m_PropertyNodeTypeNames.Insert(sPropertyNodeTypeName);

            auto& nodeTemplate                   = m_NodeCreationTemplates.ExpandAndGetRef();
            nodeTemplate.m_pType                 = m_pGetPropertyType;
            nodeTemplate.m_sTypeName             = it.Key();
            nodeTemplate.m_sCategory             = sCategoryHashed;
            nodeTemplate.m_uiPropertyValuesStart = uiStart;
            nodeTemplate.m_uiPropertyValuesCount = 2;
          }
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
    GetScriptDataType(pProp->GetSpecificType(), result, "Member", pProp->GetPropertyName()).IgnoreResult();
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

  xiiReflectedTypeDescriptor typeDesc;

  // GetReflectedProperty
  {
    FillDesc(typeDesc, "GetProperty", logicColor);

    AddInputProperty(typeDesc, "Type", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{Type}::Get {Property}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::GetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputDataPin("Object", nullptr, xiiVisualScriptDataType::Any, true, xiiHashedString(), &xiiVisualScriptTypeDeduction::DeductFromTypeProperty);
    nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

    m_pGetPropertyType = RegisterNodeType(typeDesc, std::move(nodeDesc), sPropertiesCategory);
  }

  // SetReflectedProperty
  {
    FillDesc(typeDesc, "SetProperty", logicColor);

    AddInputProperty(typeDesc, "Type", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);
    AddInputProperty(typeDesc, "Property", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{Type}::Set {Property} = {Value}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::SetReflectedProperty;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromPropertyProperty;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Object", nullptr, xiiVisualScriptDataType::Any, true, xiiHashedString(), &xiiVisualScriptTypeDeduction::DeductFromTypeProperty);
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);

    m_pSetPropertyType = RegisterNodeType(typeDesc, std::move(nodeDesc), sPropertiesCategory);
  }

  // Builtin_GetVariable
  {
    FillDesc(typeDesc, "Builtin_GetVariable", logicColor);

    AddInputProperty(typeDesc, "Name", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Get {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_GetVariable;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromVariableNameProperty;
    nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

    m_pGetVariableType = RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_SetVariable
  {
    FillDesc(typeDesc, "Builtin_SetVariable", logicColor);

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

    m_pSetVariableType = RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
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
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(nodeTypes[i]), logicColor);

      AddInputProperty(typeDesc, "Name", xiiGetStaticRTTI<xiiString>(), xiiVisualScriptDataType::String);

      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, szNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type           = nodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromVariableNameProperty;
      nodeDesc.AddInputExecutionPin("");
      nodeDesc.AddOutputExecutionPin("");
      nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
    }
  }

  // Builtin_TempVariable
  {
    FillDesc(typeDesc, "Builtin_TempVariable", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_TempVariable;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    AddInputDataPin_Any(typeDesc, nodeDesc, "Value", false, true);
    nodeDesc.AddOutputDataPin("Value", nullptr, xiiVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sVariablesCategory);
  }

  // Builtin_Branch
  {
    FillDesc(typeDesc, "Builtin_Branch", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Branch;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("True");
    nodeDesc.AddOutputExecutionPin("False");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
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

      FillDesc(typeDesc, szSwitchTypeNames[i], logicColor);

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

      RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
    }
  }

  // Builtin_WhileLoop
  {
    FillDesc(typeDesc, "Builtin_WhileLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_WhileLoop;
    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("LoopBody");
    nodeDesc.AddOutputExecutionPin("Completed");

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ForLoop
  {
    FillDesc(typeDesc, "Builtin_ForLoop", logicColor);

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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ForEachLoop
  {
    FillDesc(typeDesc, "Builtin_ForEachLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_ForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<xiiVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<xiiVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_ReverseForEachLoop
  {
    FillDesc(typeDesc, "Builtin_ReverseForEachLoop", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop;
    nodeDesc.AddInputExecutionPin("");
    AddInputDataPin<xiiVariantArray>(typeDesc, nodeDesc, "Array");

    nodeDesc.AddOutputExecutionPin("LoopBody");
    AddOutputDataPin<xiiVariant>(nodeDesc, "Element");
    AddOutputDataPin<int>(nodeDesc, "Index");
    nodeDesc.AddOutputExecutionPin("Completed");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Break
  {
    FillDesc(typeDesc, "Builtin_Break", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Break;
    nodeDesc.AddInputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_And
  {
    FillDesc(typeDesc, "Builtin_And", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_And;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Or
  {
    FillDesc(typeDesc, "Builtin_Or", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} OR {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Or;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddInputDataPin<bool>(typeDesc, nodeDesc, "B");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Not
  {
    FillDesc(typeDesc, "Builtin_Not", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "NOT {A}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Not;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Compare
  {
    FillDesc(typeDesc, "Builtin_Compare", logicColor);

    AddInputProperty(typeDesc, "Operator", xiiGetStaticRTTI<xiiComparisonOperator>(), xiiVisualScriptDataType::Int64);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_CompareExec
  {
    FillDesc(typeDesc, "Builtin_CompareExec", logicColor);

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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_IsValid
  {
    FillDesc(typeDesc, "Builtin_IsValid", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
  }

  // Builtin_Select
  {
    FillDesc(typeDesc, "Builtin_Select", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{Condition} ? {A} : {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_Select;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "Condition");
    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    nodeDesc.AddOutputDataPin("", nullptr, xiiVisualScriptDataType::Any);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sLogicCategory);
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
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(mathNodeTypes[i]), mathColor);

      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, szMathNodeTitles[i]);
      typeDesc.m_Attributes.PushBack(pAttr);

      NodeDesc nodeDesc;
      nodeDesc.m_Type           = mathNodeTypes[i];
      nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
      AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
      nodeDesc.AddOutputDataPin("", nullptr, xiiVisualScriptDataType::Any);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
    }
  }

  // Builtin_Expression
  {
    FillDesc(typeDesc, "Builtin_Expression", mathColor);

    {
      auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName    = "Expression";
      propDesc.m_sType    = xiiGetStaticRTTI<xiiString>()->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::StandardType;

      auto pExpressionWidgetAttr = XII_DEFAULT_NEW(xiiExpressionWidgetAttribute, "Inputs", "Outputs");
      propDesc.m_Attributes.PushBack(pExpressionWidgetAttr);
    }

    {
      auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Array;
      propDesc.m_sName    = "Inputs";
      propDesc.m_sType    = xiiGetStaticRTTI<xiiVisualScriptExpressionVariable>()->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::Class;

      auto pMaxSizeAttr = XII_DEFAULT_NEW(xiiMaxArraySizeAttribute, 16);
      propDesc.m_Attributes.PushBack(pMaxSizeAttr);
    }

    {
      auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Array;
      propDesc.m_sName    = "Outputs";
      propDesc.m_sType    = xiiGetStaticRTTI<xiiVisualScriptExpressionVariable>()->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::Class;

      auto pMaxSizeAttr = XII_DEFAULT_NEW(xiiMaxArraySizeAttribute, 16);
      propDesc.m_Attributes.PushBack(pMaxSizeAttr);
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Expression::{Expression}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type           = xiiVisualScriptNodeDescription::Type::Builtin_Expression;
    nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductDummy;

    nodeDesc.AddInputDataPin("Input", nullptr, xiiVisualScriptDataType::Any, false, xiiMakeHashedString("Inputs"), &xiiVisualScriptTypeDeduction::DeductFromExpressionInput);
    nodeDesc.AddOutputDataPin("Output", nullptr, xiiVisualScriptDataType::Any, xiiMakeHashedString("Outputs"), &xiiVisualScriptTypeDeduction::DeductFromExpressionOutput);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMathCategory);
  }

  // Builtin_ToBool, Builtin_ToByte, Builtin_ToInt, Builtin_ToInt64, Builtin_ToFloat, Builtin_ToDouble, Builtin_ToString, Builtin_ToVariant,
  {
    struct ConversionNodeDesc
    {
      xiiColorGammaUB               m_Color;
      xiiVisualScriptDataType::Enum m_DataType;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {logicColor, xiiVisualScriptDataType::Bool},
      {mathColor, xiiVisualScriptDataType::Byte},
      {mathColor, xiiVisualScriptDataType::Int},
      {mathColor, xiiVisualScriptDataType::Int64},
      {mathColor, xiiVisualScriptDataType::Float},
      {mathColor, xiiVisualScriptDataType::Double},
      {stringColor, xiiVisualScriptDataType::String},
      {variantColor, xiiVisualScriptDataType::Variant},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = xiiVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type           = nodeType;
      nodeDesc.m_DeductTypeFunc = &xiiVisualScriptTypeDeduction::DeductFromAllInputPins;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", xiiVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      RegisterNodeType(typeDesc, std::move(nodeDesc), sTypeConversionCategory);
    }
  }

  // Builtin_String_Format
  {
    FillDesc(typeDesc, "Builtin_String_Format", stringColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "String::Format {Text}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_String_Format;

    AddInputDataPin<xiiString>(typeDesc, nodeDesc, "Text");
    AddInputProperty(typeDesc, "Params", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array);
    nodeDesc.AddInputDataPin("Params", xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, false, xiiMakeHashedString("Params"));
    AddOutputDataPin<xiiString>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), xiiMakeHashedString("String"));
  }

  // Builtin_Variant_ConvertTo
  {
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", variantColor);

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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sTypeConversionCategory);
  }

  // Builtin_MakeArray
  {
    FillDesc(typeDesc, "Builtin_MakeArray", variantColor);

    xiiHashedString sElements = xiiMakeHashedString("Elements");
    AddInputProperty(typeDesc, sElements, xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Array);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_MakeArray;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin(sElements, xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, false, sElements);
    nodeDesc.AddOutputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_GetElement
  {
    FillDesc(typeDesc, "Builtin_Array_GetElement", variantColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Array::GetElement[{Index}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_GetElement;

    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");
    AddOutputDataPin<xiiVariant>(nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_SetElement
  {
    FillDesc(typeDesc, "Builtin_Array_SetElement", variantColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Array::SetElement[{Index}]");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_SetElement;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");
    AddInputDataPin<xiiVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_GetCount
  {
    FillDesc(typeDesc, "Builtin_Array::GetCount", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_GetCount;

    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_IsEmpty
  {
    FillDesc(typeDesc, "Builtin_Array::IsEmpty", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_IsEmpty;

    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Clear
  {
    FillDesc(typeDesc, "Builtin_Array::Clear", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_Clear;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Contains
  {
    FillDesc(typeDesc, "Builtin_Array_Contains", variantColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Array::Contains {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_Contains;

    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<xiiVariant>(typeDesc, nodeDesc, "Element");
    AddOutputDataPin<bool>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_IndexOf
  {
    FillDesc(typeDesc, "Builtin_Array_IndexOf", variantColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Array::IndexOf {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_IndexOf;

    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<xiiVariant>(typeDesc, nodeDesc, "Element");
    AddInputDataPin<int>(typeDesc, nodeDesc, "StartIndex");
    AddOutputDataPin<int>(nodeDesc, "");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Insert
  {
    FillDesc(typeDesc, "Builtin_Array::Insert", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_Insert;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<xiiVariant>(typeDesc, nodeDesc, "Element");
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_PushBack
  {
    FillDesc(typeDesc, "Builtin_Array::PushBack", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_PushBack;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<xiiVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_PushBackRange
  {
    FillDesc(typeDesc, "Builtin_Array::PushBackRange", variantColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_PushBackRange;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    nodeDesc.AddInputDataPin("Range", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_Remove
  {
    FillDesc(typeDesc, "Builtin_Array_Remove", variantColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Array::Remove {Element}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_Remove;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<xiiVariant>(typeDesc, nodeDesc, "Element");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_Array_RemoveAt
  {
    FillDesc(typeDesc, "Builtin_Array_RemoveAt", variantColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Array::RemoveAt {Index}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Array_RemoveAt;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("Array", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array, true);
    AddInputDataPin<int>(typeDesc, nodeDesc, "Index");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sArrayCategory);
  }

  // Builtin_TryGetComponentOfBaseType
  {
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", gameObjectColor);

    {
      auto& propDesc      = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName    = "TypeName";
      propDesc.m_sType    = xiiGetStaticRTTI<xiiString>()->GetTypeName();
      propDesc.m_Flags    = xiiPropertyFlags::StandardType;

      auto pAttr = XII_DEFAULT_NEW(xiiRttiTypeStringAttribute, "xiiComponent");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "GameObject::TryGet {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;

    nodeDesc.AddInputDataPin("GameObject", xiiGetStaticRTTI<xiiGameObject>(), xiiVisualScriptDataType::GameObject, false);
    AddOutputDataPin<xiiComponent>(nodeDesc, "Component");

    RegisterNodeType(typeDesc, std::move(nodeDesc), xiiMakeHashedString("GameObject"));
  }

  // Builtin_StartCoroutine
  {
    FillDesc(typeDesc, "Builtin_StartCoroutine", coroutineColor);

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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_StopCoroutine
  {
    FillDesc(typeDesc, "Builtin_StopCoroutine", coroutineColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "StopCoroutine {Name}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_StopCoroutine;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddInputDataPin("CoroutineID", xiiGetStaticRTTI<xiiScriptCoroutineHandle>(), xiiVisualScriptDataType::Coroutine, false);
    AddInputDataPin<xiiString>(typeDesc, nodeDesc, "Name");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_StopAllCoroutines
  {
    FillDesc(typeDesc, "Builtin_StopAllCoroutines", coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_StopAllCoroutines;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }

  // Builtin_WaitForAll
  {
    xiiVisualScriptNodeDescription::Type::Enum waitTypes[] = {
      xiiVisualScriptNodeDescription::Type::Builtin_WaitForAll,
      xiiVisualScriptNodeDescription::Type::Builtin_WaitForAny,
    };

    for (auto waitType : waitTypes)
    {
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(waitType), coroutineColor);

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

      RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
    }
  }

  // Builtin_Yield
  {
    FillDesc(typeDesc, "Builtin_Yield", coroutineColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Yield;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");

    RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
  }
}

void xiiVisualScriptNodeRegistry::CreateGetOwnerNodeType(const xiiRTTI* pRtti)
{
  xiiStringView sBaseClass = StripTypeName(pRtti->GetTypeName());

  xiiReflectedTypeDescriptor typeDesc;
  {
    xiiStringBuilder sTypeName;
    sTypeName.Set(sBaseClass, "::GetScriptOwner");

    xiiColorGammaUB color = NiceColorFromName(sBaseClass);

    FillDesc(typeDesc, sTypeName, color);
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

  xiiHashedString sBaseClassHashed;
  sBaseClassHashed.Assign(sBaseClass);

  RegisterNodeType(typeDesc, std::move(nodeDesc), sBaseClassHashed);
}

void xiiVisualScriptNodeRegistry::CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiHashedString& sCategory, const xiiAbstractFunctionProperty* pFunction, const xiiScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction)
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
      xiiColorGammaUB color = NiceColorFromName(sTypeName);

      FillDesc(typeDesc, pRtti, &color);
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

        m_TypesToUpdate.Insert(pReturnRtti);

        nodeDesc.AddOutputDataPin("Result", pReturnRtti, scriptDataType);
      }
    }

    XII_ASSERT_ALWAYS(pFunction->GetArgumentCount() == pScriptableFunctionAttribute->GetArgumentCount(),
                      "The function reflection for '{}::{}' does not match the actual signature. Num arguments: {}, reflected arguments: {}.", sTypeName, sFunctionName, pFunction->GetArgumentCount(), pScriptableFunctionAttribute->GetArgumentCount());

    xiiUInt32 titleArgIdx = xiiInvalidIndex;

    xiiStringBuilder sArgName;
    for (xiiUInt32 argIdx = 0; argIdx < pFunction->GetArgumentCount(); ++argIdx)
    {
      sArgName = pScriptableFunctionAttribute->GetArgumentName(argIdx);
      if (sArgName.IsEmpty())
        sArgName.SetFormat("Arg{}", argIdx);

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

      xiiVisualScriptDataType::Enum pinScriptDataType          = scriptDataType;
      const bool                    bIsArrayDynamicPinProperty = bIsDynamicPinProperty && scriptDataType == xiiVisualScriptDataType::Array;
      if (bIsArrayDynamicPinProperty)
      {
        pArgRtti          = xiiGetStaticRTTI<xiiVariant>();
        pinScriptDataType = xiiVisualScriptDataType::Variant;
      }

      m_TypesToUpdate.Insert(pArgRtti);

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

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty, nullptr, bIsArrayDynamicPinProperty);

          if (titleArgIdx == xiiInvalidIndex &&
              (pinScriptDataType == xiiVisualScriptDataType::String || pinScriptDataType == xiiVisualScriptDataType::HashedString))
          {
            titleArgIdx = argIdx;
          }
        }

        if (argType == xiiScriptableFunctionAttribute::Out || argType == xiiScriptableFunctionAttribute::Inout)
        {
          if (!pFunction->GetArgumentFlags(argIdx).IsAnySet(xiiPropertyFlags::Reference | xiiPropertyFlags::Pointer))
          {
            xiiLog::Error("Script function '{}::{}' argument {} is marked 'out' but is not a non-const reference or pointer value", sTypeName, sFunctionName, argIdx);
            return;
          }

          nodeDesc.AddOutputDataPin(sArgName, pArgRtti, scriptDataType);
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

  RegisterNodeType(typeDesc, std::move(nodeDesc), sCategory);
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
    FillDesc(typeDesc, pRtti, &coroutineColor);

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
      sArgName.SetFormat("Arg{}", argIdx);

    auto pArgRtti = pStartFunc->GetArgumentType(argIdx);
    auto argType  = pScriptableFuncAttribute->GetArgumentType(argIdx);
    if (argType != xiiScriptableFunctionAttribute::In)
    {
      // xiiLog::Error("Script function out parameter are not yet supported");
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

  RegisterNodeType(typeDesc, std::move(nodeDesc), sCoroutinesCategory);
}

void xiiVisualScriptNodeRegistry::CreateMessageNodeTypes(const xiiRTTI* pRtti)
{
  if (pRtti == xiiGetStaticRTTI<xiiMessage>() ||
      pRtti == xiiGetStaticRTTI<xiiEventMessage>() ||
      pRtti->GetTypeFlags().IsSet(xiiTypeFlags::Abstract))
    return;

  xiiStringView sTypeName = GetTypeName(pRtti);

  // Message Handler
  {
    xiiReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti);

      xiiStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "On", sTypeName);
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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEventHandlerCategory);
  }

  // Message Sender
  {
    xiiReflectedTypeDescriptor typeDesc;
    {
      FillDesc(typeDesc, pRtti);

      xiiStringBuilder temp;
      temp.Set(s_szTypeNamePrefix, "Send", sTypeName);
      typeDesc.m_sTypeName = temp;

      temp.Set("Send{?SendMode}", sTypeName, " {Delay}");
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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sMessagesCategory);
  }
}

void xiiVisualScriptNodeRegistry::CreateEnumNodeTypes(const xiiRTTI* pRtti)
{
  if (m_ExposedTypes.Insert(pRtti))
    return;

  xiiStringView   sTypeName = GetTypeName(pRtti);
  xiiColorGammaUB enumColor = PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::EnumValue);

  // Value
  {
    xiiStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Value");

    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, enumColor);
    AddInputProperty(typeDesc, "Value", pRtti, xiiVisualScriptDataType::EnumValue);

    xiiStringBuilder sTitle;
    sTitle.Set(sTypeName, "::{Value}");

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, sTitle);
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_pTargetType = pRtti;
    nodeDesc.m_Type        = xiiVisualScriptNodeDescription::Type::Builtin_Constant;
    nodeDesc.AddOutputDataPin("Value", pRtti, xiiVisualScriptDataType::EnumValue);

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEnumsCategory);
  }

  // Switch
  {
    xiiStringBuilder sFullTypeName;
    sFullTypeName.Set(sTypeName, "Switch");

    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, sFullTypeName, enumColor);

    xiiStringBuilder sTitle;
    sTitle.Set(sTypeName, "::Switch");

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

    RegisterNodeType(typeDesc, std::move(nodeDesc), sEnumsCategory);
  }
}

void xiiVisualScriptNodeRegistry::FillDesc(xiiReflectedTypeDescriptor& desc, const xiiRTTI* pRtti, const xiiColorGammaUB* pColorOverride /*= nullptr */)
{
  xiiStringBuilder sTypeName  = GetTypeName(pRtti);
  const xiiRTTI*   pBaseClass = FindTopMostBaseClass(pRtti);

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

  FillDesc(desc, sTypeName, color);
}

void xiiVisualScriptNodeRegistry::FillDesc(xiiReflectedTypeDescriptor& desc, xiiStringView sTypeName, const xiiColorGammaUB& color)
{
  xiiStringBuilder sTypeNameFull;
  sTypeNameFull.Set(s_szTypeNamePrefix, sTypeName);

  desc                   = {};
  desc.m_sTypeName       = sTypeNameFull;
  desc.m_sPluginName     = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags           = xiiTypeFlags::Phantom | xiiTypeFlags::Class;

  // Color
  {
    auto pAttr = XII_DEFAULT_NEW(xiiColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}

const xiiRTTI* xiiVisualScriptNodeRegistry::RegisterNodeType(xiiReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const xiiHashedString& sCategory)
{
  const xiiRTTI* pRtti = xiiPhantomRttiManager::RegisterType(typeDesc);
  if (m_TypeToNodeDescs.Insert(pRtti, std::move(nodeDesc)) == false)
  {
    auto& nodeTemplate       = m_NodeCreationTemplates.ExpandAndGetRef();
    nodeTemplate.m_pType     = pRtti;
    nodeTemplate.m_sCategory = sCategory;
  }

  return pRtti;
}
