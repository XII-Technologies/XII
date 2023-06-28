#pragma once

#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecuteFunctionGetter = xiiVisualScriptGraphDescription::ExecuteFunction (*)(xiiVisualScriptDataType::Enum dataType);

#define MAKE_EXEC_FUNC_GETTER(funcName)                                                                                  \
  xiiVisualScriptGraphDescription::ExecuteFunction XII_CONCAT(funcName, _Getter)(xiiVisualScriptDataType::Enum dataType) \
  {                                                                                                                      \
    static xiiVisualScriptGraphDescription::ExecuteFunction functionTable[] = {                                          \
      nullptr, /* Invalid*/                                                                                              \
      &funcName<bool>,                                                                                                   \
      &funcName<xiiUInt8>,                                                                                               \
      &funcName<xiiInt32>,                                                                                               \
      &funcName<xiiInt64>,                                                                                               \
      &funcName<float>,                                                                                                  \
      &funcName<double>,                                                                                                 \
      &funcName<xiiColor>,                                                                                               \
      &funcName<xiiVec3>,                                                                                                \
      &funcName<xiiQuat>,                                                                                                \
      &funcName<xiiTransform>,                                                                                           \
      &funcName<xiiTime>,                                                                                                \
      &funcName<xiiAngle>,                                                                                               \
      &funcName<xiiString>,                                                                                              \
      &funcName<xiiGameObjectHandle>,                                                                                    \
      &funcName<xiiComponentHandle>,                                                                                     \
      &funcName<xiiTypedPointer>,                                                                                        \
      &funcName<xiiVariant>,                                                                                             \
      &funcName<xiiVariantArray>,                                                                                        \
      &funcName<xiiVariantDictionary>,                                                                                   \
    };                                                                                                                   \
                                                                                                                         \
    static_assert(XII_ARRAY_SIZE(functionTable) == xiiVisualScriptDataType::Count);                                      \
    if (dataType >= 0 && dataType < XII_ARRAY_SIZE(functionTable))                                                       \
      return functionTable[dataType];                                                                                    \
                                                                                                                         \
    xiiLog::Error("Invalid data type for deducted type {}. Script needs re-transform.", dataType);                       \
    return nullptr;                                                                                                      \
  }

template <typename T>
const char* GetTypeName()
{
  if constexpr (std::is_same<T, xiiTypedPointer>::value)
  {
    return "xiiTypePointer";
  }
  else
  {
    return xiiGetStaticRTTI<T>()->GetTypeName();
  }
}

namespace
{
  static int NodeFunction_ReflectedFunction(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    XII_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == xiiPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
    auto pFunction = static_cast<const xiiAbstractFunctionProperty*>(userData.m_pProperty);

    xiiTypedPointer pInstance;
    xiiUInt32       uiSlot = 0;

    if (pFunction->GetFunctionType() == xiiFunctionType::Member)
    {
      pInstance = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      if (pInstance.m_pObject == nullptr)
      {
        xiiLog::Error("Visual script function call '{}': Target object is invalid (nullptr)", pFunction->GetPropertyName());
        return xiiVisualScriptGraphDescription::ReturnValue::Error;
      }

      if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
      {
        xiiLog::Error("Visual script function call '{}': Target object is not of expected type '{}'", pFunction->GetPropertyName(), userData.m_pType->GetTypeName());
        return xiiVisualScriptGraphDescription::ReturnValue::Error;
      }

      ++uiSlot;
    }

    xiiHybridArray<xiiVariant, 8> args;
    xiiUInt32                     uiArgCount = pFunction->GetArgumentCount();
    if (uiArgCount != node.m_NumInputDataOffsets - uiSlot)
    {
      xiiLog::Error("Visual script function call '{}': Argument count mismatch. Script needs re-transform.", pFunction->GetPropertyName());
      return xiiVisualScriptGraphDescription::ReturnValue::Error;
    }

    for (xiiUInt32 uiArgIndex = 0; uiArgIndex < uiArgCount; ++uiArgIndex)
    {
      const xiiRTTI*       pArgType     = pFunction->GetArgumentType(uiArgIndex);
      xiiVariantType::Enum expectedType = pArgType->GetVariantType();
      args.PushBack(ref_instance.GetDataAsVariant(node.GetInputDataOffset(uiSlot), expectedType));

      ++uiSlot;
    }

    xiiVariant returnValue;
    pFunction->Execute(pInstance.m_pObject, args, returnValue);

    uiSlot = 0;
    if (returnValue.IsValid())
    {
      ref_instance.SetDataFromVariant(node.GetOutputDataOffset(0), returnValue);
    }

    return 0;
  }

  static int NodeFunction_GetScriptOwner(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiWorld* pWorld = ref_instance.GetWorld();
    ref_instance.SetPointerData(node.GetOutputDataOffset(0), pWorld, xiiGetStaticRTTI<xiiWorld>());

    xiiReflectedClass& owner = ref_instance.GetOwner();
    if (auto pComponent = xiiDynamicCast<xiiComponent*>(&owner))
    {
      ref_instance.SetPointerData(node.GetOutputDataOffset(1), pComponent->GetOwner());
      ref_instance.SetPointerData(node.GetOutputDataOffset(2), pComponent);
    }
    else
    {
      ref_instance.SetPointerData(node.GetOutputDataOffset(1), &owner, owner.GetDynamicRTTI());
    }

    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  static int NodeFunction_Builtin_Branch(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    return bCondition ? 0 : 1;
  }

  static int NodeFunction_Builtin_And(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool a = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    bool b = ref_instance.GetData<bool>(node.GetInputDataOffset(1));
    ref_instance.SetData(node.GetOutputDataOffset(0), a && b);
    return 0;
  }

  static int NodeFunction_Builtin_Or(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool a = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    bool b = ref_instance.GetData<bool>(node.GetInputDataOffset(1));
    ref_instance.SetData(node.GetOutputDataOffset(0), a || b);
    return 0;
  }

  static int NodeFunction_Builtin_Not(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool a = ref_instance.GetData<bool>(node.GetInputDataOffset(0));
    ref_instance.SetData(node.GetOutputDataOffset(0), !a);
    return 0;
  }

  template <typename T>
  static int NodeFunction_Builtin_Compare(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Comparison>();
    bool  bRes     = false;

    if constexpr (std::is_same<T, bool>::value ||
                  std::is_same<T, xiiUInt8>::value ||
                  std::is_same<T, xiiInt32>::value ||
                  std::is_same<T, xiiInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, xiiColor>::value ||
                  std::is_same<T, xiiVec3>::value ||
                  std::is_same<T, xiiTime>::value ||
                  std::is_same<T, xiiAngle>::value ||
                  std::is_same<T, xiiString>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      bRes       = xiiComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same<T, xiiGameObjectHandle>::value ||
                       std::is_same<T, xiiComponentHandle>::value ||
                       std::is_same<T, xiiTypedPointer>::value)
    {
      xiiTypedPointer a = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      xiiTypedPointer b = ref_instance.GetPointerData(node.GetInputDataOffset(1));
      bRes              = xiiComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same<T, xiiQuat>::value ||
                       std::is_same<T, xiiTransform>::value ||
                       std::is_same<T, xiiVariant>::value ||
                       std::is_same<T, xiiVariantArray>::value ||
                       std::is_same<T, xiiVariantDictionary>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));

      if (userData.m_ComparisonOperator == xiiComparisonOperator::Equal)
      {
        bRes = a == b;
      }
      else if (userData.m_ComparisonOperator == xiiComparisonOperator::NotEqual)
      {
        bRes = a != b;
      }
      else
      {
        xiiStringBuilder sCompOp;
        xiiReflectionUtils::EnumerationToString(userData.m_ComparisonOperator, sCompOp, xiiReflectionUtils::EnumConversionMode::ValueNameOnly);

        xiiLog::Error("Comparison '{}' is not defined for type '{}'", sCompOp, GetTypeName<T>());
      }
    }
    else
    {
      xiiLog::Error("Comparison is not defined for type '{}'", GetTypeName<T>());
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), bRes);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Compare);

  template <typename T>
  static int NodeFunction_Builtin_IsValid(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bIsValid = true;
    if constexpr (std::is_same<T, float>::value)
    {
      bIsValid = xiiMath::IsFinite(ref_instance.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same<T, double>::value)
    {
      bIsValid = xiiMath::IsFinite(ref_instance.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same<T, xiiColor>::value)
    {
      bIsValid = ref_instance.GetData<xiiColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, xiiVec3>::value)
    {
      bIsValid = ref_instance.GetData<xiiVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, xiiQuat>::value)
    {
      bIsValid = ref_instance.GetData<xiiQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same<T, xiiString>::value || std::is_same<T, xiiStringView>::value)
    {
      bIsValid = ref_instance.GetData<xiiString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same<T, xiiGameObjectHandle>::value ||
                       std::is_same<T, xiiComponentHandle>::value ||
                       std::is_same<T, xiiTypedPointer>::value)
    {
      bIsValid = ref_instance.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same<T, xiiVariant>::value)
    {
      bIsValid = ref_instance.GetData<xiiVariant>(dataOffset).IsValid();
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), bIsValid);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static int NodeFunction_Builtin_Add(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, xiiUInt8>::value ||
                  std::is_same<T, xiiInt32>::value ||
                  std::is_same<T, xiiInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, xiiColor>::value ||
                  std::is_same<T, xiiVec3>::value ||
                  std::is_same<T, xiiTime>::value ||
                  std::is_same<T, xiiAngle>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else
    {
      xiiLog::Error("Add is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Add);

  template <typename T>
  static int NodeFunction_Builtin_Sub(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, xiiUInt8>::value ||
                  std::is_same<T, xiiInt32>::value ||
                  std::is_same<T, xiiInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, xiiColor>::value ||
                  std::is_same<T, xiiVec3>::value ||
                  std::is_same<T, xiiTime>::value ||
                  std::is_same<T, xiiAngle>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else
    {
      xiiLog::Error("Subtract is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Sub);

  template <typename T>
  static int NodeFunction_Builtin_Mul(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, xiiUInt8>::value ||
                  std::is_same<T, xiiInt32>::value ||
                  std::is_same<T, xiiInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, xiiColor>::value ||
                  std::is_same<T, xiiTime>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same<T, xiiVec3>::value)
    {
      const xiiVec3& a = ref_instance.GetData<xiiVec3>(node.GetInputDataOffset(0));
      const xiiVec3& b = ref_instance.GetData<xiiVec3>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same<T, xiiAngle>::value)
    {
      const xiiAngle& a = ref_instance.GetData<xiiAngle>(node.GetInputDataOffset(0));
      const xiiAngle& b = ref_instance.GetData<xiiAngle>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), xiiAngle(a * b.GetRadian()));
    }
    else
    {
      xiiLog::Error("Multiply is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mul);

  template <typename T>
  static int NodeFunction_Builtin_Div(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same<T, xiiUInt8>::value ||
                  std::is_same<T, xiiInt32>::value ||
                  std::is_same<T, xiiInt64>::value ||
                  std::is_same<T, float>::value ||
                  std::is_same<T, double>::value ||
                  std::is_same<T, xiiTime>::value)
    {
      const T& a = ref_instance.GetData<T>(node.GetInputDataOffset(0));
      const T& b = ref_instance.GetData<T>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same<T, xiiVec3>::value)
    {
      const xiiVec3& a = ref_instance.GetData<xiiVec3>(node.GetInputDataOffset(0));
      const xiiVec3& b = ref_instance.GetData<xiiVec3>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same<T, xiiAngle>::value)
    {
      const xiiAngle& a = ref_instance.GetData<xiiAngle>(node.GetInputDataOffset(0));
      const xiiAngle& b = ref_instance.GetData<xiiAngle>(node.GetInputDataOffset(1));
      ref_instance.SetData(node.GetOutputDataOffset(0), xiiAngle(a / b.GetRadian()));
    }
    else
    {
      xiiLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static int NodeFunction_Builtin_ToBool(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same<T, bool>::value)
    {
      bRes = ref_instance.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same<T, xiiUInt8>::value ||
                       std::is_same<T, xiiInt32>::value ||
                       std::is_same<T, xiiInt64>::value ||
                       std::is_same<T, float>::value ||
                       std::is_same<T, double>::value)
    {
      bRes = ref_instance.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same<T, xiiGameObjectHandle>::value ||
                       std::is_same<T, xiiComponentHandle>::value ||
                       std::is_same<T, xiiTypedPointer>::value)
    {
      bRes = ref_instance.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else
    {
      xiiLog::Error("ToBool is not defined for type '{}'", GetTypeName<T>());
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), bRes);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToBool);

  template <typename NumberType, typename T>
  XII_FORCE_INLINE static int NodeFunction_Builtin_ToNumber(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node, const char* szName)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    NumberType res = 0;
    if constexpr (std::is_same<T, bool>::value)
    {
      res = ref_instance.GetData<T>(dataOffset) ? 1 : 0;
    }
    else if constexpr (std::is_same<T, xiiUInt8>::value ||
                       std::is_same<T, xiiInt32>::value ||
                       std::is_same<T, xiiInt64>::value ||
                       std::is_same<T, float>::value ||
                       std::is_same<T, double>::value)
    {
      res = static_cast<NumberType>(ref_instance.GetData<T>(dataOffset));
    }
    else
    {
      xiiLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    ref_instance.SetData(node.GetOutputDataOffset(0), res);
    return 0;
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                                 \
  template <typename T>                                                                                                                           \
  static int XII_CONCAT(NodeFunction_Builtin_To, Name)(xiiVisualScriptInstance & ref_instance, const xiiVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                               \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(ref_instance, node, #Name);                                                               \
  }

  MAKE_TONUMBER_EXEC_FUNC(xiiUInt8, Byte);
  MAKE_TONUMBER_EXEC_FUNC(xiiInt32, Int);
  MAKE_TONUMBER_EXEC_FUNC(xiiInt64, Int64);
  MAKE_TONUMBER_EXEC_FUNC(float, Float);
  MAKE_TONUMBER_EXEC_FUNC(double, Double);

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToByte);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToInt64);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToFloat);
  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToDouble);

  template <typename T>
  static int NodeFunction_Builtin_ToString(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiStringBuilder s;
    if constexpr (std::is_same<T, xiiGameObjectHandle>::value ||
                  std::is_same<T, xiiComponentHandle>::value ||
                  std::is_same<T, xiiTypedPointer>::value)
    {
      xiiTypedPointer p = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      s.Format("{} {}", p.m_pType->GetTypeName(), xiiArgP(p.m_pObject));
    }
    else
    {
      xiiConversionUtils::ToString(ref_instance.GetData<T>(node.GetInputDataOffset(0)), s);
    }
    ref_instance.SetData(node.GetOutputDataOffset(0), xiiString(s));
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  template <typename T>
  static int NodeFunction_Builtin_ToVariant(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiVariant v;
    if constexpr (std::is_same<T, xiiTypedPointer>::value)
    {
      xiiTypedPointer p = ref_instance.GetPointerData(node.GetInputDataOffset(0));
      v                 = xiiVariant(p.m_pObject, p.m_pType);
    }
    else
    {
      v = ref_instance.GetData<T>(node.GetInputDataOffset(0));
    }
    ref_instance.SetData(node.GetOutputDataOffset(0), v);
    return 0;
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToVariant);

  template <typename T>
  static int NodeFunction_Builtin_Variant_ConvertTo(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    const xiiVariant& v = ref_instance.GetData<xiiVariant>(node.GetInputDataOffset(0));
    if constexpr (std::is_same<T, xiiTypedPointer>::value)
    {
      if (v.IsA<xiiTypedPointer>())
      {
        xiiTypedPointer typedPtr = v.Get<xiiTypedPointer>();
        ref_instance.SetPointerData(node.GetOutputDataOffset(0), typedPtr.m_pObject, typedPtr.m_pType);
        return 0;
      }

      ref_instance.SetPointerData<void*>(node.GetOutputDataOffset(0), nullptr, nullptr);
      return 1;
    }
    else if constexpr (std::is_same<T, xiiVariant>::value)
    {
      ref_instance.SetData(node.GetOutputDataOffset(0), v);
      return 0;
    }
    else
    {
      xiiResult conversionResult = XII_SUCCESS;
      ref_instance.SetData(node.GetOutputDataOffset(0), v.ConvertTo<T>(&conversionResult));
      return conversionResult.Succeeded() ? 0 : 1;
    }
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Variant_ConvertTo);

  //////////////////////////////////////////////////////////////////////////

  static int NodeFunction_Builtin_MakeArray(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiVariantArray& a = ref_instance.GetWritableData<xiiVariantArray>(node.GetOutputDataOffset(0));
    a.Clear();
    a.Reserve(node.m_NumInputDataOffsets);

    for (xiiUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      if (dataOffset.m_uiIsConstant)
      {
        auto expectedType = xiiVisualScriptDataType::GetVariantType(static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType));
        a.PushBack(ref_instance.GetDataAsVariant(dataOffset, expectedType));
      }
      else
      {
        a.PushBack(ref_instance.GetData<xiiVariant>(dataOffset));
      }
    }

    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  static int NodeFunction_Builtin_TryGetComponentOfBaseType(xiiVisualScriptInstance& ref_instance, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    xiiTypedPointer p = ref_instance.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != xiiGetStaticRTTI<xiiGameObject>())
    {
      xiiLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'xiiGameObject'");
      return 0;
    }

    xiiComponent* pComponent = nullptr;
    static_cast<xiiGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent);
    ref_instance.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return 0;
  }

  //////////////////////////////////////////////////////////////////////////

  struct ExecuteFunctionContext
  {
    xiiVisualScriptGraphDescription::ExecuteFunction m_Func       = nullptr;
    ExecuteFunctionGetter                            m_FuncGetter = nullptr;
  };

  static ExecuteFunctionContext s_TypeToExecuteFunctions[] = {
    {},                                // Invalid,
    {},                                // EntryCall,
    {},                                // MessageHandler,
    {&NodeFunction_ReflectedFunction}, // ReflectedFunction,
    {&NodeFunction_GetScriptOwner},    // GetOwner,

    {}, // FirstBuiltin,

    {&NodeFunction_Builtin_Branch},                  // Builtin_Branch,
    {&NodeFunction_Builtin_And},                     // Builtin_And,
    {&NodeFunction_Builtin_Or},                      // Builtin_Or,
    {&NodeFunction_Builtin_Not},                     // Builtin_Not,
    {nullptr, &NodeFunction_Builtin_Compare_Getter}, // Builtin_Compare,
    {nullptr, &NodeFunction_Builtin_IsValid_Getter}, // Builtin_IsValid,

    {nullptr, &NodeFunction_Builtin_Add_Getter}, // Builtin_Add,
    {nullptr, &NodeFunction_Builtin_Sub_Getter}, // Builtin_Subtract,
    {nullptr, &NodeFunction_Builtin_Mul_Getter}, // Builtin_Multiply,
    {nullptr, &NodeFunction_Builtin_Div_Getter}, // Builtin_Divide,

    {nullptr, &NodeFunction_Builtin_ToBool_Getter},            // Builtin_ToBool,
    {nullptr, &NodeFunction_Builtin_ToByte_Getter},            // Builtin_ToByte,
    {nullptr, &NodeFunction_Builtin_ToInt_Getter},             // Builtin_ToInt,
    {nullptr, &NodeFunction_Builtin_ToInt64_Getter},           // Builtin_ToInt64,
    {nullptr, &NodeFunction_Builtin_ToFloat_Getter},           // Builtin_ToFloat,
    {nullptr, &NodeFunction_Builtin_ToDouble_Getter},          // Builtin_ToDouble,
    {nullptr, &NodeFunction_Builtin_ToString_Getter},          // Builtin_ToString,
    {nullptr, &NodeFunction_Builtin_ToVariant_Getter},         // Builtin_ToVariant,
    {nullptr, &NodeFunction_Builtin_Variant_ConvertTo_Getter}, // Builtin_Variant_ConvertTo,

    {&NodeFunction_Builtin_MakeArray}, // Builtin_MakeArray

    {&NodeFunction_Builtin_TryGetComponentOfBaseType}, // Builtin_TryGetComponentOfBaseType

    {}, // LastBuiltin,
  };

  static_assert(XII_ARRAY_SIZE(s_TypeToExecuteFunctions) == xiiVisualScriptNodeDescription::Type::Count);
} // namespace

xiiVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(xiiVisualScriptNodeDescription::Type::Enum nodeType, xiiVisualScriptDataType::Enum dataType)
{
  XII_ASSERT_DEBUG(nodeType >= 0 && nodeType < XII_ARRAY_SIZE(s_TypeToExecuteFunctions), "Out of bounds access");
  auto& context = s_TypeToExecuteFunctions[nodeType];
  if (context.m_Func != nullptr)
  {
    return context.m_Func;
  }

  if (context.m_FuncGetter != nullptr)
  {
    return context.m_FuncGetter(dataType);
  }

  return nullptr;
}

#undef MAKE_EXEC_FUNC_GETTER
#undef MAKE_TONUMBER_EXEC_FUNC
