#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/Component.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GetName),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_OnEnter, In, "StateMachineInstance", In, "FromState")->AddAttributes(new xiiScriptBaseClassFunctionAttribute(xiiStateMachineState_ScriptBaseClassFunctions::OnEnter)),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_OnExit, In, "StateMachineInstance", In, "ToState")->AddAttributes(new xiiScriptBaseClassFunctionAttribute(xiiStateMachineState_ScriptBaseClassFunctions::OnExit)),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "StateMachineInstance", In, "DeltaTime")->AddAttributes(new xiiScriptBaseClassFunctionAttribute(xiiStateMachineState_ScriptBaseClassFunctions::Update)),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState_Empty, 1, xiiRTTIDefaultAllocator<xiiStateMachineState_Empty>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiHiddenAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineState::xiiStateMachineState(xiiStringView sName)
{
  m_sName.Assign(sName);
}

void xiiStateMachineState::SetName(xiiStringView sName)
{
  XII_ASSERT_DEV(m_sName.IsEmpty(), "Name can't be changed afterwards");
  m_sName.Assign(sName);
}

void xiiStateMachineState::OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const
{
}

void xiiStateMachineState::Update(xiiStateMachineInstance& ref_instance, void* pInstanceData, xiiTime deltaTime) const
{
}

xiiResult xiiStateMachineState::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  return XII_SUCCESS;
}

xiiResult xiiStateMachineState::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiUInt32 uiVersion = xiiTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  XII_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sName;
  return XII_SUCCESS;
}

bool xiiStateMachineState::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc)
{
  return false;
}

void xiiStateMachineState::Reflection_OnEnter(xiiStateMachineInstance* pStateMachineInstance, const xiiStateMachineState* pFromState)
{
}

void xiiStateMachineState::Reflection_OnExit(xiiStateMachineInstance* pStateMachineInstance, const xiiStateMachineState* pToState)
{
}

void xiiStateMachineState::Reflection_Update(xiiStateMachineInstance* pStateMachineInstance, xiiTime deltaTime)
{
}

xiiStateMachineState_Empty::xiiStateMachineState_Empty(xiiStringView sName) :
  xiiStateMachineState(sName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineTransition, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiStateMachineTransition::Serialize(xiiStreamWriter& inout_stream) const
{
  return XII_SUCCESS;
}

xiiResult xiiStateMachineTransition::Deserialize(xiiStreamReader& inout_stream)
{
  return XII_SUCCESS;
}

bool xiiStateMachineTransition::GetInstanceDataDesc(xiiInstanceDataDesc& out_desc)
{
  return false;
}

//////////////////////////////////////////////////////////////////////////

xiiStateMachineDescription::xiiStateMachineDescription()  = default;
xiiStateMachineDescription::~xiiStateMachineDescription() = default;

xiiUInt32 xiiStateMachineDescription::AddState(xiiUniquePtr<xiiStateMachineState>&& pState)
{
  const xiiUInt32 uiIndex = m_States.GetCount();

  auto& sStateName = pState->GetNameHashed();
  if (sStateName.IsEmpty() == false)
  {
    XII_VERIFY(m_StateNameToIndexTable.Contains(sStateName) == false, "A state with name '{}' already exists.", sStateName);
    m_StateNameToIndexTable.Insert(sStateName, uiIndex);
  }

  StateContext& stateContext = m_States.ExpandAndGetRef();

  xiiInstanceDataDesc instanceDataDesc;
  if (pState->GetInstanceDataDesc(instanceDataDesc))
  {
    stateContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  stateContext.m_pState = std::move(pState);

  return uiIndex;
}

void xiiStateMachineDescription::AddTransition(xiiUInt32 uiFromStateIndex, xiiUInt32 uiToStateIndex, xiiUniquePtr<xiiStateMachineTransition>&& pTransistion)
{
  XII_ASSERT_DEV(uiFromStateIndex != uiToStateIndex, "Can't add a transition to itself");

  TransitionArray* pTransitions = nullptr;
  if (uiFromStateIndex == xiiInvalidIndex)
  {
    pTransitions = &m_FromAnyTransitions;
  }
  else
  {
    XII_ASSERT_DEV(uiFromStateIndex < m_States.GetCount(), "Invalid from state index {}", uiFromStateIndex);
    pTransitions = &m_States[uiFromStateIndex].m_Transitions;
  }

  XII_ASSERT_DEV(uiToStateIndex < m_States.GetCount(), "Invalid to state index {}", uiToStateIndex);

  TransitionContext& transitionContext = pTransitions->ExpandAndGetRef();

  xiiInstanceDataDesc instanceDataDesc;
  if (pTransistion->GetInstanceDataDesc(instanceDataDesc))
  {
    transitionContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  transitionContext.m_pTransition    = std::move(pTransistion);
  transitionContext.m_uiToStateIndex = uiToStateIndex;
}

constexpr xiiTypeVersion s_StateMachineDescriptionVersion = 1;

xiiResult xiiStateMachineDescription::Serialize(xiiStreamWriter& ref_originalStream) const
{
  ref_originalStream.WriteVersion(s_StateMachineDescriptionVersion);

  xiiStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_originalStream);
  xiiTypeVersionWriteContext         typeVersionWriteContext;
  auto&                              stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  xiiUInt32 uiNumTransitions = m_FromAnyTransitions.GetCount();

  // states
  {
    const xiiUInt32 uiNumStates = m_States.GetCount();
    stream << uiNumStates;

    for (auto& stateContext : m_States)
    {
      auto pStateType = stateContext.m_pState->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pStateType);

      stream << pStateType->GetTypeName();
      XII_SUCCEED_OR_RETURN(stateContext.m_pState->Serialize(stream));

      uiNumTransitions += stateContext.m_Transitions.GetCount();
    }
  }

  // transitions
  {
    stream << uiNumTransitions;

    auto SerializeTransitions = [&](const TransitionArray& transitions, xiiUInt32 uiFromStateIndex) -> xiiResult {
      for (auto& transitionContext : transitions)
      {
        const xiiUInt32 uiToStateIndex = transitionContext.m_uiToStateIndex;

        stream << uiFromStateIndex;
        stream << uiToStateIndex;

        auto pTransitionType = transitionContext.m_pTransition->GetDynamicRTTI();
        typeVersionWriteContext.AddType(pTransitionType);

        stream << pTransitionType->GetTypeName();
        XII_SUCCEED_OR_RETURN(transitionContext.m_pTransition->Serialize(stream));
      }

      return XII_SUCCESS;
    };

    XII_SUCCEED_OR_RETURN(SerializeTransitions(m_FromAnyTransitions, xiiInvalidIndex));

    for (xiiUInt32 uiFromStateIndex = 0; uiFromStateIndex < m_States.GetCount(); ++uiFromStateIndex)
    {
      auto& transitions = m_States[uiFromStateIndex].m_Transitions;

      XII_SUCCEED_OR_RETURN(SerializeTransitions(transitions, uiFromStateIndex));
    }
  }

  XII_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  XII_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return XII_SUCCESS;
}

xiiResult xiiStateMachineDescription::Deserialize(xiiStreamReader& inout_stream)
{
  const auto uiVersion = inout_stream.ReadVersion(s_StateMachineDescriptionVersion);
  XII_IGNORE_UNUSED(uiVersion);

  xiiStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  xiiTypeVersionReadContext         typeVersionReadContext(inout_stream);

  xiiStringBuilder sTypeName;

  // states
  {
    xiiUInt32 uiNumStates = 0;
    inout_stream >> uiNumStates;

    for (xiiUInt32 i = 0; i < uiNumStates; ++i)
    {
      inout_stream >> sTypeName;
      if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
      {
        xiiUniquePtr<xiiStateMachineState> pState = pType->GetAllocator()->Allocate<xiiStateMachineState>();
        XII_SUCCEED_OR_RETURN(pState->Deserialize(inout_stream));

        XII_VERIFY(AddState(std::move(pState)) == i, "Implementation error");
      }
      else
      {
        xiiLog::Error("Unknown state machine state type '{}'", sTypeName);
        return XII_FAILURE;
      }
    }
  }

  // transitions
  {
    xiiUInt32 uiNumTransitions = 0;
    inout_stream >> uiNumTransitions;

    for (xiiUInt32 i = 0; i < uiNumTransitions; ++i)
    {
      xiiUInt32 uiFromStateIndex = 0;
      xiiUInt32 uiToStateIndex   = 0;

      inout_stream >> uiFromStateIndex;
      inout_stream >> uiToStateIndex;

      inout_stream >> sTypeName;
      if (const xiiRTTI* pType = xiiRTTI::FindTypeByName(sTypeName))
      {
        xiiUniquePtr<xiiStateMachineTransition> pTransition = pType->GetAllocator()->Allocate<xiiStateMachineTransition>();
        XII_SUCCEED_OR_RETURN(pTransition->Deserialize(inout_stream));

        AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
      }
      else
      {
        xiiLog::Error("Unknown state machine transition type '{}'", sTypeName);
        return XII_FAILURE;
      }
    }
  }

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiStateMachineInstance, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SetState, In, "StateName"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetCurrentState),
    XII_SCRIPT_FUNCTION_PROPERTY(GetTimeInCurrentState),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwnerComponent),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_GetBlackboard),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineInstance::xiiStateMachineInstance(xiiReflectedClass& ref_owner, const xiiSharedPtr<const xiiStateMachineDescription>& pDescription /*= nullptr*/) :
  m_Owner(ref_owner), m_pDescription(pDescription)
{
  if (pDescription != nullptr)
  {
    m_InstanceData = pDescription->m_InstanceDataAllocator.AllocateAndConstruct();
  }
}

xiiStateMachineInstance::~xiiStateMachineInstance()
{
  ExitCurrentState(nullptr);

  m_pCurrentState       = nullptr;
  m_uiCurrentStateIndex = xiiInvalidIndex;

  if (m_pDescription != nullptr)
  {
    m_pDescription->m_InstanceDataAllocator.DestructAndDeallocate(m_InstanceData);
  }
}

xiiResult xiiStateMachineInstance::SetState(xiiStateMachineState* pState)
{
  if (pState != nullptr && m_pDescription != nullptr)
  {
    return SetState(pState->GetNameHashed());
  }

  const auto pFromState = m_pCurrentState;
  const auto pToState   = pState;

  ExitCurrentState(pToState);

  m_pCurrentState       = pState;
  m_uiCurrentStateIndex = xiiInvalidIndex;
  m_pCurrentTransitions = nullptr;

  EnterCurrentState(pFromState);

  return XII_SUCCESS;
}

xiiResult xiiStateMachineInstance::SetState(const xiiHashedString& sStateName)
{
  XII_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by name");

  xiiUInt32 uiStateIndex = 0;
  if (m_pDescription->m_StateNameToIndexTable.TryGetValue(sStateName, uiStateIndex))
  {
    SetStateInternal(uiStateIndex);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiStateMachineInstance::SetState(xiiUInt32 uiStateIndex)
{
  XII_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by index");

  if (uiStateIndex < m_pDescription->m_States.GetCount())
  {
    SetStateInternal(uiStateIndex);
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

xiiResult xiiStateMachineInstance::SetStateOrFallback(const xiiHashedString& sStateName, xiiUInt32 uiFallbackStateIndex /*= 0*/)
{
  if (SetState(sStateName).Failed())
  {
    return SetState(uiFallbackStateIndex);
  }

  return XII_SUCCESS;
}

void xiiStateMachineInstance::Update(xiiTime deltaTime)
{
  xiiUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != xiiInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->Update(*this, pInstanceData, deltaTime);
  }

  m_TimeInCurrentState += deltaTime;
}

xiiWorld* xiiStateMachineInstance::GetOwnerWorld()
{
  if (auto pComponent = xiiDynamicCast<xiiComponent*>(&m_Owner))
  {
    return pComponent->GetWorld();
  }

  return nullptr;
}

void xiiStateMachineInstance::SetBlackboard(const xiiSharedPtr<xiiBlackboard>& pBlackboard)
{
  m_pBlackboard = pBlackboard;
}

void xiiStateMachineInstance::FireTransitionEvent(xiiStringView sEvent)
{
  m_sCurrentTransitionEvent = sEvent;

  xiiUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != xiiInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  m_sCurrentTransitionEvent = {};
}

bool xiiStateMachineInstance::Reflection_SetState(const xiiHashedString& sStateName)
{
  return SetState(sStateName).Succeeded();
}

xiiComponent* xiiStateMachineInstance::Reflection_GetOwnerComponent() const
{
  return xiiDynamicCast<xiiComponent*>(&m_Owner);
}

void xiiStateMachineInstance::SetStateInternal(xiiUInt32 uiStateIndex)
{
  if (m_uiCurrentStateIndex == uiStateIndex)
    return;

  const auto& stateContext = m_pDescription->m_States[uiStateIndex];
  const auto  pFromState   = m_pCurrentState;
  const auto  pToState     = stateContext.m_pState.Borrow();

  ExitCurrentState(pToState);

  m_pCurrentState       = pToState;
  m_uiCurrentStateIndex = uiStateIndex;
  m_pCurrentTransitions = &stateContext.m_Transitions;

  EnterCurrentState(pFromState);
}

void xiiStateMachineInstance::EnterCurrentState(const xiiStateMachineState* pFromState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnEnter(*this, pInstanceData, pFromState);

    m_TimeInCurrentState = xiiTime::Zero();
  }
}

void xiiStateMachineInstance::ExitCurrentState(const xiiStateMachineState* pToState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnExit(*this, pInstanceData, pToState);
  }
}

xiiUInt32 xiiStateMachineInstance::FindNewStateToTransitionTo()
{
  if (m_pCurrentTransitions != nullptr)
  {
    for (auto& transitionContext : *m_pCurrentTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  if (m_pDescription != nullptr)
  {
    for (auto& transitionContext : m_pDescription->m_FromAnyTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  return xiiInvalidIndex;
}


XII_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachine);
