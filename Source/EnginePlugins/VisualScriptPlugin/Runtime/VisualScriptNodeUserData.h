#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

using SerializeFunction   = xiiResult (*)(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_Size, xiiUInt32& out_alignment);
using DeserializeFunction = xiiResult (*)(xiiVisualScriptGraphDescription::Node& node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData);
using ToStringFunction    = void (*)(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult);

namespace
{
  template <typename T, typename U>
  static xiiUInt32 GetDynamicSize(xiiUInt32 uiCount)
  {
    xiiUInt32 uiSize = sizeof(T);
    if (uiCount > 1)
    {
      uiSize += sizeof(U) * (uiCount - 1);
    }
    return uiSize;
  }


  template <typename T>
  static constexpr xiiUInt32 GetUserDataAlignment()
  {
    return xiiVisualScriptGraphDescription::Node::GetUserDataAlignment<T>();
  }

  struct NodeUserData_Type
  {
    const xiiRTTI* m_pType = nullptr;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding;
#endif

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      inout_stream << nodeDesc.m_sTargetTypeName;

      out_uiSize      = sizeof(NodeUserData_Type);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Type>();
      return XII_SUCCESS;
    }

    static xiiResult ReadType(xiiStreamReader& inout_stream, const xiiRTTI*& out_pType)
    {
      xiiStringBuilder sTypeName;
      inout_stream >> sTypeName;

      out_pType = xiiRTTI::FindTypeByName(sTypeName);
      if (out_pType == nullptr)
      {
        xiiLog::Error("Unknown type '{}'", sTypeName);
        return XII_FAILURE;
      }

      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Type>(inout_pAdditionalData);
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      if (nodeDesc.m_sTargetTypeName.IsEmpty() == false)
      {
        out_sResult.Append(nodeDesc.m_sTargetTypeName);
      }
    }
  };

  static_assert(sizeof(NodeUserData_Type) == 8);
  static_assert(GetUserDataAlignment<NodeUserData_Type>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperty : public NodeUserData_Type
  {
    const xiiAbstractProperty* m_pProperty = nullptr;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding;
#endif

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      XII_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const xiiVariantArray& propertiesVar = nodeDesc.m_Value.Get<xiiVariantArray>();
      XII_ASSERT_DEBUG(propertiesVar.GetCount() == 1, "Invalid number of properties");

      inout_stream << propertiesVar[0].Get<xiiHashedString>();

      out_uiSize      = sizeof(NodeUserData_TypeAndProperty);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_TypeAndProperty>();
      return XII_SUCCESS;
    }

    template <typename T>
    static xiiResult ReadProperty(xiiStreamReader& inout_stream, const xiiRTTI* pType, xiiArrayPtr<T> properties, const xiiAbstractProperty*& out_pProp)
    {
      xiiStringBuilder sPropName;
      inout_stream >> sPropName;

      out_pProp = nullptr;
      for (auto& pProp : properties)
      {
        if (sPropName == pProp->GetPropertyName())
        {
          out_pProp = pProp;
          break;
        }
      }

      if (out_pProp == nullptr)
      {
        constexpr bool isFunction = std::is_same_v<T, const xiiAbstractFunctionProperty* const>;
        xiiLog::Error("{} '{}' not found on type '{}'", isFunction ? "Function" : "Property", sPropName, pType->GetTypeName());
        return XII_FAILURE;
      }

      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndProperty>(inout_pAdditionalData);
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      XII_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetProperties(), userData.m_pProperty));

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      if (nodeDesc.m_Value.IsA<xiiVariantArray>())
      {
        const xiiVariantArray& propertiesVar = nodeDesc.m_Value.Get<xiiVariantArray>();
        if (propertiesVar.IsEmpty() == false)
        {
          out_sResult.Append(".", propertiesVar[0].Get<xiiHashedString>());
        }
      }
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperty) == 16);
  static_assert(GetUserDataAlignment<NodeUserData_TypeAndProperty>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndProperties : public NodeUserData_Type
  {
    xiiUInt32 m_uiNumProperties = 0;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding0;
#endif

    // This struct is allocated with enough space behind it to hold an array with m_uiNumProperties size.
    const xiiAbstractProperty* m_Properties[1];

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding1;
#endif

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      XII_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const xiiVariantArray& propertiesVar = nodeDesc.m_Value.Get<xiiVariantArray>();

      xiiUInt32 uiCount = propertiesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : propertiesVar)
      {
        xiiHashedString sPropName = var.Get<xiiHashedString>();
        inout_stream << sPropName;
      }

      static_assert(sizeof(void*) <= sizeof(xiiUInt64));
      out_uiSize      = GetDynamicSize<NodeUserData_TypeAndProperties, xiiUInt64>(uiCount);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_TypeAndProperties>();
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      const xiiRTTI* pType = nullptr;
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, pType));

      xiiUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const xiiUInt32 uiByteSize = GetDynamicSize<NodeUserData_TypeAndProperties, xiiUInt64>(uiCount);
      auto&           userData   = ref_node.InitUserData<NodeUserData_TypeAndProperties>(inout_pAdditionalData, uiByteSize);
      userData.m_pType           = pType;
      userData.m_uiNumProperties = uiCount;

      xiiHybridArray<const xiiAbstractProperty*, 32> properties;
      userData.m_pType->GetAllProperties(properties);

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        const xiiAbstractProperty* pProperty = nullptr;
        XII_SUCCEED_OR_RETURN(NodeUserData_TypeAndProperty::ReadProperty(inout_stream, userData.m_pType, properties.GetArrayPtr(), pProperty));
        userData.m_Properties[i] = pProperty;
      }

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      NodeUserData_TypeAndProperty::ToString(nodeDesc, out_sResult);
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperties) == 24);
  static_assert(GetUserDataAlignment<NodeUserData_TypeAndProperties>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_TypeAndFunction : public NodeUserData_TypeAndProperty
  {
    xiiUInt32 m_uiInputArgsMask  = 0;
    xiiUInt32 m_uiOutputArgsMask = 0;

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      XII_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      const xiiVariantArray& propertiesVar = nodeDesc.m_Value.Get<xiiVariantArray>();
      XII_ASSERT_DEBUG(propertiesVar.GetCount() == 1, "Invalid number of properties");

      xiiHashedString sFunctionName = propertiesVar[0].Get<xiiHashedString>();
      inout_stream << sFunctionName;

      const xiiRTTI* pType = xiiRTTI::FindTypeByName(nodeDesc.m_sTargetTypeName);
      if (pType == nullptr)
        return XII_FAILURE;

      const xiiAbstractFunctionProperty* pFunction = nullptr;
      for (auto pFunc : pType->GetFunctions())
      {
        if (pFunc->GetPropertyName() == sFunctionName)
        {
          pFunction = pFunc;
          break;
        }
      }

      if (pFunction == nullptr)
        return XII_FAILURE;

      auto pScriptableFunctionAttribute = pFunction->GetAttributeByType<xiiScriptableFunctionAttribute>();
      if (pScriptableFunctionAttribute == nullptr)
        return XII_FAILURE;

      xiiUInt32 uiInputArgsMask  = 0;
      xiiUInt32 uiOutputArgsMask = 0;
      for (xiiUInt32 i = 0; i < pScriptableFunctionAttribute->GetArgumentCount(); ++i)
      {
        auto argType = pScriptableFunctionAttribute->GetArgumentType(i);
        if (argType == xiiScriptableFunctionAttribute::In || argType == xiiScriptableFunctionAttribute::Inout)
        {
          uiInputArgsMask |= XII_BIT(i);
        }

        if (argType == xiiScriptableFunctionAttribute::Out || argType == xiiScriptableFunctionAttribute::Inout)
        {
          uiOutputArgsMask |= XII_BIT(i);
        }
      }

      inout_stream << uiInputArgsMask;
      inout_stream << uiOutputArgsMask;

      out_uiSize      = sizeof(NodeUserData_TypeAndFunction);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_TypeAndFunction>();
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_TypeAndFunction>(inout_pAdditionalData);
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      XII_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetFunctions(), userData.m_pProperty));

      inout_stream >> userData.m_uiInputArgsMask;
      inout_stream >> userData.m_uiOutputArgsMask;

      if (static_cast<const xiiAbstractFunctionProperty*>(userData.m_pProperty)->GetArgumentCount() != xiiMath::CountBits(userData.m_uiInputArgsMask | userData.m_uiOutputArgsMask))
      {
        xiiLog::Error("Visual script {} '{}': Argument count mismatch. Script needs re-transform.", xiiVisualScriptNodeDescription::Type::GetName(ref_node.m_Type), userData.m_pProperty->GetPropertyName());
        return XII_FAILURE;
      }

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      NodeUserData_TypeAndProperty::ToString(nodeDesc, out_sResult);
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndFunction) == 24);
  static_assert(GetUserDataAlignment<NodeUserData_TypeAndFunction>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Switch
  {
    xiiUInt32 m_uiNumCases = 0;

    // This struct is allocated with enough space behind it to hold an array with m_uiNumCases size.
    xiiInt64 m_Cases[1];

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      const xiiVariantArray& casesVar = nodeDesc.m_Value.Get<xiiVariantArray>();

      xiiUInt32 uiCount = casesVar.GetCount();
      inout_stream << uiCount;

      for (auto& var : casesVar)
      {
        xiiInt64 iCaseValue = var.ConvertTo<xiiInt64>();
        inout_stream << iCaseValue;
      }

      out_uiSize      = GetDynamicSize<NodeUserData_Switch, xiiInt64>(uiCount);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Switch>();
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      xiiUInt32 uiCount = 0;
      inout_stream >> uiCount;

      const xiiUInt32 uiByteSize = GetDynamicSize<NodeUserData_Switch, xiiInt64>(uiCount);
      auto&           userData   = ref_node.InitUserData<NodeUserData_Switch>(inout_pAdditionalData, uiByteSize);
      userData.m_uiNumCases      = uiCount;

      for (xiiUInt32 i = 0; i < uiCount; ++i)
      {
        inout_stream >> userData.m_Cases[i];
      }

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      // Nothing to add here
    }
  };

  static_assert(sizeof(NodeUserData_Switch) == 16);
  static_assert(GetUserDataAlignment<NodeUserData_Switch>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Comparison
  {
    xiiEnum<xiiComparisonOperator> m_ComparisonOperator;

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      xiiEnum<xiiComparisonOperator> compOp = static_cast<xiiComparisonOperator::Enum>(nodeDesc.m_Value.Get<xiiInt64>());
      inout_stream << compOp;

      out_uiSize      = sizeof(NodeUserData_Comparison);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Comparison>();
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Comparison>(inout_pAdditionalData);
      inout_stream >> userData.m_ComparisonOperator;

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      xiiStringBuilder sCompOp;
      xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiComparisonOperator>(), nodeDesc.m_Value.Get<xiiInt64>(), sCompOp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCompOp);
    }
  };

  static_assert(sizeof(NodeUserData_Comparison) == 1);
  static_assert(GetUserDataAlignment<NodeUserData_Comparison>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Expression
  {
    xiiExpressionByteCode m_ByteCode;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding[4];
#endif

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      const xiiExpressionByteCode& byteCode = nodeDesc.m_Value.Get<xiiExpressionByteCode>();

      xiiUInt32 uiDataSize = static_cast<xiiUInt32>(byteCode.GetDataBlob().GetCount());
      inout_stream << uiDataSize;

      XII_SUCCEED_OR_RETURN(byteCode.Save(inout_stream));

      out_uiSize      = sizeof(NodeUserData_Expression) + uiDataSize;
      out_uiAlignment = GetUserDataAlignment<NodeUserData_Expression>();
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_Expression>(inout_pAdditionalData);

      xiiUInt32 uiDataSize = 0;
      inout_stream >> uiDataSize;

      auto externalMemory = xiiMakeArrayPtr(inout_pAdditionalData, uiDataSize);
      inout_pAdditionalData += uiDataSize;

      XII_SUCCEED_OR_RETURN(userData.m_ByteCode.Load(inout_stream, externalMemory));

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      // Nothing to add here
    }
  };

  static_assert(sizeof(NodeUserData_Expression) == 64);
  static_assert(GetUserDataAlignment<NodeUserData_Expression>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_StartCoroutine : public NodeUserData_Type
  {
    xiiEnum<xiiScriptCoroutineCreationMode> m_CreationMode;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding;
#endif

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      XII_SUCCEED_OR_RETURN(NodeUserData_Type::Serialize(nodeDesc, inout_stream, out_uiSize, out_uiAlignment));

      xiiEnum<xiiScriptCoroutineCreationMode> creationMode = static_cast<xiiScriptCoroutineCreationMode::Enum>(nodeDesc.m_Value.Get<xiiInt64>());
      inout_stream << creationMode;

      out_uiSize      = sizeof(NodeUserData_StartCoroutine);
      out_uiAlignment = GetUserDataAlignment<NodeUserData_StartCoroutine>();
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      auto& userData = ref_node.InitUserData<NodeUserData_StartCoroutine>(inout_pAdditionalData);
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      inout_stream >> userData.m_CreationMode;

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      xiiStringBuilder sCreationMode;
      xiiReflectionUtils::EnumerationToString(xiiGetStaticRTTI<xiiScriptCoroutineCreationMode>(), nodeDesc.m_Value.Get<xiiInt64>(), sCreationMode, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(" ", sCreationMode);
    }
  };

  static_assert(sizeof(NodeUserData_StartCoroutine) == 16);
  static_assert(GetUserDataAlignment<NodeUserData_StartCoroutine>() == 8);

  //////////////////////////////////////////////////////////////////////////

  struct UserDataContext
  {
    SerializeFunction   m_SerializeFunc   = nullptr;
    DeserializeFunction m_DeserializeFunc = nullptr;
    ToStringFunction    m_ToStringFunc    = nullptr;
  };

  inline UserDataContext s_TypeToUserDataContexts[] = {
    {}, // Invalid,
    {}, // EntryCall,
    {}, // EntryCall_Coroutine,
    {&NodeUserData_TypeAndProperties::Serialize,
     &NodeUserData_TypeAndProperties::Deserialize,
     &NodeUserData_TypeAndProperties::ToString}, // MessageHandler,
    {&NodeUserData_TypeAndProperties::Serialize,
     &NodeUserData_TypeAndProperties::Deserialize,
     &NodeUserData_TypeAndProperties::ToString}, // MessageHandler_Coroutine,
    {&NodeUserData_TypeAndFunction::Serialize,
     &NodeUserData_TypeAndFunction::Deserialize,
     &NodeUserData_TypeAndFunction::ToString}, // ReflectedFunction,
    {&NodeUserData_TypeAndProperty::Serialize,
     &NodeUserData_TypeAndProperty::Deserialize,
     &NodeUserData_TypeAndProperty::ToString}, // GetReflectedProperty,
    {&NodeUserData_TypeAndProperty::Serialize,
     &NodeUserData_TypeAndProperty::Deserialize,
     &NodeUserData_TypeAndProperty::ToString}, // SetReflectedProperty,
    {&NodeUserData_TypeAndFunction::Serialize,
     &NodeUserData_TypeAndFunction::Deserialize,
     &NodeUserData_TypeAndFunction::ToString}, // InplaceCoroutine,
    {},                                        // GetScriptOwner,
    {&NodeUserData_TypeAndProperties::Serialize,
     &NodeUserData_TypeAndProperties::Deserialize,
     &NodeUserData_TypeAndProperties::ToString}, // SendMessage,

    {}, // FirstBuiltin,

    {}, // Builtin_Constant,
    {}, // Builtin_GetVariable,
    {}, // Builtin_SetVariable,
    {}, // Builtin_IncVariable,
    {}, // Builtin_DecVariable,
    {}, // Builtin_TempVariable,

    {}, // Builtin_Branch,
    {&NodeUserData_Switch::Serialize,
     &NodeUserData_Switch::Deserialize,
     &NodeUserData_Switch::ToString}, // Builtin_Switch,
    {},                               // Builtin_WhileLoop,
    {},                               // Builtin_ForLoop,
    {},                               // Builtin_ForEachLoop,
    {},                               // Builtin_ReverseForEachLoop,
    {},                               // Builtin_Break,
    {},                               // Builtin_Jump,

    {}, // Builtin_And,
    {}, // Builtin_Or,
    {}, // Builtin_Not,
    {&NodeUserData_Comparison::Serialize,
     &NodeUserData_Comparison::Deserialize,
     &NodeUserData_Comparison::ToString}, // Builtin_Compare,
    {},                                   // Builtin_CompareExec,
    {},                                   // Builtin_IsValid,
    {},                                   // Builtin_Select,

    {}, // Builtin_Add,
    {}, // Builtin_Subtract,
    {}, // Builtin_Multiply,
    {}, // Builtin_Divide,
    {&NodeUserData_Expression::Serialize,
     &NodeUserData_Expression::Deserialize,
     &NodeUserData_Expression::ToString}, // Builtin_Expression,

    {}, // Builtin_ToBool,
    {}, // Builtin_ToByte,
    {}, // Builtin_ToInt,
    {}, // Builtin_ToInt64,
    {}, // Builtin_ToFloat,
    {}, // Builtin_ToDouble,
    {}, // Builtin_ToString,
    {}, // Builtin_String_Format,
    {}, // Builtin_ToHashedString,
    {}, // Builtin_ToVariant,
    {}, // Builtin_Variant_ConvertTo,

    {}, // Builtin_MakeArray
    {}, // Builtin_Array_GetElement,
    {}, // Builtin_Array_SetElement,
    {}, // Builtin_Array_GetCount,
    {}, // Builtin_Array_IsEmpty,
    {}, // Builtin_Array_Clear,
    {}, // Builtin_Array_Contains,
    {}, // Builtin_Array_IndexOf,
    {}, // Builtin_Array_Insert,
    {}, // Builtin_Array_PushBack,
    {}, // Builtin_Array_PushBackRange,
    {}, // Builtin_Array_Remove,
    {}, // Builtin_Array_RemoveAt,

    {&NodeUserData_Type::Serialize,
     &NodeUserData_Type::Deserialize,
     &NodeUserData_Type::ToString}, // Builtin_TryGetComponentOfBaseType

    {&NodeUserData_StartCoroutine::Serialize,
     &NodeUserData_StartCoroutine::Deserialize,
     &NodeUserData_StartCoroutine::ToString}, // Builtin_StartCoroutine,
    {},                                       // Builtin_StopCoroutine,
    {},                                       // Builtin_StopAllCoroutines,
    {},                                       // Builtin_WaitForAll,
    {},                                       // Builtin_WaitForAny,
    {},                                       // Builtin_Yield,

    {}, // LastBuiltin,
  };

  static_assert(XII_ARRAY_SIZE(s_TypeToUserDataContexts) == xiiVisualScriptNodeDescription::Type::Count);
} // namespace

const UserDataContext& GetUserDataContext(xiiVisualScriptNodeDescription::Type::Enum nodeType)
{
  XII_ASSERT_DEBUG(nodeType >= 0 && static_cast<xiiUInt32>(nodeType) < XII_ARRAY_SIZE(s_TypeToUserDataContexts), "Out of bounds access");
  return s_TypeToUserDataContexts[nodeType];
}
