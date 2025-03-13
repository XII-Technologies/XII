#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState_NestedStateMachine, 1, xiiRTTIDefaultAllocator<xiiStateMachineState_NestedStateMachine>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Resource", GetResource, SetResource)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_StateMachine", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    XII_MEMBER_PROPERTY("KeepCurrentStateOnExit", m_bKeepCurrentStateOnExit),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineState_NestedStateMachine::xiiStateMachineState_NestedStateMachine(xiiStringView sName) :
  xiiStateMachineState(sName)
{
}

xiiStateMachineState_NestedStateMachine::~xiiStateMachineState_NestedStateMachine() = default;

void xiiStateMachineState_NestedStateMachine::OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;

  if (pStateMachineInstance == nullptr)
  {
    if (m_hResource.IsValid() == false)
      return;

    xiiResourceLock<xiiStateMachineResource> pStateMachineResource(m_hResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pStateMachineResource.GetAcquireResult() != xiiResourceAcquireResult::Final)
    {
      xiiLog::Error("Failed to load state machine '{}'", GetResource().GetResourceID());
      return;
    }

    pStateMachineInstance = pStateMachineResource->CreateInstance(ref_instance.GetOwner());
    pStateMachineInstance->SetBlackboard(ref_instance.GetBlackboard());
  }

  if (pStateMachineInstance->GetCurrentState() == nullptr)
  {
    pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
  }
}

void xiiStateMachineState_NestedStateMachine::OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const
{
  if (m_bKeepCurrentStateOnExit == false)
  {
    auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
    if (pStateMachineInstance != nullptr)
    {
      pStateMachineInstance->SetState(nullptr).IgnoreResult();
    }
  }
}

void xiiStateMachineState_NestedStateMachine::Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
  if (pStateMachineInstance != nullptr)
  {
    pStateMachineInstance->Update(deltaTime);
  }
}

xiiResult xiiStateMachineState_NestedStateMachine::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_hResource;
  inout_stream << m_sInitialState;
  inout_stream << m_bKeepCurrentStateOnExit;
  return XII_SUCCESS;
}

xiiResult xiiStateMachineState_NestedStateMachine::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_hResource;
  inout_stream >> m_sInitialState;
  inout_stream >> m_bKeepCurrentStateOnExit;
  return XII_SUCCESS;
}

bool xiiStateMachineState_NestedStateMachine::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void xiiStateMachineState_NestedStateMachine::SetResource(const xiiStateMachineResourceHandle& hResource)
{
  m_hResource = hResource;
}

void xiiStateMachineState_NestedStateMachine::SetInitialState(const char* szName)
{
  m_sInitialState.Assign(szName);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState_Compound, 1, xiiRTTIDefaultAllocator<xiiStateMachineState_Compound>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("SubStates", m_SubStates)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineState_Compound::xiiStateMachineState_Compound(xiiStringView sName) :
  xiiStateMachineState(sName)
{
}

xiiStateMachineState_Compound::~xiiStateMachineState_Compound()
{
  for (auto pSubState : m_SubStates)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

void xiiStateMachineState_Compound::OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const
{
  auto pData = static_cast<xiiStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  for (xiiUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnEnter(ref_instance, pSubInstanceData, pFromState);
  }
}

void xiiStateMachineState_Compound::OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const
{
  auto pData = static_cast<xiiStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (xiiUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnExit(ref_instance, pSubInstanceData, pToState);
  }
}

void xiiStateMachineState_Compound::Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const
{
  auto pData = static_cast<xiiStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (xiiUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->Update(ref_instance, pSubInstanceData, deltaTime);
  }
}

xiiResult xiiStateMachineState_Compound::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  const xiiUInt32 uiNumSubStates = m_SubStates.GetCount();
  inout_stream << uiNumSubStates;

  for (auto pSubState : m_SubStates)
  {
    auto pStateType = pSubState->GetDynamicRTTI();
    xiiTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    XII_SUCCEED_OR_RETURN(pSubState->Serialize(inout_stream));
  }

  return XII_SUCCESS;
}

xiiResult xiiStateMachineState_Compound::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  xiiUInt32 uiNumSubStates = 0;
  inout_stream >> uiNumSubStates;
  m_SubStates.Reserve(uiNumSubStates);

  xiiStringBuilder sTypeName;
  for (xiiUInt32 i = 0; i < uiNumSubStates; ++i)
  {
    inout_stream >> sTypeName;
    if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
    {
      xiiUniquePtr<xiiStateMachineState> pSubState = pType->GetAllocator()->Allocate<xiiStateMachineState>();
      XII_SUCCEED_OR_RETURN(pSubState->Deserialize(inout_stream));

      m_SubStates.PushBack(pSubState.Release());
    }
    else
    {
      xiiLog::Error("Unknown state machine state type '{}'", sTypeName);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

bool xiiStateMachineState_Compound::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubStates.GetArrayPtr(), out_desc);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiStateMachineLogicOperator, 1)
  XII_ENUM_CONSTANTS(xiiStateMachineLogicOperator::And, xiiStateMachineLogicOperator::Or)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineTransition_BlackboardConditions, 1, xiiRTTIDefaultAllocator<xiiStateMachineTransition_BlackboardConditions>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiStateMachineLogicOperator, m_Operator),
    XII_ARRAY_MEMBER_PROPERTY("Conditions", m_Conditions),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineTransition_BlackboardConditions::xiiStateMachineTransition_BlackboardConditions()  = default;
xiiStateMachineTransition_BlackboardConditions::~xiiStateMachineTransition_BlackboardConditions() = default;

bool xiiStateMachineTransition_BlackboardConditions::IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const
{
  if (m_Conditions.IsEmpty())
    return true;

  auto pBlackboard = ref_instance.GetBlackboard();
  if (pBlackboard == nullptr)
    return false;

  const bool bCheckFor = (m_Operator == xiiStateMachineLogicOperator::Or) ? true : false;
  for (auto& condition : m_Conditions)
  {
    if (condition.IsConditionMet(*pBlackboard) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

xiiResult xiiStateMachineTransition_BlackboardConditions::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;
  return inout_stream.WriteArray(m_Conditions);
}

xiiResult xiiStateMachineTransition_BlackboardConditions::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Operator;
  return inout_stream.ReadArray(m_Conditions);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineTransition_Timeout, 1, xiiRTTIDefaultAllocator<xiiStateMachineTransition_Timeout>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Timeout", m_Timeout),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineTransition_Timeout::xiiStateMachineTransition_Timeout()  = default;
xiiStateMachineTransition_Timeout::~xiiStateMachineTransition_Timeout() = default;

bool xiiStateMachineTransition_Timeout::IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetTimeInCurrentState() >= m_Timeout;
}

xiiResult xiiStateMachineTransition_Timeout::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Timeout;
  return XII_SUCCESS;
}

xiiResult xiiStateMachineTransition_Timeout::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Timeout;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineTransition_Compound, 1, xiiRTTIDefaultAllocator<xiiStateMachineTransition_Compound>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("Operator", xiiStateMachineLogicOperator, m_Operator),
    XII_ARRAY_MEMBER_PROPERTY("SubTransitions", m_SubTransitions)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineTransition_Compound::xiiStateMachineTransition_Compound() = default;

xiiStateMachineTransition_Compound::~xiiStateMachineTransition_Compound()
{
  for (auto pSubState : m_SubTransitions)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

bool xiiStateMachineTransition_Compound::IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const
{
  auto pData = static_cast<xiiStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  const bool bCheckFor = (m_Operator == xiiStateMachineLogicOperator::Or) ? true : false;
  for (xiiUInt32 i = 0; i < m_SubTransitions.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    if (m_SubTransitions[i]->IsConditionMet(ref_instance, pSubInstanceData) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

xiiResult xiiStateMachineTransition_Compound::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;

  const xiiUInt32 uiNumSubTransitions = m_SubTransitions.GetCount();
  inout_stream << uiNumSubTransitions;

  for (auto pSubTransition : m_SubTransitions)
  {
    auto pStateType = pSubTransition->GetDynamicRTTI();
    xiiTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    XII_SUCCEED_OR_RETURN(pSubTransition->Serialize(inout_stream));
  }

  return XII_SUCCESS;
}

xiiResult xiiStateMachineTransition_Compound::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Operator;

  xiiUInt32 uiNumSubTransitions = 0;
  inout_stream >> uiNumSubTransitions;
  m_SubTransitions.Reserve(uiNumSubTransitions);

  xiiStringBuilder sTypeName;
  for (xiiUInt32 i = 0; i < uiNumSubTransitions; ++i)
  {
    inout_stream >> sTypeName;
    if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
    {
      xiiUniquePtr<xiiStateMachineTransition> pSubTransition = pType->GetAllocator()->Allocate<xiiStateMachineTransition>();
      XII_SUCCEED_OR_RETURN(pSubTransition->Deserialize(inout_stream));

      m_SubTransitions.PushBack(pSubTransition.Release());
    }
    else
    {
      xiiLog::Error("Unknown state machine state type '{}'", sTypeName);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

bool xiiStateMachineTransition_Compound::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubTransitions.GetArrayPtr(), out_desc);
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineTransition_TransitionEvent, 1, xiiRTTIDefaultAllocator<xiiStateMachineTransition_TransitionEvent>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EventName", m_sEventName),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineTransition_TransitionEvent::xiiStateMachineTransition_TransitionEvent()  = default;
xiiStateMachineTransition_TransitionEvent::~xiiStateMachineTransition_TransitionEvent() = default;

bool xiiStateMachineTransition_TransitionEvent::IsConditionMet(xiiStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetCurrentTransitionEvent() == m_sEventName.GetView();
}

xiiResult xiiStateMachineTransition_TransitionEvent::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sEventName;
  return XII_SUCCESS;
}

xiiResult xiiStateMachineTransition_TransitionEvent::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sEventName;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineBuiltins);
