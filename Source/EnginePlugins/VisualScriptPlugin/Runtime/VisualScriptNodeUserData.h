#pragma once

using SerializeFunction   = xiiResult (*)(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_Size, xiiUInt32& out_alignment);
using DeserializeFunction = xiiResult (*)(xiiVisualScriptGraphDescription::Node& node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData);
using ToStringFunction    = void (*)(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult);

namespace
{
  struct NodeUserData_Type
  {
    const xiiRTTI* m_pType = nullptr;

#if XII_ENABLED(XII_PLATFORM_32BIT)
    xiiUInt32 m_uiPadding;
#endif

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      inout_stream << nodeDesc.m_UserData.m_pTargetType->GetTypeName();

      out_uiSize      = sizeof(NodeUserData_Type);
      out_uiAlignment = XII_ALIGNMENT_OF(NodeUserData_Type);
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
      NodeUserData_Type userData;
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));
      ref_node.SetUserData(userData, inout_pAdditionalData);
      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      if (nodeDesc.m_UserData.m_pTargetType != nullptr)
      {
        out_sResult.Append(nodeDesc.m_UserData.m_pTargetType->GetTypeName());
      }
    }
  };

  static_assert(sizeof(NodeUserData_Type) == 8);

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

      inout_stream << nodeDesc.m_UserData.m_pTargetProperty->GetPropertyName();

      out_uiSize      = sizeof(NodeUserData_TypeAndProperty);
      out_uiAlignment = XII_ALIGNMENT_OF(NodeUserData_TypeAndProperty);
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
        xiiLog::Error("{} '{}' not found on type '{}'",
                      std::is_same<T, xiiAbstractFunctionProperty>::value ? "Function" : "Property",
                      sPropName, pType->GetTypeName());
        return XII_FAILURE;
      }

      return XII_SUCCESS;
    }

    template <bool PropIsFunction>
    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      NodeUserData_TypeAndProperty userData;
      XII_SUCCEED_OR_RETURN(ReadType(inout_stream, userData.m_pType));

      if constexpr (PropIsFunction)
      {
        XII_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetFunctions(), userData.m_pProperty));
      }
      else
      {
        XII_SUCCEED_OR_RETURN(ReadProperty(inout_stream, userData.m_pType, userData.m_pType->GetProperties(), userData.m_pProperty));
      }

      ref_node.SetUserData(userData, inout_pAdditionalData);
      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      NodeUserData_Type::ToString(nodeDesc, out_sResult);

      if (nodeDesc.m_UserData.m_pTargetProperty != nullptr)
      {
        out_sResult.Append(".", nodeDesc.m_UserData.m_pTargetProperty->GetPropertyName());
      }
    }
  };

  static_assert(sizeof(NodeUserData_TypeAndProperty) == 16);

  //////////////////////////////////////////////////////////////////////////

  struct NodeUserData_Comparison
  {
    xiiEnum<xiiComparisonOperator> m_ComparisonOperator;

    static xiiResult Serialize(const xiiVisualScriptNodeDescription& nodeDesc, xiiStreamWriter& inout_stream, xiiUInt32& out_uiSize, xiiUInt32& out_uiAlignment)
    {
      xiiEnum<xiiComparisonOperator> compOp = nodeDesc.m_UserData.m_ComparisonOperator;
      inout_stream << compOp;

      out_uiSize      = sizeof(NodeUserData_Comparison);
      out_uiAlignment = XII_ALIGNMENT_OF(NodeUserData_Comparison);
      return XII_SUCCESS;
    }

    static xiiResult Deserialize(xiiVisualScriptGraphDescription::Node& ref_node, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
    {
      NodeUserData_Comparison userData;
      inout_stream >> userData.m_ComparisonOperator;
      ref_node.SetUserData(userData, inout_pAdditionalData);

      return XII_SUCCESS;
    }

    static void ToString(const xiiVisualScriptNodeDescription& nodeDesc, xiiStringBuilder& out_sResult)
    {
      xiiStringBuilder sCompOp;
      xiiReflectionUtils::EnumerationToString<xiiComparisonOperator>(nodeDesc.m_UserData.m_ComparisonOperator, sCompOp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);

      out_sResult.Append(sCompOp);
    }
  };

  //////////////////////////////////////////////////////////////////////////

  struct UserDataContext
  {
    SerializeFunction   m_SerializeFunc   = nullptr;
    DeserializeFunction m_DeserializeFunc = nullptr;
    ToStringFunction    m_ToStringFunc    = nullptr;
  };

  static UserDataContext s_TypeToUserDataContexts[] = {
    {}, // Invalid,
    {}, // EntryCall,
    {}, // MessageHandler,
    {&NodeUserData_TypeAndProperty::Serialize,
     &NodeUserData_TypeAndProperty::Deserialize<true>,
     &NodeUserData_TypeAndProperty::ToString}, // ReflectedFunction,
    {},                                        // GetOwner,

    {}, // FirstBuiltin,

    {}, // Builtin_Branch,
    {}, // Builtin_And,
    {}, // Builtin_Or,
    {}, // Builtin_Not,
    {&NodeUserData_Comparison::Serialize,
     &NodeUserData_Comparison::Deserialize,
     &NodeUserData_Comparison::ToString}, // Builtin_Compare,
    {},                                   // Builtin_IsValid,

    {}, // Builtin_Add,
    {}, // Builtin_Subtract,
    {}, // Builtin_Multiply,
    {}, // Builtin_Divide,

    {}, // Builtin_ToBool,
    {}, // Builtin_ToByte,
    {}, // Builtin_ToInt,
    {}, // Builtin_ToInt64,
    {}, // Builtin_ToFloat,
    {}, // Builtin_ToDouble,
    {}, // Builtin_ToString,
    {}, // Builtin_ToVariant,
    {}, // Builtin_Variant_ConvertTo,

    {}, // Builtin_MakeArray

    {&NodeUserData_Type::Serialize,
     &NodeUserData_Type::Deserialize,
     &NodeUserData_Type::ToString}, // Builtin_TryGetComponentOfBaseType

    {}, // LastBuiltin,
  };

  static_assert(XII_ARRAY_SIZE(s_TypeToUserDataContexts) == xiiVisualScriptNodeDescription::Type::Count);
} // namespace

const UserDataContext& GetUserDataContext(xiiVisualScriptNodeDescription::Type::Enum nodeType)
{
  XII_ASSERT_DEBUG(nodeType >= 0 && nodeType < XII_ARRAY_SIZE(s_TypeToUserDataContexts), "Out of bounds access");
  return s_TypeToUserDataContexts[nodeType];
}
