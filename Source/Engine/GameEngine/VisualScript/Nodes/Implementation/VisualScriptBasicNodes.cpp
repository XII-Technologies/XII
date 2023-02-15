#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptBasicNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

namespace
{
  const void* GetDataPointer(const xiiVariant& var, const xiiRTTI* pTargetType)
  {
    if (pTargetType == xiiGetStaticRTTI<xiiVariant>())
    {
      return &var;
    }

    return var.GetData();
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Sequence, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Sequence>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic")
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    XII_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    XII_OUTPUT_EXECUTION_PIN("then1", 0),
    XII_OUTPUT_EXECUTION_PIN("then2", 1),
    XII_OUTPUT_EXECUTION_PIN("then3", 2),
    XII_OUTPUT_EXECUTION_PIN("then4", 3),
    XII_OUTPUT_EXECUTION_PIN("then5", 4),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Sequence::xiiVisualScriptNode_Sequence()  = default;
xiiVisualScriptNode_Sequence::~xiiVisualScriptNode_Sequence() = default;

void xiiVisualScriptNode_Sequence::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
  pInstance->ExecuteConnectedNodes(this, 1);
  pInstance->ExecuteConnectedNodes(this, 2);
  pInstance->ExecuteConnectedNodes(this, 3);
  pInstance->ExecuteConnectedNodes(this, 4);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Delay, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Delay>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
    new xiiTitleAttribute("Delay: '{Delay}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    XII_INPUT_DATA_PIN_AND_PROPERTY("Delay", 0, xiiVisualScriptDataPinType::Number, m_Delay),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Delay::xiiVisualScriptNode_Delay()
{
  xiiStringBuilder sb;
  sb.Format("VisualSciptDelay_{}", xiiArgP(this));

  m_sMessage.Assign(sb.GetView());
}

xiiVisualScriptNode_Delay::~xiiVisualScriptNode_Delay() = default;

void xiiVisualScriptNode_Delay::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_bMessageReceived)
  {
    pInstance->ExecuteConnectedNodes(this, 0);
    m_bMessageReceived = false;
  }
  else
  {
    xiiMsgComponentInternalTrigger msg;
    msg.m_sMessage = m_sMessage;
    pInstance->GetWorld()->PostMessage(pInstance->GetOwnerComponent(), msg, m_Delay);
  }
}

void* xiiVisualScriptNode_Delay::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Delay;
  }

  return nullptr;
}

xiiInt32 xiiVisualScriptNode_Delay::HandlesMessagesWithID() const
{
  return xiiMsgComponentInternalTrigger::GetTypeMsgId();
}

void xiiVisualScriptNode_Delay::HandleMessage(xiiMessage* pMsg)
{
  xiiMsgComponentInternalTrigger& msg = *static_cast<xiiMsgComponentInternalTrigger*>(pMsg);

  if (msg.m_sMessage == m_sMessage)
  {
    m_bMessageReceived = true;
    m_bStepNode        = true;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_Log, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_Log>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Debug"),
    new xiiTitleAttribute("Log: '{Text}'"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    // Properties
    XII_MEMBER_PROPERTY("Text", m_sLog)->AddAttributes(new xiiDefaultValueAttribute(xiiStringView("Value1: {0}, Value2: {1}, Value3: {2}"))),
    // Execution Pins
    XII_INPUT_EXECUTION_PIN("run", 0),
    XII_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins
    XII_MEMBER_PROPERTY("Value0", m_Value0)->AddAttributes(new xiiVisScriptDataPinInAttribute(0, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
    XII_MEMBER_PROPERTY("Value1", m_Value1)->AddAttributes(new xiiVisScriptDataPinInAttribute(1, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
    XII_MEMBER_PROPERTY("Value2", m_Value2)->AddAttributes(new xiiVisScriptDataPinInAttribute(2, xiiVisualScriptDataPinType::Variant), new xiiDefaultValueAttribute(0)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_Log::xiiVisualScriptNode_Log() {}
xiiVisualScriptNode_Log::~xiiVisualScriptNode_Log() {}

void xiiVisualScriptNode_Log::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  xiiLog::Info(m_sLog, m_Value0, m_Value1, m_Value2);

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_Log::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value0;
    case 1:
      return &m_Value1;
    case 2:
      return &m_Value2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_MessageSender, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_MessageSender>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Messages"),
    new xiiHiddenAttribute()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_MessageSender::xiiVisualScriptNode_MessageSender() = default;
xiiVisualScriptNode_MessageSender::~xiiVisualScriptNode_MessageSender()
{
  for (xiiUInt32 uiProp = 0; uiProp < m_PropertyIndexToDataPinType.GetCount(); ++uiProp)
  {
    auto dataPinType = m_PropertyIndexToDataPinType[uiProp];
    if (dataPinType == xiiVisualScriptDataPinType::None)
      continue;

    xiiUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
    void*     ptr      = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];

    switch (dataPinType)
    {
      case xiiVisualScriptDataPinType::String:
        static_cast<xiiString*>(ptr)->~xiiString();
        break;

      case xiiVisualScriptDataPinType::Variant:
        static_cast<xiiVariant*>(ptr)->~xiiVariant();
        break;

      default:
        // Nothing to do for other types
        break;
    }
  }

  m_ScratchMemory.Clear();
}

void xiiVisualScriptNode_MessageSender::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_pMessageToSend != nullptr)
  {
    // fill message properties from inputs
    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

    const xiiUInt8 uiPropCount = static_cast<xiiUInt8>(properties.GetCount());
    for (xiiUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
    {
      auto dataPinType = m_PropertyIndexToDataPinType[uiProp];
      if (dataPinType == xiiVisualScriptDataPinType::None)
        continue;

      xiiUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
      void*     ptr      = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];

      xiiVariant var;

      switch (dataPinType)
      {
        case xiiVisualScriptDataPinType::Number:
          var = *static_cast<double*>(ptr);
          break;

        case xiiVisualScriptDataPinType::Boolean:
          var = *static_cast<bool*>(ptr);
          break;

        case xiiVisualScriptDataPinType::Vec3:
          var = *static_cast<xiiVec3*>(ptr);
          break;

        case xiiVisualScriptDataPinType::String:
          var = *static_cast<xiiString*>(ptr);
          break;

        case xiiVisualScriptDataPinType::Variant:
          var = *static_cast<xiiVariant*>(ptr);
          break;

          XII_DEFAULT_CASE_NOT_IMPLEMENTED;
      }

      xiiAbstractMemberProperty* pAbsMember = static_cast<xiiAbstractMemberProperty*>(properties[uiProp]);
      xiiReflectionUtils::SetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow(), var);
    }


    xiiWorld* pWorld = pInstance->GetWorld();

    if (m_Delay.GetSeconds() == 0)
    {
      // Delay == 0 -> SendMessage

      if (!m_hComponent.IsInvalidated())
      {
        xiiComponent* pComponent = nullptr;
        if (pWorld->TryGetComponent(m_hComponent, pComponent))
        {
          pComponent->SendMessage(*m_pMessageToSend);
        }
      }
      else
      {
        xiiGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;
        xiiGameObject*      pObject = nullptr;
        if (pWorld->TryGetObject(hObject, pObject))
        {
          if (m_bRecursive)
          {
            pObject->SendMessageRecursive(*m_pMessageToSend);
          }
          else
          {
            pObject->SendMessage(*m_pMessageToSend);
          }
        }
      }

      {
        // could skip this, if we knew that there are no output pins, at all
        for (xiiUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
        {
          if (properties[uiProp]->GetCategory() == xiiPropertyCategory::Member &&
              properties[uiProp]->GetFlags().IsAnySet(xiiPropertyFlags::VarInOut | xiiPropertyFlags::VarOut))
          {
            xiiAbstractMemberProperty* pAbsMember = static_cast<xiiAbstractMemberProperty*>(properties[uiProp]);

            const xiiRTTI* pType = pAbsMember->GetSpecificType();
            if (xiiVisualScriptDataPinType::IsTypeSupported(pType))
            {
              xiiVariant var = xiiReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow());
              xiiVisualScriptDataPinType::EnforceSupportedType(var);
              pInstance->SetOutputPinValue(this, uiProp, GetDataPointer(var, pType));
            }
          }
        }
      }
    }
    else
    {
      // Delay > 0 -> PostMessage

      if (!m_hComponent.IsInvalidated())
      {
        pWorld->PostMessage(m_hComponent, *m_pMessageToSend, m_Delay);
      }
      else
      {
        xiiGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;
        if (m_bRecursive)
        {
          pWorld->PostMessageRecursive(hObject, *m_pMessageToSend, m_Delay);
        }
        else
        {
          pWorld->PostMessage(hObject, *m_pMessageToSend, m_Delay);
        }
      }
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_MessageSender::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  if (uiPin == 0)
    return &m_hObject;

  if (uiPin == 1)
    return &m_hComponent;

  if (uiPin == 2)
    return &m_Delay;

  if (m_pMessageToSend != nullptr)
  {
    const xiiUInt32 uiProp = uiPin - 3;

    xiiUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
    if (uiOffset != 0xFFFF)
    {
      void* pPropertyPointer = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];
      return pPropertyPointer;
    }

    XII_ASSERT_NOT_IMPLEMENTED;
  }

  return nullptr;
}

void xiiVisualScriptNode_MessageSender::SetMessageToSend(xiiUniquePtr<xiiMessage>&& pMsg)
{
  m_pMessageToSend = std::move(pMsg);

  // Calculate scratch memory size and build mapping from property index to memory offset
  xiiUInt32 uiScratchMemorySize = 0;

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  m_pMessageToSend->GetDynamicRTTI()->GetAllProperties(properties);

  const xiiUInt8 uiPropCount = static_cast<xiiUInt8>(properties.GetCount());
  m_PropertyIndexToMemoryOffset.SetCount(uiPropCount, 0xFFFF);
  m_PropertyIndexToDataPinType.SetCount(uiPropCount);

  for (xiiUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    if (properties[uiProp]->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pAbsMember = static_cast<xiiAbstractMemberProperty*>(properties[uiProp]);

      const xiiRTTI* pType       = pAbsMember->GetSpecificType();
      auto           dataPinType = xiiVisualScriptDataPinType::GetDataPinTypeForType(pType);
      if (dataPinType == xiiVisualScriptDataPinType::None)
        continue;

      m_PropertyIndexToMemoryOffset[uiProp] = static_cast<xiiUInt16>(uiScratchMemorySize);
      m_PropertyIndexToDataPinType[uiProp]  = dataPinType;

      uiScratchMemorySize += xiiMath::Max<xiiUInt32>(xiiVisualScriptDataPinType::GetStorageByteSize(dataPinType), XII_ALIGNMENT_MINIMUM);
    }
  }

  m_ScratchMemory.SetCountUninitialized(uiScratchMemorySize);
  m_ScratchMemory.ZeroFill();

  // Default construct and assign initial values
  for (xiiUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    auto dataPinType = m_PropertyIndexToDataPinType[uiProp];
    if (dataPinType == xiiVisualScriptDataPinType::None)
      continue;

    xiiUInt32 uiOffset = m_PropertyIndexToMemoryOffset[uiProp];
    void*     ptr      = &m_ScratchMemory.GetByteBlobPtr()[uiOffset];

    xiiAbstractMemberProperty* pAbsMember = static_cast<xiiAbstractMemberProperty*>(properties[uiProp]);
    xiiVariant                 var        = xiiReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMessageToSend.Borrow());
    xiiVisualScriptDataPinType::EnforceSupportedType(var);

    switch (dataPinType)
    {
      case xiiVisualScriptDataPinType::String:
        new (ptr) xiiString(var.Get<xiiString>());
        break;

      case xiiVisualScriptDataPinType::Variant:
        new (ptr) xiiVariant(var);
        break;

      default:
        xiiMemoryUtils::RawByteCopy(ptr, var.GetData(), xiiVisualScriptDataPinType::GetStorageByteSize(dataPinType));
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_MessageHandler, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_MessageHandler>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Messages"),
    new xiiHiddenAttribute()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_MessageHandler::xiiVisualScriptNode_MessageHandler() {}
xiiVisualScriptNode_MessageHandler::~xiiVisualScriptNode_MessageHandler() {}

void xiiVisualScriptNode_MessageHandler::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_pMsgCopy == nullptr)
    return;

  xiiHybridArray<xiiAbstractProperty*, 32> properties;
  m_pMsgCopy->GetDynamicRTTI()->GetAllProperties(properties);

  const xiiUInt8 uiPropCount = static_cast<xiiUInt8>(properties.GetCount());
  for (xiiUInt8 uiProp = 0; uiProp < uiPropCount; ++uiProp)
  {
    auto prop = properties[uiProp];

    if (prop->GetCategory() == xiiPropertyCategory::Member)
    {
      xiiAbstractMemberProperty* pAbsMember = static_cast<xiiAbstractMemberProperty*>(prop);

      const xiiRTTI* pType = pAbsMember->GetSpecificType();
      if (xiiVisualScriptDataPinType::IsTypeSupported(pType))
      {
        xiiVariant var = xiiReflectionUtils::GetMemberPropertyValue(pAbsMember, m_pMsgCopy.Borrow());
        xiiVisualScriptDataPinType::EnforceSupportedType(var);
        pInstance->SetOutputPinValue(this, uiProp, GetDataPointer(var, pType));
      }
      else
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);

  m_pMsgCopy = nullptr;
}

xiiInt32 xiiVisualScriptNode_MessageHandler::HandlesMessagesWithID() const
{
  xiiInt32 res = -1;

  if (m_pMessageTypeToHandle != nullptr && m_pMessageTypeToHandle->IsDerivedFrom<xiiMessage>() && m_pMessageTypeToHandle->GetAllocator()->CanAllocate())
  {
    xiiUniquePtr<xiiMessage> pMsg = m_pMessageTypeToHandle->GetAllocator()->Allocate<xiiMessage>();
    res                           = pMsg->GetId();
  }

  return res;
}

void xiiVisualScriptNode_MessageHandler::HandleMessage(xiiMessage* pMsg)
{
  xiiRTTIAllocator* pMsgRTTIAllocator = pMsg->GetDynamicRTTI()->GetAllocator();
  m_pMsgCopy                          = pMsgRTTIAllocator->Clone<xiiMessage>(pMsg);

  m_bStepNode = true;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVisualScriptNode_FunctionCall, 1, xiiRTTIDefaultAllocator<xiiVisualScriptNode_FunctionCall>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Functions"),
    new xiiHiddenAttribute()
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiVisualScriptNode_FunctionCall::xiiVisualScriptNode_FunctionCall() {}

xiiVisualScriptNode_FunctionCall::~xiiVisualScriptNode_FunctionCall() {}

void xiiVisualScriptNode_FunctionCall::Execute(xiiVisualScriptInstance* pInstance, xiiUInt8 uiExecPin)
{
  if (m_pFunctionToCall == nullptr)
    return;

  xiiWorld* pWorld = pInstance->GetWorld();

  if (m_hComponent.IsInvalidated())
  {
    // no component given -> try to look up the component type on the object instead

    if (m_hObject.IsInvalidated())
    {
      xiiLog::Error("VisScript function call: Target component or object is not specified");

      m_pFunctionToCall = nullptr;
      return;
    }

    xiiGameObject* pObject;
    if (!pWorld->TryGetObject(m_hObject, pObject))
    {
      // object is dead -> deactivate this node silently
      m_pFunctionToCall = nullptr;
      return;
    }

    xiiComponent* pComponent;
    if (!pObject->TryGetComponentOfBaseType(m_pExpectedType, pComponent))
    {
      xiiLog::Error("VisScript function call: Target object does not have a component of type {}", m_pExpectedType->GetTypeName());

      m_pFunctionToCall = nullptr;
      return;
    }

    m_hComponent = pComponent->GetHandle();
  }

  xiiComponent* pComponent = nullptr;
  if (!pWorld->TryGetComponent(m_hComponent, pComponent))
  {
    // component is dead -> deactivate this node silently
    m_pFunctionToCall = nullptr;
    return;
  }

  if (!pComponent->GetDynamicRTTI()->IsDerivedFrom(m_pExpectedType))
  {
    xiiLog::Error("VisScript function call: Target component of type '{}' is not of the expected base type '{}'",
                  pComponent->GetDynamicRTTI()->GetTypeName(), m_pExpectedType->GetTypeName());

    m_pFunctionToCall = nullptr;
    return;
  }

  for (xiiUInt32 arg = 0; arg < m_pFunctionToCall->GetArgumentCount(); ++arg)
  {
    const xiiRTTI* pArgumentType = m_pFunctionToCall->GetArgumentType(arg);
    if (pArgumentType == xiiGetStaticRTTI<xiiVariant>())
      continue; // Nothing to do

    const xiiVariant&          var        = m_Arguments[arg];
    const xiiVariantType::Enum targetType = pArgumentType->GetVariantType();

    if (ConvertArgumentToRequiredType(m_Arguments[arg], targetType).Failed())
    {
      xiiLog::Error("VisScript function call: Could not convert argument {} from variant type '{}' to target type '{}'", arg, (int)var.GetType(),
                    (int)targetType);

      // probably a stale script with a mismatching pin <-> argument configuration
      m_pFunctionToCall = nullptr;
      return;
    }
  }

  // call the function on the target object
  m_pFunctionToCall->Execute(pComponent, m_Arguments, m_ReturnValue);

  // now we need to pull the data from return values and out parameters and pass them into our output pins
  xiiUInt8 uiOutputPinIndex = 0;

  if (m_ReturnValue.IsValid())
  {
    xiiVisualScriptDataPinType::EnforceSupportedType(m_ReturnValue);
    pInstance->SetOutputPinValue(this, uiOutputPinIndex, GetDataPointer(m_ReturnValue, m_pFunctionToCall->GetReturnType()));
    ++uiOutputPinIndex;
  }

  for (xiiUInt32 arg = 0; arg < m_pFunctionToCall->GetArgumentCount(); ++arg)
  {
    // also do this for non-out parameters, as an 'in' parameter may still be a non-const reference (bad but valid)
    xiiVisualScriptDataPinType::EnforceSupportedType(m_Arguments[arg]);

    if ((m_ArgumentIsOutParamMask & XII_BIT(arg)) != 0) // if this argument represents an out or inout parameter, pull the data
    {
      const xiiRTTI* pArgumentType = m_pFunctionToCall->GetArgumentType(arg);

      pInstance->SetOutputPinValue(this, uiOutputPinIndex, GetDataPointer(m_Arguments[arg], pArgumentType));
      ++uiOutputPinIndex;
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* xiiVisualScriptNode_FunctionCall::GetInputPinDataPointer(xiiUInt8 uiPin)
{
  if (uiPin == 0)
    return &m_hObject;

  if (uiPin == 1)
    return &m_hComponent;

  if (uiPin >= m_Arguments.GetCount() + 2)
    return &m_ReturnValue; // unused dummy just to return anything in case of a mismatch

  return m_Arguments[uiPin - 2].GetWriteAccess().m_pObject;
}

xiiResult xiiVisualScriptNode_FunctionCall::ConvertArgumentToRequiredType(xiiVariant& var, xiiVariantType::Enum type)
{
  if (var.GetType() == type)
    return XII_SUCCESS;

  xiiResult couldConvert = XII_FAILURE;
  var                    = var.ConvertTo(type, &couldConvert);

  return couldConvert;
}

//////////////////////////////////////////////////////////////////////////


XII_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_Implementation_VisualScriptBasicNodes);
