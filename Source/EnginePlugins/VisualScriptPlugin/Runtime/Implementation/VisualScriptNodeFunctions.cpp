#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

using ExecResult            = xiiVisualScriptGraphDescription::ExecResult;
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
      &funcName<xiiHashedString>,                                                                                        \
      &funcName<xiiGameObjectHandle>,                                                                                    \
      &funcName<xiiComponentHandle>,                                                                                     \
      &funcName<xiiTypedPointer>,                                                                                        \
      &funcName<xiiVariant>,                                                                                             \
      &funcName<xiiVariantArray>,                                                                                        \
      &funcName<xiiVariantDictionary>,                                                                                   \
      &funcName<xiiScriptCoroutineHandle>,                                                                               \
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
xiiStringView GetTypeName()
{
  if constexpr (std::is_same_v<T, xiiTypedPointer>)
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
  static XII_FORCE_INLINE xiiResult FillFunctionArgs(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node, const xiiAbstractFunctionProperty* pFunction, xiiUInt32 uiStartSlot, xiiDynamicArray<xiiVariant>& out_args)
  {
    const xiiUInt32 uiArgCount = pFunction->GetArgumentCount();
    if (uiArgCount != node.m_NumInputDataOffsets - uiStartSlot)
    {
      xiiLog::Error("Visual script {} '{}': Argument count mismatch. Script needs re-transform.", xiiVisualScriptNodeDescription::Type::GetName(node.m_Type), pFunction->GetPropertyName());
      return XII_FAILURE;
    }

    for (xiiUInt32 i = 0; i < uiArgCount; ++i)
    {
      const xiiRTTI* pArgType = pFunction->GetArgumentType(i);
      out_args.PushBack(inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pArgType));
    }

    return XII_SUCCESS;
  }

  static XII_FORCE_INLINE xiiScriptWorldModule* GetScriptModule(xiiVisualScriptExecutionContext& inout_context)
  {
    xiiWorld* pWorld = inout_context.GetInstance().GetWorld();
    if (pWorld == nullptr)
    {
      xiiLog::Error("Visual script coroutines need a script instance with a valid xiiWorld");
      return nullptr;
    }

    return pWorld->GetOrCreateModule<xiiScriptWorldModule>();
  }

  static ExecResult NodeFunction_ReflectedFunction(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
    XII_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == xiiPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
    auto pFunction = static_cast<const xiiAbstractFunctionProperty*>(userData.m_pProperty);

    xiiTypedPointer pInstance;
    xiiUInt32       uiSlot = 0;

    if (pFunction->GetFunctionType() == xiiFunctionType::Member)
    {
      pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));
      if (pInstance.m_pObject == nullptr)
      {
        xiiLog::Error("Visual script function call '{}': Target object is invalid (nullptr)", pFunction->GetPropertyName());
        return ExecResult::Error();
      }

      if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
      {
        xiiLog::Error("Visual script function call '{}': Target object is not of expected type '{}'", pFunction->GetPropertyName(), userData.m_pType->GetTypeName());
        return ExecResult::Error();
      }

      ++uiSlot;
    }

    xiiHybridArray<xiiVariant, 8> args;
    if (FillFunctionArgs(inout_context, node, pFunction, uiSlot, args).Failed())
    {
      return ExecResult::Error();
    }

    xiiVariant returnValue;
    pFunction->Execute(pInstance.m_pObject, args, returnValue);

    auto dataOffsetR = node.GetOutputDataOffset(0);
    if (dataOffsetR.IsValid())
    {
      inout_context.SetDataFromVariant(dataOffsetR, returnValue);
    }

    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_GetReflectedProperty(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData  = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto  pProperty = userData.m_pProperty;

    xiiTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pObject == nullptr)
    {
      xiiLog::Error("Visual script get property '{}': Target object is invalid (nullptr)", pProperty->GetPropertyName());
      return ExecResult::Error();
    }

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      xiiLog::Error("Visual script get property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == xiiPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const xiiAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                    std::is_same_v<T, xiiComponentHandle> ||
                    std::is_same_v<T, xiiTypedPointer>)
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        XII_ASSERT_DEBUG(pProperty->GetSpecificType() == xiiGetStaticRTTI<T>(), "");

        T value;
        pMemberProperty->GetValuePtr(pInstance.m_pObject, &value);
        inout_context.SetData(node.GetOutputDataOffset(0), value);
      }
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_GetReflectedProperty);

  template <typename T>
  static ExecResult NodeFunction_SetReflectedProperty(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData  = node.GetUserData<NodeUserData_TypeAndProperty>();
    auto  pProperty = userData.m_pProperty;

    xiiTypedPointer pInstance;
    pInstance = inout_context.GetPointerData(node.GetInputDataOffset(0));

    if (pInstance.m_pObject == nullptr)
    {
      xiiLog::Error("Visual script set property '{}': Target object is invalid (nullptr)", pProperty->GetPropertyName());
      return ExecResult::Error();
    }

    if (pInstance.m_pType->IsDerivedFrom(userData.m_pType) == false)
    {
      xiiLog::Error("Visual script set property '{}': Target object is not of expected type '{}'", pProperty->GetPropertyName(), userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    if (pProperty->GetCategory() == xiiPropertyCategory::Member)
    {
      auto pMemberProperty = static_cast<const xiiAbstractMemberProperty*>(pProperty);

      if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                    std::is_same_v<T, xiiComponentHandle> ||
                    std::is_same_v<T, xiiTypedPointer>)
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        XII_ASSERT_DEBUG(pProperty->GetSpecificType() == xiiGetStaticRTTI<T>(), "");

        const T& value = inout_context.GetData<T>(node.GetInputDataOffset(1));
        pMemberProperty->SetValuePtr(pInstance.m_pObject, &value);
      }
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_SetReflectedProperty);

  static ExecResult NodeFunction_InplaceCoroutine(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      auto pModule = GetScriptModule(inout_context);
      if (pModule == nullptr)
        return ExecResult::Error();

      auto& userData = node.GetUserData<NodeUserData_TypeAndProperty>();
      pModule->CreateCoroutine(userData.m_pType, userData.m_pType->GetTypeName(), inout_context.GetInstance(), xiiScriptCoroutineCreationMode::AllowOverlap, pCoroutine);

      XII_ASSERT_DEBUG(userData.m_pProperty->GetCategory() == xiiPropertyCategory::Function, "Property '{}' is not a function", userData.m_pProperty->GetPropertyName());
      auto pFunction = static_cast<const xiiAbstractFunctionProperty*>(userData.m_pProperty);

      xiiHybridArray<xiiVariant, 8> args;
      if (FillFunctionArgs(inout_context, node, pFunction, 0, args).Failed())
      {
        return ExecResult::Error();
      }

      pCoroutine->Start(args);

      inout_context.SetCurrentCoroutine(pCoroutine);
    }

    auto result = pCoroutine->Update(inout_context.GetDeltaTimeSinceLastExecution());
    if (result.m_State == xiiScriptCoroutine::Result::State::Running)
    {
      return ExecResult::ContinueLater(result.m_MaxDelay);
    }

    xiiWorld* pWorld  = inout_context.GetInstance().GetWorld();
    auto      pModule = pWorld->GetOrCreateModule<xiiScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(pCoroutine->GetHandle());
    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(result.m_State == xiiScriptCoroutine::Result::State::Completed ? 0 : 1);
  }

  static ExecResult NodeFunction_GetScriptOwner(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiWorld* pWorld = inout_context.GetInstance().GetWorld();
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pWorld, xiiGetStaticRTTI<xiiWorld>());

    xiiReflectedClass& owner = inout_context.GetInstance().GetOwner();
    if (auto pComponent = xiiDynamicCast<xiiComponent*>(&owner))
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), pComponent->GetOwner());
      inout_context.SetPointerData(node.GetOutputDataOffset(2), pComponent);
    }
    else
    {
      inout_context.SetPointerData(node.GetOutputDataOffset(1), &owner, owner.GetDynamicRTTI());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_SendMessage(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData                  = node.GetUserData<NodeUserData_TypeAndProperties>();
    auto  targetObjectDataOffset    = node.GetInputDataOffset(0);
    auto  targetComponentDataOffset = node.GetInputDataOffset(1);

    auto pTargetObject    = targetObjectDataOffset.IsValid() ? static_cast<xiiGameObject*>(inout_context.GetPointerData(targetObjectDataOffset).m_pObject) : nullptr;
    auto pTargetComponent = targetComponentDataOffset.IsValid() ? static_cast<xiiComponent*>(inout_context.GetPointerData(targetComponentDataOffset).m_pObject) : nullptr;
    if (pTargetObject == nullptr && pTargetComponent == nullptr)
    {
      xiiLog::Error("Visual script send '{}': Invalid target game object and component.", userData.m_pType->GetTypeName());
      return ExecResult::Error();
    }

    auto    mode  = static_cast<xiiVisualScriptSendMessageMode::Enum>(inout_context.GetData<xiiInt64>(node.GetInputDataOffset(2)));
    xiiTime delay = inout_context.GetData<xiiTime>(node.GetInputDataOffset(3));

    xiiScriptComponent* pSenderComponent = nullptr;
    if (mode == xiiVisualScriptSendMessageMode::Event)
    {
      pSenderComponent = xiiDynamicCast<xiiScriptComponent*>(&inout_context.GetInstance().GetOwner());
    }

    const xiiUInt32 uiStartSlot = 4;

    xiiUniquePtr<xiiMessage> pMessage = userData.m_pType->GetAllocator()->Allocate<xiiMessage>();
    for (xiiUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
    {
      auto           pProp     = userData.m_Properties[i];
      const xiiRTTI* pPropType = pProp->GetSpecificType();
      xiiVariant     value     = inout_context.GetDataAsVariant(node.GetInputDataOffset(uiStartSlot + i), pPropType);

      if (pProp->GetCategory() == xiiPropertyCategory::Member)
      {
        xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), pMessage.Borrow(), value);
      }
      else
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
    }

    bool bWriteOutputs = false;
    if (pTargetComponent != nullptr)
    {
      if (delay.IsPositive())
      {
        pTargetComponent->PostMessage(*pMessage, delay);
      }
      else
      {
        bWriteOutputs = pTargetComponent->SendMessage(*pMessage);
      }
    }
    else if (pTargetObject != nullptr)
    {
      if (delay.IsPositive())
      {
        if (mode == xiiVisualScriptSendMessageMode::Direct)
          pTargetObject->PostMessage(*pMessage, delay);
        else if (mode == xiiVisualScriptSendMessageMode::Recursive)
          pTargetObject->PostMessageRecursive(*pMessage, delay);
        else
          pTargetObject->PostEventMessage(*pMessage, pSenderComponent, delay);
      }
      else
      {
        if (mode == xiiVisualScriptSendMessageMode::Direct)
          bWriteOutputs = pTargetObject->SendMessage(*pMessage);
        else if (mode == xiiVisualScriptSendMessageMode::Recursive)
          bWriteOutputs = pTargetObject->SendMessageRecursive(*pMessage);
        else
          bWriteOutputs = pTargetObject->SendEventMessage(*pMessage, pSenderComponent);
      }
    }

    if (bWriteOutputs)
    {
      for (xiiUInt32 i = 0; i < userData.m_uiNumProperties; ++i)
      {
        auto dataOffset = node.GetOutputDataOffset(i);
        if (dataOffset.IsValid() == false)
          continue;

        auto       pProp = userData.m_Properties[i];
        xiiVariant value;

        if (pProp->GetCategory() == xiiPropertyCategory::Member)
        {
          value = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), pMessage.Borrow());
        }
        else
        {
          XII_ASSERT_NOT_IMPLEMENTED;
        }

        inout_context.SetDataFromVariant(dataOffset, value);
      }
    }

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_SetVariable(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                  std::is_same_v<T, xiiComponentHandle> ||
                  std::is_same_v<T, xiiTypedPointer>)
    {
      xiiTypedPointer ptr = inout_context.GetPointerData(node.GetInputDataOffset(0));
      inout_context.SetPointerData(node.GetOutputDataOffset(0), ptr.m_pObject, ptr.m_pType);
    }
    else
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<T>(node.GetInputDataOffset(0)));
    }
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_SetVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IncVariable(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), ++a);
    }
    else
    {
      xiiLog::Error("Increment is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IncVariable);

  template <typename T>
  static ExecResult NodeFunction_Builtin_DecVariable(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double>)
    {
      T a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      inout_context.SetData(node.GetOutputDataOffset(0), --a);
    }
    else
    {
      xiiLog::Error("Decrement is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_DecVariable);

  static ExecResult NodeFunction_Builtin_Branch(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool bCondition = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    return ExecResult::RunNext(bCondition ? 0 : 1);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Switch(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiInt64 iValue = 0;
    if constexpr (std::is_same_v<T, xiiInt64>)
    {
      iValue = inout_context.GetData<xiiInt64>(node.GetInputDataOffset(0));
    }
    else if constexpr (std::is_same_v<T, xiiHashedString>)
    {
      iValue = inout_context.GetData<xiiHashedString>(node.GetInputDataOffset(0)).GetHash();
    }
    else
    {
      XII_ASSERT_NOT_IMPLEMENTED;
    }

    auto& userData = node.GetUserData<NodeUserData_Switch>();
    for (xiiUInt32 i = 0; i < userData.m_uiNumCases; ++i)
    {
      if (iValue == userData.m_Cases[i])
      {
        return ExecResult::RunNext(i);
      }
    }

    return ExecResult::RunNext(userData.m_uiNumCases);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Switch);

  static ExecResult NodeFunction_Builtin_And(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a && b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Or(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    bool b = inout_context.GetData<bool>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a || b);
    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Not(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    bool a = inout_context.GetData<bool>(node.GetInputDataOffset(0));
    inout_context.SetData(node.GetOutputDataOffset(0), !a);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_Compare(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Comparison>();
    bool  bRes     = false;

    if constexpr (std::is_same_v<T, bool> ||
                  std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, xiiColor> ||
                  std::is_same_v<T, xiiVec3> ||
                  std::is_same_v<T, xiiTime> ||
                  std::is_same_v<T, xiiAngle> ||
                  std::is_same_v<T, xiiString> ||
                  std::is_same_v<T, xiiHashedString>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      bRes       = xiiComparisonOperator::Compare(userData.m_ComparisonOperator, a, b);
    }
    else if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                       std::is_same_v<T, xiiComponentHandle> ||
                       std::is_same_v<T, xiiTypedPointer>)
    {
      xiiTypedPointer a = inout_context.GetPointerData(node.GetInputDataOffset(0));
      xiiTypedPointer b = inout_context.GetPointerData(node.GetInputDataOffset(1));
      bRes              = xiiComparisonOperator::Compare(userData.m_ComparisonOperator, a.m_pObject, b.m_pObject);
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      xiiVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      xiiVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);

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
    else if constexpr (std::is_same_v<T, xiiQuat> ||
                       std::is_same_v<T, xiiTransform> ||
                       std::is_same_v<T, xiiVariantArray> ||
                       std::is_same_v<T, xiiVariantDictionary>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));

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

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Compare);

  template <typename T>
  static ExecResult NodeFunction_Builtin_IsValid(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bIsValid = true;
    if constexpr (std::is_same_v<T, float>)
    {
      bIsValid = xiiMath::IsFinite(inout_context.GetData<float>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, double>)
    {
      bIsValid = xiiMath::IsFinite(inout_context.GetData<double>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, xiiColor>)
    {
      bIsValid = inout_context.GetData<xiiColor>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, xiiVec3>)
    {
      bIsValid = inout_context.GetData<xiiVec3>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, xiiQuat>)
    {
      bIsValid = inout_context.GetData<xiiQuat>(dataOffset).IsValid();
    }
    else if constexpr (std::is_same_v<T, xiiString>)
    {
      bIsValid = inout_context.GetData<xiiString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, xiiHashedString>)
    {
      bIsValid = inout_context.GetData<xiiHashedString>(dataOffset).IsEmpty() == false;
    }
    else if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                       std::is_same_v<T, xiiComponentHandle> ||
                       std::is_same_v<T, xiiTypedPointer>)
    {
      bIsValid = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      bIsValid = inout_context.GetData<xiiVariant>(dataOffset).IsValid();
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bIsValid);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_IsValid);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_Add(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, xiiColor> ||
                  std::is_same_v<T, xiiVec3> ||
                  std::is_same_v<T, xiiTime> ||
                  std::is_same_v<T, xiiAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a + b));
    }
    else if constexpr (std::is_same_v<T, xiiString>)
    {
      auto& a = inout_context.GetData<xiiString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<xiiString>(node.GetInputDataOffset(1));

      xiiStringBuilder s;
      s.Set(a, b);

      inout_context.SetData(node.GetOutputDataOffset(0), xiiString(s.GetView()));
    }
    else if constexpr (std::is_same_v<T, xiiHashedString>)
    {
      auto& a = inout_context.GetData<xiiHashedString>(node.GetInputDataOffset(0));
      auto& b = inout_context.GetData<xiiHashedString>(node.GetInputDataOffset(1));

      xiiStringBuilder s;
      s.Set(a, b);
      xiiHashedString sHashed;
      sHashed.Assign(s);

      inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      xiiVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      xiiVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a + b);
    }
    else
    {
      xiiLog::Error("Add is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Add);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Sub(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, xiiColor> ||
                  std::is_same_v<T, xiiVec3> ||
                  std::is_same_v<T, xiiTime> ||
                  std::is_same_v<T, xiiAngle>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a - b));
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      xiiVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      xiiVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a - b);
    }
    else
    {
      xiiLog::Error("Subtract is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Sub);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Mul(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, xiiColor> ||
                  std::is_same_v<T, xiiTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a * b));
    }
    else if constexpr (std::is_same_v<T, xiiVec3>)
    {
      const xiiVec3& a = inout_context.GetData<xiiVec3>(node.GetInputDataOffset(0));
      const xiiVec3& b = inout_context.GetData<xiiVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompMul(b));
    }
    else if constexpr (std::is_same_v<T, xiiAngle>)
    {
      const xiiAngle& a = inout_context.GetData<xiiAngle>(node.GetInputDataOffset(0));
      const xiiAngle& b = inout_context.GetData<xiiAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), xiiAngle(a * b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      xiiVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      xiiVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a * b);
    }
    else
    {
      xiiLog::Error("Multiply is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Mul);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Div(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    if constexpr (std::is_same_v<T, xiiUInt8> ||
                  std::is_same_v<T, xiiInt32> ||
                  std::is_same_v<T, xiiInt64> ||
                  std::is_same_v<T, float> ||
                  std::is_same_v<T, double> ||
                  std::is_same_v<T, xiiTime>)
    {
      const T& a = inout_context.GetData<T>(node.GetInputDataOffset(0));
      const T& b = inout_context.GetData<T>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), T(a / b));
    }
    else if constexpr (std::is_same_v<T, xiiVec3>)
    {
      const xiiVec3& a = inout_context.GetData<xiiVec3>(node.GetInputDataOffset(0));
      const xiiVec3& b = inout_context.GetData<xiiVec3>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), a.CompDiv(b));
    }
    else if constexpr (std::is_same_v<T, xiiAngle>)
    {
      const xiiAngle& a = inout_context.GetData<xiiAngle>(node.GetInputDataOffset(0));
      const xiiAngle& b = inout_context.GetData<xiiAngle>(node.GetInputDataOffset(1));
      inout_context.SetData(node.GetOutputDataOffset(0), xiiAngle(a / b.GetRadian()));
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      xiiVariant a = inout_context.GetDataAsVariant(node.GetInputDataOffset(0), nullptr);
      xiiVariant b = inout_context.GetDataAsVariant(node.GetInputDataOffset(1), nullptr);
      inout_context.SetData(node.GetOutputDataOffset(0), a / b);
    }
    else
    {
      xiiLog::Error("Divide is not defined for type '{}'", GetTypeName<T>());
    }

    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Div);

  //////////////////////////////////////////////////////////////////////////

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToBool(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    bool bRes = false;
    if constexpr (std::is_same_v<T, bool>)
    {
      bRes = inout_context.GetData<T>(dataOffset);
    }
    else if constexpr (std::is_same_v<T, xiiUInt8> ||
                       std::is_same_v<T, xiiInt32> ||
                       std::is_same_v<T, xiiInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      bRes = inout_context.GetData<T>(dataOffset) != 0;
    }
    else if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                       std::is_same_v<T, xiiComponentHandle> ||
                       std::is_same_v<T, xiiTypedPointer>)
    {
      bRes = inout_context.GetPointerData(dataOffset).m_pObject != nullptr;
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      bRes = inout_context.GetData<xiiVariant>(dataOffset).ConvertTo<bool>();
    }
    else
    {
      xiiLog::Error("ToBool is not defined for type '{}'", GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), bRes);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToBool);

  template <typename NumberType, typename T>
  XII_FORCE_INLINE static ExecResult NodeFunction_Builtin_ToNumber(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node, const char* szName)
  {
    auto dataOffset = node.GetInputDataOffset(0);

    NumberType res = 0;
    if constexpr (std::is_same_v<T, bool>)
    {
      res = inout_context.GetData<T>(dataOffset) ? 1 : 0;
    }
    else if constexpr (std::is_same_v<T, xiiUInt8> ||
                       std::is_same_v<T, xiiInt32> ||
                       std::is_same_v<T, xiiInt64> ||
                       std::is_same_v<T, float> ||
                       std::is_same_v<T, double>)
    {
      res = static_cast<NumberType>(inout_context.GetData<T>(dataOffset));
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      res = inout_context.GetData<xiiVariant>(dataOffset).ConvertTo<NumberType>();
    }
    else
    {
      xiiLog::Error("To{} is not defined for type '{}'", szName, GetTypeName<T>());
    }

    inout_context.SetData(node.GetOutputDataOffset(0), res);
    return ExecResult::RunNext(0);
  }

#define MAKE_TONUMBER_EXEC_FUNC(NumberType, Name)                                                                                                                 \
  template <typename T>                                                                                                                                           \
  static ExecResult XII_CONCAT(NodeFunction_Builtin_To, Name)(xiiVisualScriptExecutionContext & inout_context, const xiiVisualScriptGraphDescription::Node& node) \
  {                                                                                                                                                               \
    return NodeFunction_Builtin_ToNumber<NumberType, T>(inout_context, node, #Name);                                                                              \
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
  static ExecResult NodeFunction_Builtin_ToString(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiStringBuilder sb;
    xiiStringView    s;
    if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                  std::is_same_v<T, xiiComponentHandle> ||
                  std::is_same_v<T, xiiTypedPointer>)
    {
      xiiTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      sb.SetFormat("{} {}", p.m_pType->GetTypeName(), xiiArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, xiiString>)
    {
      s = inout_context.GetData<xiiString>(node.GetInputDataOffset(0));
    }
    else
    {
      s = xiiConversionUtils::ToString(inout_context.GetData<T>(node.GetInputDataOffset(0)), sb);
    }

    inout_context.SetData(node.GetOutputDataOffset(0), s);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToString);

  static ExecResult NodeFunction_Builtin_String_Format(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& sText  = inout_context.GetData<xiiString>(node.GetInputDataOffset(0));
    auto& params = inout_context.GetData<xiiVariantArray>(node.GetInputDataOffset(1));

    xiiHybridArray<xiiString, 12> stringStorage;
    stringStorage.Reserve(params.GetCount());
    for (auto& param : params)
    {
      stringStorage.PushBack(param.ConvertTo<xiiString>());
    }

    xiiHybridArray<xiiStringView, 12> stringViews;
    stringViews.Reserve(stringStorage.GetCount());
    for (auto& s : stringStorage)
    {
      stringViews.PushBack(s);
    }

    xiiFormatString  fs(sText.GetView());
    xiiStringBuilder sStorage;
    xiiStringView    sFormatted = fs.BuildFormattedText(sStorage, stringViews.GetData(), stringViews.GetCount());

    inout_context.SetData(node.GetOutputDataOffset(0), sFormatted);
    return ExecResult::RunNext(0);
  }

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToHashedString(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiStringBuilder sb;
    xiiStringView    s;
    if constexpr (std::is_same_v<T, xiiGameObjectHandle> ||
                  std::is_same_v<T, xiiComponentHandle> ||
                  std::is_same_v<T, xiiTypedPointer>)
    {
      xiiTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      sb.SetFormat("{} {}", p.m_pType->GetTypeName(), xiiArgP(p.m_pObject));
      s = sb;
    }
    else if constexpr (std::is_same_v<T, xiiString>)
    {
      s = inout_context.GetData<xiiString>(node.GetInputDataOffset(0));
    }
    else if constexpr (std::is_same_v<T, xiiHashedString>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), inout_context.GetData<xiiHashedString>(node.GetInputDataOffset(0)));
      return ExecResult::RunNext(0);
    }
    else
    {
      s = xiiConversionUtils::ToString(inout_context.GetData<T>(node.GetInputDataOffset(0)), sb);
    }

    xiiHashedString sHashed;
    sHashed.Assign(s);
    inout_context.SetData(node.GetOutputDataOffset(0), sHashed);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToHashedString);

  template <typename T>
  static ExecResult NodeFunction_Builtin_ToVariant(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiVariant v;
    if constexpr (std::is_same_v<T, xiiTypedPointer>)
    {
      xiiTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
      v                 = xiiVariant(p.m_pObject, p.m_pType);
    }
    else
    {
      v = inout_context.GetData<T>(node.GetInputDataOffset(0));
    }
    inout_context.SetData(node.GetOutputDataOffset(0), v);
    return ExecResult::RunNext(0);
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_ToVariant);

  template <typename T>
  static ExecResult NodeFunction_Builtin_Variant_ConvertTo(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    const xiiVariant& v = inout_context.GetData<xiiVariant>(node.GetInputDataOffset(0));
    if constexpr (std::is_same_v<T, xiiTypedPointer>)
    {
      if (v.IsA<xiiTypedPointer>())
      {
        xiiTypedPointer typedPtr = v.Get<xiiTypedPointer>();
        inout_context.SetPointerData(node.GetOutputDataOffset(0), typedPtr.m_pObject, typedPtr.m_pType);
        return ExecResult::RunNext(0);
      }

      inout_context.SetPointerData<void*>(node.GetOutputDataOffset(0), nullptr, nullptr);
      return ExecResult::RunNext(1);
    }
    else if constexpr (std::is_same_v<T, xiiVariant>)
    {
      inout_context.SetData(node.GetOutputDataOffset(0), v);
      return ExecResult::RunNext(0);
    }
    else
    {
      xiiResult conversionResult = XII_SUCCESS;
      inout_context.SetData(node.GetOutputDataOffset(0), v.ConvertTo<T>(&conversionResult));
      return ExecResult::RunNext(conversionResult.Succeeded() ? 0 : 1);
    }
  }

  MAKE_EXEC_FUNC_GETTER(NodeFunction_Builtin_Variant_ConvertTo);

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_MakeArray(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiVariantArray& a = inout_context.GetWritableData<xiiVariantArray>(node.GetOutputDataOffset(0));
    a.Clear();
    a.Reserve(node.m_NumInputDataOffsets);

    for (xiiUInt32 i = 0; i < node.m_NumInputDataOffsets; ++i)
    {
      auto dataOffset = node.GetInputDataOffset(i);

      if (dataOffset.IsConstant())
      {
        a.PushBack(inout_context.GetDataAsVariant(dataOffset, nullptr));
      }
      else
      {
        a.PushBack(inout_context.GetData<xiiVariant>(dataOffset));
      }
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_GetElement(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    const xiiVariantArray& a       = inout_context.GetData<xiiVariantArray>(node.GetInputDataOffset(0));
    xiiUInt32              uiIndex = inout_context.GetData<int>(node.GetInputDataOffset(1));
    inout_context.SetData(node.GetOutputDataOffset(0), a[uiIndex]);

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_Array_GetCount(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    const xiiVariantArray& a = inout_context.GetData<xiiVariantArray>(node.GetInputDataOffset(0));
    inout_context.SetData<int>(node.GetOutputDataOffset(0), a.GetCount());

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_TryGetComponentOfBaseType(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto& userData = node.GetUserData<NodeUserData_Type>();

    xiiTypedPointer p = inout_context.GetPointerData(node.GetInputDataOffset(0));
    if (p.m_pType != xiiGetStaticRTTI<xiiGameObject>())
    {
      xiiLog::Error("Visual script call TryGetComponentOfBaseType: Game object is not of type 'xiiGameObject'");
      return ExecResult::Error();
    }

    if (p.m_pObject == nullptr)
    {
      xiiLog::Error("Visual script call TryGetComponentOfBaseType: Game object is null");
      return ExecResult::Error();
    }

    xiiComponent* pComponent = nullptr;
    static_cast<xiiGameObject*>(p.m_pObject)->TryGetComponentOfBaseType(userData.m_pType, pComponent);
    inout_context.SetPointerData(node.GetOutputDataOffset(0), pComponent);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  static ExecResult NodeFunction_Builtin_StartCoroutine(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto&     userData = node.GetUserData<NodeUserData_StartCoroutine>();
    xiiString sName    = inout_context.GetData<xiiString>(node.GetInputDataOffset(0));

    xiiScriptCoroutine* pCoroutine = nullptr;
    auto                hCoroutine = pModule->CreateCoroutine(userData.m_pType, sName, inout_context.GetInstance(), userData.m_CreationMode, pCoroutine);
    pModule->StartCoroutine(hCoroutine, xiiArrayPtr<xiiVariant>());

    inout_context.SetData(node.GetOutputDataOffset(0), hCoroutine);


    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopCoroutine(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    auto hCoroutine = inout_context.GetData<xiiScriptCoroutineHandle>(node.GetInputDataOffset(0));
    if (pModule->IsCoroutineFinished(hCoroutine) == false)
    {
      pModule->StopAndDeleteCoroutine(hCoroutine);
    }
    else
    {
      auto sName = inout_context.GetData<xiiString>(node.GetInputDataOffset(1));
      pModule->StopAndDeleteCoroutine(sName, &inout_context.GetInstance());
    }

    return ExecResult::RunNext(0);
  }

  static ExecResult NodeFunction_Builtin_StopAllCoroutines(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    pModule->StopAndDeleteAllCoroutines(&inout_context.GetInstance());

    return ExecResult::RunNext(0);
  }

  template <bool bWaitForAll>
  static ExecResult NodeFunction_Builtin_WaitForX(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    auto pModule = GetScriptModule(inout_context);
    if (pModule == nullptr)
      return ExecResult::Error();

    const xiiUInt32 uiNumCoroutines         = node.m_NumInputDataOffsets;
    xiiUInt32       uiNumFinishedCoroutines = 0;

    for (xiiUInt32 i = 0; i < uiNumCoroutines; ++i)
    {
      auto hCoroutine = inout_context.GetData<xiiScriptCoroutineHandle>(node.GetInputDataOffset(i));
      if (pModule->IsCoroutineFinished(hCoroutine))
      {
        if constexpr (bWaitForAll == false)
        {
          return ExecResult::RunNext(0);
        }
        else
        {
          ++uiNumFinishedCoroutines;
        }
      }
    }

    if constexpr (bWaitForAll)
    {
      if (uiNumFinishedCoroutines == uiNumCoroutines)
      {
        return ExecResult::RunNext(0);
      }
    }

    return ExecResult::ContinueLater(xiiTime::Zero());
  }

  static ExecResult NodeFunction_Builtin_Yield(xiiVisualScriptExecutionContext& inout_context, const xiiVisualScriptGraphDescription::Node& node)
  {
    xiiScriptCoroutine* pCoroutine = inout_context.GetCurrentCoroutine();
    if (pCoroutine == nullptr)
    {
      // set marker value of 0x1 to indicate we are in a yield
      inout_context.SetCurrentCoroutine(reinterpret_cast<xiiScriptCoroutine*>(0x1));

      return ExecResult::ContinueLater(xiiTime::Zero());
    }

    inout_context.SetCurrentCoroutine(nullptr);

    return ExecResult::RunNext(0);
  }

  //////////////////////////////////////////////////////////////////////////

  struct ExecuteFunctionContext
  {
    xiiVisualScriptGraphDescription::ExecuteFunction m_Func       = nullptr;
    ExecuteFunctionGetter                            m_FuncGetter = nullptr;
  };

  static ExecuteFunctionContext s_TypeToExecuteFunctions[] = {
    {},                                                   // Invalid,
    {},                                                   // EntryCall,
    {},                                                   // EntryCall_Coroutine,
    {},                                                   // MessageHandler,
    {},                                                   // MessageHandler_Coroutine,
    {&NodeFunction_ReflectedFunction},                    // ReflectedFunction,
    {nullptr, &NodeFunction_GetReflectedProperty_Getter}, // GetReflectedProperty,
    {nullptr, &NodeFunction_SetReflectedProperty_Getter}, // SetReflectedProperty,
    {&NodeFunction_InplaceCoroutine},                     // InplaceCoroutine,
    {&NodeFunction_GetScriptOwner},                       // GetScriptOwner,
    {&NodeFunction_SendMessage},                          // SendMessage,

    {}, // FirstBuiltin,

    {},                                                  // Builtin_Constant,
    {},                                                  // Builtin_GetVariable,
    {nullptr, &NodeFunction_Builtin_SetVariable_Getter}, // Builtin_SetVariable,
    {nullptr, &NodeFunction_Builtin_IncVariable_Getter}, // Builtin_IncVariable,
    {nullptr, &NodeFunction_Builtin_DecVariable_Getter}, // Builtin_DecVariable,

    {&NodeFunction_Builtin_Branch},                 // Builtin_Branch,
    {nullptr, &NodeFunction_Builtin_Switch_Getter}, // Builtin_Switch,
    {},                                             // Builtin_WhileLoop,
    {},                                             // Builtin_ForLoop,
    {},                                             // Builtin_ForEachLoop,
    {},                                             // Builtin_ReverseForEachLoop,
    {},                                             // Builtin_Break,
    {},                                             // Builtin_Jump,

    {&NodeFunction_Builtin_And},                     // Builtin_And,
    {&NodeFunction_Builtin_Or},                      // Builtin_Or,
    {&NodeFunction_Builtin_Not},                     // Builtin_Not,
    {nullptr, &NodeFunction_Builtin_Compare_Getter}, // Builtin_Compare,
    {},                                              // Builtin_CompareExec,
    {nullptr, &NodeFunction_Builtin_IsValid_Getter}, // Builtin_IsValid,
    {},                                              // Builtin_Select,

    {nullptr, &NodeFunction_Builtin_Add_Getter}, // Builtin_Add,
    {nullptr, &NodeFunction_Builtin_Sub_Getter}, // Builtin_Subtract,
    {nullptr, &NodeFunction_Builtin_Mul_Getter}, // Builtin_Multiply,
    {nullptr, &NodeFunction_Builtin_Div_Getter}, // Builtin_Divide,
    {},                                          // Builtin_Expression,

    {nullptr, &NodeFunction_Builtin_ToBool_Getter},            // Builtin_ToBool,
    {nullptr, &NodeFunction_Builtin_ToByte_Getter},            // Builtin_ToByte,
    {nullptr, &NodeFunction_Builtin_ToInt_Getter},             // Builtin_ToInt,
    {nullptr, &NodeFunction_Builtin_ToInt64_Getter},           // Builtin_ToInt64,
    {nullptr, &NodeFunction_Builtin_ToFloat_Getter},           // Builtin_ToFloat,
    {nullptr, &NodeFunction_Builtin_ToDouble_Getter},          // Builtin_ToDouble,
    {nullptr, &NodeFunction_Builtin_ToString_Getter},          // Builtin_ToString,
    {&NodeFunction_Builtin_String_Format},                     // Builtin_String_Format,
    {nullptr, &NodeFunction_Builtin_ToHashedString_Getter},    // Builtin_ToHashedString,
    {nullptr, &NodeFunction_Builtin_ToVariant_Getter},         // Builtin_ToVariant,
    {nullptr, &NodeFunction_Builtin_Variant_ConvertTo_Getter}, // Builtin_Variant_ConvertTo,

    {&NodeFunction_Builtin_MakeArray},        // Builtin_MakeArray
    {&NodeFunction_Builtin_Array_GetElement}, // Builtin_Array_GetElement,
    {},                                       // Builtin_Array_SetElement,
    {&NodeFunction_Builtin_Array_GetCount},   // Builtin_Array_GetCount,
    {},                                       // Builtin_Array_IsEmpty,
    {},                                       // Builtin_Array_Clear,
    {},                                       // Builtin_Array_Contains,
    {},                                       // Builtin_Array_IndexOf,
    {},                                       // Builtin_Array_Insert,
    {},                                       // Builtin_Array_PushBack,
    {},                                       // Builtin_Array_Remove,
    {},                                       // Builtin_Array_RemoveAt,

    {&NodeFunction_Builtin_TryGetComponentOfBaseType}, // Builtin_TryGetComponentOfBaseType

    {&NodeFunction_Builtin_StartCoroutine},    // Builtin_StartCoroutine,
    {&NodeFunction_Builtin_StopCoroutine},     // Builtin_StopCoroutine,
    {&NodeFunction_Builtin_StopAllCoroutines}, // Builtin_StopAllCoroutines,
    {&NodeFunction_Builtin_WaitForX<true>},    // Builtin_WaitForAll,
    {&NodeFunction_Builtin_WaitForX<false>},   // Builtin_WaitForAny,
    {&NodeFunction_Builtin_Yield},             // Builtin_Yield,

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
