#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <Core/Messages/EventMessage.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

namespace
{
  constexpr const char* szPluginName = "EditorPluginVisualScript";
  constexpr const char* szEventHandlerCategory = "Add Event Handler/";

  const xiiRTTI* FindTopMostBaseClass(const xiiRTTI* pRtti)
  {
    const xiiRTTI* pReflectedClass = xiiGetStaticRTTI<xiiReflectedClass>();
    while (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != pReflectedClass)
    {
      pRtti = pRtti->GetParentType();
    }
    return pRtti;
  }

  xiiResult GetScriptDataType(const xiiRTTI* pRtti, xiiVisualScriptDataType::Enum& out_scriptDataType, xiiStringView sFunctionName, xiiStringView sArgName)
  {
    xiiVisualScriptDataType::Enum scriptDataType = xiiVisualScriptDataType::FromRtti(pRtti);
    if (scriptDataType == xiiVisualScriptDataType::Invalid)
    {
      xiiLog::Warning("The script function '{}' uses an argument '{}' of type '{}' which is not a valid script data type, therefore this function will not be available in visual scripts", sFunctionName, sArgName, pRtti->GetTypeName());
      return XII_FAILURE;
    }

    out_scriptDataType = scriptDataType;
    return XII_SUCCESS;
  }

  void AddInputProperty(xiiReflectedTypeDescriptor& typeDesc, xiiStringView name, xiiVisualScriptDataType::Enum scriptDataType)
  {
    auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
    propDesc.m_sName = name;
    propDesc.m_Flags = xiiPropertyFlags::StandardType;

    if (scriptDataType == xiiVisualScriptDataType::Variant)
    {
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sType = xiiGetStaticRTTI<xiiVariant>()->GetTypeName();
      propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiVisualScriptVariableAttribute));
    }
    else if (scriptDataType == xiiVisualScriptDataType::Array)
    {
      propDesc.m_Category = xiiPropertyCategory::Array;
      propDesc.m_sType = xiiGetStaticRTTI<xiiVariant>()->GetTypeName();
      propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiVisualScriptVariableAttribute));
    }
    else if (scriptDataType == xiiVisualScriptDataType::Map)
    {
      propDesc.m_Category = xiiPropertyCategory::Map;
      propDesc.m_sType = xiiGetStaticRTTI<xiiVariant>()->GetTypeName();
      propDesc.m_Attributes.PushBack(XII_DEFAULT_NEW(xiiVisualScriptVariableAttribute));
    }
    else
    {
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sType = xiiVisualScriptDataType::GetRtti(scriptDataType)->GetTypeName();
    }
  }

  template <typename T>
  void AddInputDataPin(xiiReflectedTypeDescriptor& typeDesc, xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, xiiStringView name)
  {
    const xiiRTTI* pDataType = xiiGetStaticRTTI<T>();

    xiiVisualScriptDataType::Enum scriptDataType;
    XII_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", name).Succeeded(), "Invalid script data type");

    AddInputProperty(typeDesc, name, scriptDataType);

    nodeDesc.AddInputDataPin(name, pDataType, scriptDataType, false);
  };

  void AddInputDataPin_Any(xiiReflectedTypeDescriptor& typeDesc, xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, xiiStringView name, bool bRequired, bool bAddVariantProperty = false)
  {
    if (bAddVariantProperty)
    {
      AddInputProperty(typeDesc, name, xiiVisualScriptDataType::Variant);
    }

    nodeDesc.AddInputDataPin(name, nullptr, xiiVisualScriptDataType::Any, bRequired);
    nodeDesc.m_bNeedsDataTypeDeduction = true;
  }

  template <typename T>
  void AddOutputDataPin(xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, xiiStringView name)
  {
    const xiiRTTI* pDataType = xiiGetStaticRTTI<T>();

    xiiVisualScriptDataType::Enum scriptDataType;
    XII_VERIFY(GetScriptDataType(pDataType, scriptDataType, "", name).Succeeded(), "Invalid script data type");

    nodeDesc.AddOutputDataPin(name, pDataType, scriptDataType);
  };

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
  xiiColorScheme::Blue,   // GameObject,
  xiiColorScheme::Blue,   // Component,
  xiiColorScheme::Blue,   // TypedPointer,
  xiiColorScheme::Pink,   // Variant,
  xiiColorScheme::Pink,   // VariantArray,
  xiiColorScheme::Pink,   // VariantDictionary,
};

static_assert(XII_ARRAY_SIZE(s_scriptDataTypeToPinColor) == xiiVisualScriptDataType::Count);

// static
xiiColor xiiVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(xiiVisualScriptDataType::Enum dataType)
{
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

  if (m_ScriptDataType == xiiVisualScriptDataType::Any)
  {
    return xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  }

  return xiiColorScheme::DarkUI(xiiColorScheme::Blue);
}

//////////////////////////////////////////////////////////////////////////

void AddExecutionPin(xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, xiiStringView sName, xiiHashedString sDynamicPinProperty, xiiSmallArray<xiiVisualScriptNodeRegistry::PinDesc, 4>& pins)
{
  auto& pin = pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType = nullptr;
  pin.m_ScriptDataType = xiiVisualScriptDataType::Invalid;

  nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddInputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, m_InputPins);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddOutputExecutionPin(xiiStringView sName, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/)
{
  AddExecutionPin(*this, sName, sDynamicPinProperty, m_OutputPins);
}

void AddDataPin(xiiVisualScriptNodeRegistry::NodeDesc& nodeDesc, xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, xiiHashedString sDynamicPinProperty, xiiSmallArray<xiiVisualScriptNodeRegistry::PinDesc, 4>& pins)
{
  auto& pin = pins.ExpandAndGetRef();
  pin.m_sName.Assign(sName);
  pin.m_sDynamicPinProperty = sDynamicPinProperty;
  pin.m_pDataType = pDataType;
  pin.m_ScriptDataType = scriptDataType;
  pin.m_bRequired = bRequired;

  nodeDesc.m_bHasDynamicPins |= (sDynamicPinProperty.IsEmpty() == false);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddInputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, bool bRequired, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, bRequired, sDynamicPinProperty, m_InputPins);
}

void xiiVisualScriptNodeRegistry::NodeDesc::AddOutputDataPin(xiiStringView sName, const xiiRTTI* pDataType, xiiVisualScriptDataType::Enum scriptDataType, const xiiHashedString& sDynamicPinProperty /*= xiiHashedString()*/)
{
  AddDataPin(*this, sName, pDataType, scriptDataType, false, sDynamicPinProperty, m_OutputPins);
}

//////////////////////////////////////////////////////////////////////////

XII_IMPLEMENT_SINGLETON(xiiVisualScriptNodeRegistry);

xiiVisualScriptNodeRegistry::xiiVisualScriptNodeRegistry()
  : m_SingletonRegistrar(this)
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
    desc.m_sTypeName = "xiiVisualScriptNodeBase";
    desc.m_sPluginName = szPluginName;
    desc.m_sParentTypeName = xiiGetStaticRTTI<xiiReflectedClass>()->GetTypeName();
    desc.m_Flags = xiiTypeFlags::Phantom | xiiTypeFlags::Abstract | xiiTypeFlags::Class;

    m_pBaseType = xiiPhantomRttiManager::RegisterType(desc);
  }

  if (m_bBuiltinTypesCreated == false)
  {
    CreateBuiltinTypes();
    m_bBuiltinTypesCreated = true;
  }

  auto& componentTypesDynEnum = xiiDynamicStringEnum::CreateDynamicEnum("ComponentTypes");
  auto& scriptBaseClassesDynEnum = xiiDynamicStringEnum::CreateDynamicEnum("ScriptBaseClasses");

  for (const xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    UpdateNodeType(pRtti);
  }
}

void xiiVisualScriptNodeRegistry::UpdateNodeType(const xiiRTTI* pRtti)
{
  if (pRtti->GetAttributeByType<xiiHiddenAttribute>() != nullptr)
    return;

  if (pRtti->IsDerivedFrom<xiiComponent>())
  {
    auto& componentTypesDynEnum = xiiDynamicStringEnum::GetDynamicEnum("ComponentTypes");
    componentTypesDynEnum.AddValidValue(pRtti->GetTypeName(), true);
  }

  // expose reflected functions and properties to visual scripts
  {
    bool bHasBaseClassFunctions = false;

    for (const xiiAbstractFunctionProperty* pFuncProp : pRtti->GetFunctions())
    {
      bool bIsBaseClassFunction = pFuncProp->GetAttributeByType<xiiScriptBaseClassFunctionAttribute>() != nullptr;
      if (bIsBaseClassFunction)
      {
        bHasBaseClassFunctions = true;
      }

      CreateFunctionCallNodeType(pRtti, pFuncProp, bIsBaseClassFunction);
    }

    if (bHasBaseClassFunctions)
    {
      auto& scriptBaseClassesDynEnum = xiiDynamicStringEnum::GetDynamicEnum("ScriptBaseClasses");
      scriptBaseClassesDynEnum.AddValidValue(StripTypeName(pRtti->GetTypeName()));

      CreateGetOwnerNodeType(pRtti);
    }
  }
}

void xiiVisualScriptNodeRegistry::CreateBuiltinTypes()
{
  xiiColorGammaUB logicColor = xiiColorScheme::DarkUI(xiiColorScheme::Gray);
  xiiColorGammaUB mathColor = xiiColorScheme::DarkUI(xiiColorScheme::Teal);
  xiiColorGammaUB stringColor = xiiColorScheme::DarkUI(xiiColorScheme::Grape);
  xiiColorGammaUB gameObjectColor = xiiColorScheme::DarkUI(xiiColorScheme::Blue);
  xiiColorGammaUB variantColor = xiiColorScheme::DarkUI(xiiColorScheme::Pink);

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

  // Builtin_And
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_And", "Logic", logicColor);

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} AND {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_And;
    nodeDesc.m_bImplicitExecution = true;

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
    nodeDesc.m_bImplicitExecution = true;

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
    nodeDesc.m_bImplicitExecution = true;

    AddInputDataPin<bool>(typeDesc, nodeDesc, "A");
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_Compare
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Compare", "Logic", logicColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName = "Operator";
      propDesc.m_sType = xiiGetStaticRTTI<xiiComparisonOperator>()->GetTypeName();
      propDesc.m_Flags = xiiPropertyFlags::IsEnum;
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "{A} {Operator} {B}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Compare;
    nodeDesc.m_bImplicitExecution = true;

    AddInputDataPin_Any(typeDesc, nodeDesc, "A", false, true);
    AddInputDataPin_Any(typeDesc, nodeDesc, "B", false, true);
    AddOutputDataPin<bool>(nodeDesc, "");

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_IsValid
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_IsValid", "Logic", logicColor);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_IsValid;
    nodeDesc.m_bImplicitExecution = true;

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
      nodeDesc.m_Type = mathNodeTypes[i];
      nodeDesc.m_bImplicitExecution = true;

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
      xiiVisualScriptDataType::Enum m_DataType;
      const char* m_szCategory;
      xiiColorGammaUB m_Color;
    };

    ConversionNodeDesc conversionNodeDescs[] = {
      {xiiVisualScriptDataType::Bool, "Logic", logicColor},
      {xiiVisualScriptDataType::Byte, "Math", mathColor},
      {xiiVisualScriptDataType::Int, "Math", mathColor},
      {xiiVisualScriptDataType::Int64, "Math", mathColor},
      {xiiVisualScriptDataType::Float, "Math", mathColor},
      {xiiVisualScriptDataType::Double, "Math", mathColor},
      {xiiVisualScriptDataType::String, "String", stringColor},
      {xiiVisualScriptDataType::Variant, "Variant", variantColor},
    };

    for (auto& conversionNodeDesc : conversionNodeDescs)
    {
      auto nodeType = xiiVisualScriptNodeDescription::Type::GetConversionType(conversionNodeDesc.m_DataType);

      xiiReflectedTypeDescriptor typeDesc;
      FillDesc(typeDesc, xiiVisualScriptNodeDescription::Type::GetName(nodeType), conversionNodeDesc.m_szCategory, conversionNodeDesc.m_Color);

      NodeDesc nodeDesc;
      nodeDesc.m_Type = nodeType;
      nodeDesc.m_bImplicitExecution = true;

      AddInputDataPin_Any(typeDesc, nodeDesc, "", true);
      nodeDesc.AddOutputDataPin("", xiiVisualScriptDataType::GetRtti(conversionNodeDesc.m_DataType), conversionNodeDesc.m_DataType);

      m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
    }
  }

  // Builtin_Variant_ConvertTo
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_Variant_ConvertTo", "Variant", variantColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName = "Type";
      propDesc.m_sType = xiiGetStaticRTTI<xiiVisualScriptDataType>()->GetTypeName();
      propDesc.m_Flags = xiiPropertyFlags::IsEnum;

      auto pAttr = XII_DEFAULT_NEW(xiiDefaultValueAttribute, xiiVisualScriptDataType::Bool);
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "Variant::ConvertTo {Type}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_Variant_ConvertTo;
    nodeDesc.m_bNeedsDataTypeDeduction = true;

    nodeDesc.AddInputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("");
    nodeDesc.AddOutputExecutionPin("Failed");
    nodeDesc.AddInputDataPin("Variant", xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, true);
    nodeDesc.AddOutputDataPin("Result", nullptr, xiiVisualScriptDataType::Any);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_MakeArray
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_MakeArray", "Array", variantColor);

    xiiHashedString sCount = xiiMakeHashedString("Count");
    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName = sCount.GetView();
      propDesc.m_sType = xiiGetStaticRTTI<xiiUInt32>()->GetTypeName();
      propDesc.m_Flags = xiiPropertyFlags::StandardType;

      auto pNoTempAttr = XII_DEFAULT_NEW(xiiNoTemporaryTransactionsAttribute);
      propDesc.m_Attributes.PushBack(pNoTempAttr);

      auto pClampAttr = XII_DEFAULT_NEW(xiiClampValueAttribute, 0, 16);
      propDesc.m_Attributes.PushBack(pClampAttr);
    }

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_MakeArray;
    nodeDesc.m_bImplicitExecution = true;

    nodeDesc.AddInputDataPin("", xiiGetStaticRTTI<xiiVariant>(), xiiVisualScriptDataType::Variant, false, sCount);
    nodeDesc.AddOutputDataPin("", xiiGetStaticRTTI<xiiVariantArray>(), xiiVisualScriptDataType::Array);

    m_TypeToNodeDescs.Insert(xiiPhantomRttiManager::RegisterType(typeDesc), std::move(nodeDesc));
  }

  // Builtin_TryGetComponentOfBaseType
  {
    xiiReflectedTypeDescriptor typeDesc;
    FillDesc(typeDesc, "Builtin_TryGetComponentOfBaseType", "GameObject", gameObjectColor);

    {
      auto& propDesc = typeDesc.m_Properties.ExpandAndGetRef();
      propDesc.m_Category = xiiPropertyCategory::Member;
      propDesc.m_sName = "TypeName";
      propDesc.m_sType = xiiGetStaticRTTI<xiiString>()->GetTypeName();
      propDesc.m_Flags = xiiPropertyFlags::StandardType;

      auto pAttr = XII_DEFAULT_NEW(xiiDynamicStringEnumAttribute, "ComponentTypes");
      propDesc.m_Attributes.PushBack(pAttr);
    }

    auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, "GameObject::TryGetComponentOfBaseType {TypeName}");
    typeDesc.m_Attributes.PushBack(pAttr);

    NodeDesc nodeDesc;
    nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::Builtin_TryGetComponentOfBaseType;
    nodeDesc.m_bImplicitExecution = true;

    nodeDesc.AddInputDataPin("GameObject", xiiGetStaticRTTI<xiiGameObject>(), xiiVisualScriptDataType::GameObject, true);
    AddOutputDataPin<xiiComponent>(nodeDesc, "Component");

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
  nodeDesc.m_Type = xiiVisualScriptNodeDescription::Type::GetScriptOwner;
  nodeDesc.m_bImplicitExecution = true;

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

void xiiVisualScriptNodeRegistry::CreateFunctionCallNodeType(const xiiRTTI* pRtti, const xiiAbstractFunctionProperty* pFunction, bool bIsEntryFunction)
{
  const xiiScriptableFunctionAttribute* pScriptableFunctionAttribute = pFunction->GetAttributeByType<xiiScriptableFunctionAttribute>();
  if (pScriptableFunctionAttribute == nullptr)
    return;

  xiiHashSet<xiiStringView> dynamicPins;
  for (auto pAttribute : pFunction->GetAttributes())
  {
    if (auto pDynamicPinAttribute = xiiDynamicCast<xiiDynamicPinAttribute*>(pAttribute))
    {
      dynamicPins.Insert(pDynamicPinAttribute->GetProperty());
    }
  }

  xiiStringView sTypeName = StripTypeName(pRtti->GetTypeName());

  xiiStringView sFunctionName = pFunction->GetPropertyName();
  sFunctionName.TrimWordStart("Reflection_");

  xiiReflectedTypeDescriptor typeDesc;
  bool bHasTitle = false;
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

    if (auto pTitleAttribute = pFunction->GetAttributeByType<xiiTitleAttribute>())
    {
      auto pAttr = XII_DEFAULT_NEW(xiiTitleAttribute, pTitleAttribute->GetTitle());
      typeDesc.m_Attributes.PushBack(pAttr);

      bHasTitle = true;
    }
  }

  NodeDesc nodeDesc;
  nodeDesc.m_pTargetType = pRtti;
  nodeDesc.m_pTargetProperty = pFunction;
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
    if (pFunction->GetFlags().IsSet(xiiPropertyFlags::Const))
    {
      nodeDesc.m_bImplicitExecution = true;
    }
    else
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
        if (pRtti->IsDerivedFrom<xiiComponent>())
        {
          nodeDesc.AddInputDataPin("Component", pRtti, xiiVisualScriptDataType::Component, true);
        }
        else if (pRtti->IsDerivedFrom<xiiGameObject>())
        {
          nodeDesc.AddInputDataPin("GameObject", pRtti, xiiVisualScriptDataType::GameObject, true);
        }
        else
        {
          nodeDesc.AddInputDataPin("Object", pRtti, xiiVisualScriptDataType::TypedPointer, true);
        }
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

      auto pArgRtti = pFunction->GetArgumentType(argIdx);
      auto argType = pScriptableFunctionAttribute->GetArgumentType(argIdx);
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
        pArgRtti = xiiGetStaticRTTI<xiiVariant>();
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
          if (scriptDataType != xiiVisualScriptDataType::GameObject &&
              scriptDataType != xiiVisualScriptDataType::Component &&
              scriptDataType != xiiVisualScriptDataType::TypedPointer)
          {
            AddInputProperty(typeDesc, sArgName, scriptDataType);
          }

          nodeDesc.AddInputDataPin(sArgName, pArgRtti, pinScriptDataType, false, sDynamicPinProperty);

          if (titleArgIdx == xiiInvalidIndex && pinScriptDataType == xiiVisualScriptDataType::String)
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

void xiiVisualScriptNodeRegistry::FillDesc(xiiReflectedTypeDescriptor& desc, const xiiRTTI* pRtti, xiiStringView sCategoryOverride /*= xiiStringView()*/, xiiColorGammaUB* pColorOverride /*= nullptr */)
{
  xiiStringBuilder sTypeName = GetTypeName(pRtti);
  const xiiRTTI* pBaseClass = FindTopMostBaseClass(pRtti);

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

  desc.m_sTypeName = sTypeNameFull;
  desc.m_sPluginName = szPluginName;
  desc.m_sParentTypeName = m_pBaseType->GetTypeName();
  desc.m_Flags = xiiTypeFlags::Phantom | xiiTypeFlags::Class;

  // Category
  {
    xiiStringBuilder tmp;
    auto pAttr = XII_DEFAULT_NEW(xiiCategoryAttribute, sCategory.GetData(tmp));
    desc.m_Attributes.PushBack(pAttr);
  }

  // Color
  {
    auto pAttr = XII_DEFAULT_NEW(xiiColorAttribute, color);
    desc.m_Attributes.PushBack(pAttr);
  }
}
