/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/StateMachine/StateMachineComponent.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgStateMachineStateChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgStateMachineStateChanged, 1, xiiRTTIDefaultAllocator<xiiMsgStateMachineStateChanged>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("OldStateName", GetOldStateName, SetOldStateName),
    XII_ACCESSOR_PROPERTY("NewStateName", GetNewStateName, SetNewStateName),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState_SendMsg, 1, xiiRTTIDefaultAllocator<xiiStateMachineState_SendMsg>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MessageDelay", m_MessageDelay),
    XII_MEMBER_PROPERTY("SendMessageOnEnter", m_bSendMessageOnEnter)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("SendMessageOnExit", m_bSendMessageOnExit),
    XII_MEMBER_PROPERTY("LogOnEnter", m_bLogOnEnter),
    XII_MEMBER_PROPERTY("LogOnExit", m_bLogOnExit),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineState_SendMsg::xiiStateMachineState_SendMsg(xiiStringView sName) :
  xiiStateMachineState(sName)
{
}

xiiStateMachineState_SendMsg::~xiiStateMachineState_SendMsg() = default;

void xiiStateMachineState_SendMsg::OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const
{
  xiiHashedString sFromState = (pFromState != nullptr) ? pFromState->GetNameHashed() : xiiHashedString();

  if (m_bSendMessageOnEnter)
  {
    if (auto pOwner = xiiDynamicCast<xiiStateMachineComponent*>(&ref_instance.GetOwner()))
    {
      xiiMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = sFromState;
      msg.m_sNewStateName = GetNameHashed();

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnEnter)
  {
    xiiLog::Info("State Machine: Entering '{}' State from '{}'", GetNameHashed(), sFromState);
  }
}

void xiiStateMachineState_SendMsg::OnExit(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pToState) const
{
  xiiHashedString sToState = (pToState != nullptr) ? pToState->GetNameHashed() : xiiHashedString();

  if (m_bSendMessageOnExit)
  {
    if (auto pOwner = xiiDynamicCast<xiiStateMachineComponent*>(&ref_instance.GetOwner()))
    {
      xiiMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = GetNameHashed();
      msg.m_sNewStateName = sToState;

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnExit)
  {
    xiiLog::Info("State Machine: Exiting '{}' State to '{}'", GetNameHashed(), sToState);
  }
}

xiiResult xiiStateMachineState_SendMsg::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_MessageDelay;
  inout_stream << m_bSendMessageOnEnter;
  inout_stream << m_bSendMessageOnExit;
  inout_stream << m_bLogOnEnter;
  inout_stream << m_bLogOnExit;
  return XII_SUCCESS;
}

xiiResult xiiStateMachineState_SendMsg::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_MessageDelay;
  inout_stream >> m_bSendMessageOnEnter;
  inout_stream >> m_bSendMessageOnExit;
  inout_stream >> m_bLogOnEnter;
  inout_stream >> m_bLogOnExit;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineState_SwitchObject, 1, xiiRTTIDefaultAllocator<xiiStateMachineState_SwitchObject>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PathToGroup", m_sGroupPath),
    XII_MEMBER_PROPERTY("ObjectToEnable", m_sObjectToEnable),
    XII_MEMBER_PROPERTY("DeactivateOthers", m_bDeactivateOthers)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiStateMachineState_SwitchObject::xiiStateMachineState_SwitchObject(xiiStringView sName) :
  xiiStateMachineState(sName)
{
}

xiiStateMachineState_SwitchObject::~xiiStateMachineState_SwitchObject() = default;

void xiiStateMachineState_SwitchObject::OnEnter(xiiStateMachineInstance& ref_instance, void* pInstanceData, const xiiStateMachineState* pFromState) const
{
  if (auto pOwner = xiiDynamicCast<xiiStateMachineComponent*>(&ref_instance.GetOwner()))
  {
    if (xiiGameObject* pOwnerGO = pOwner->GetOwner()->FindChildByPath(m_sGroupPath))
    {
      for (auto it = pOwnerGO->GetChildren(); it.IsValid(); ++it)
      {
        if (it->GetName() == m_sObjectToEnable)
        {
          it->SetActiveFlag(true);
        }
        else if (m_bDeactivateOthers)
        {
          it->SetActiveFlag(false);
        }
      }
    }
  }
}

xiiResult xiiStateMachineState_SwitchObject::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sGroupPath;
  inout_stream << m_sObjectToEnable;
  inout_stream << m_bDeactivateOthers;
  return XII_SUCCESS;
}

xiiResult xiiStateMachineState_SwitchObject::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sGroupPath;
  inout_stream >> m_sObjectToEnable;
  inout_stream >> m_bDeactivateOthers;
  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

xiiStateMachineComponentManager::xiiStateMachineComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<ComponentType, xiiBlockStorageType::Compact>(pWorld)
{
  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiStateMachineComponentManager::ResourceEventHandler, this));
}

xiiStateMachineComponentManager::~xiiStateMachineComponentManager()
{
  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiStateMachineComponentManager::ResourceEventHandler, this));
}

void xiiStateMachineComponentManager::Initialize()
{
  auto desc = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiStateMachineComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void xiiStateMachineComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  // reload
  {
    for (auto hComponent : m_ComponentsToReload)
    {
      xiiStateMachineComponent* pComponent = nullptr;
      if (TryGetComponent(hComponent, pComponent) && pComponent->IsActive())
      {
        pComponent->InstantiateStateMachine();
      }
    }
    m_ComponentsToReload.Clear();
  }

  // update
  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      ComponentType* pComponent = it;
      if (pComponent->IsActiveAndSimulating())
      {
        pComponent->Update();
      }
    }
  }
}

void xiiStateMachineComponentManager::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiStateMachineResource>())
  {
    xiiStateMachineResourceHandle hResource((xiiStateMachineResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        m_ComponentsToReload.Insert(it->GetHandle());
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiStateMachineComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Resource", GetResource, SetResource)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_StateMachine", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    XII_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName)->AddAttributes(new xiiDynamicStringEnumAttribute("BlackboardNamesEnum")),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_MESSAGESENDERS
  {
    XII_MESSAGE_SENDER(m_StateChangedSender)
  }
  XII_END_MESSAGESENDERS;

  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetState, In, "Name"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetCurrentState),
    XII_SCRIPT_FUNCTION_PROPERTY(FireTransitionEvent, In, "Name"),
  }
  XII_END_FUNCTIONS;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Logic"),
  }
  XII_END_ATTRIBUTES;
}

XII_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

xiiStateMachineComponent::xiiStateMachineComponent()                                            = default;
xiiStateMachineComponent::xiiStateMachineComponent(xiiStateMachineComponent&& other)            = default;
xiiStateMachineComponent::~xiiStateMachineComponent()                                           = default;
xiiStateMachineComponent& xiiStateMachineComponent::operator=(xiiStateMachineComponent&& other) = default;

void xiiStateMachineComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s << m_sInitialState;
  s << m_sBlackboardName;
}

void xiiStateMachineComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32  uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = inout_stream.GetStream();

  s >> m_hResource;
  s >> m_sInitialState;

  if (uiVersion >= 2)
  {
    s >> m_sBlackboardName;
  }
}

void xiiStateMachineComponent::OnActivated()
{
  SUPER::OnActivated();

  InstantiateStateMachine();
}

void xiiStateMachineComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_pStateMachineInstance = nullptr;
}

void xiiStateMachineComponent::SetResource(const xiiStateMachineResourceHandle& hResource)
{
  if (m_hResource == hResource)
    return;

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void xiiStateMachineComponent::SetInitialState(const char* szName)
{
  xiiHashedString sInitialState;
  sInitialState.Assign(szName);

  if (m_sInitialState == sInitialState)
    return;

  m_sInitialState = std::move(sInitialState);

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void xiiStateMachineComponent::SetBlackboardName(const char* szName)
{
  xiiHashedString sBlackboardName;
  sBlackboardName.Assign(szName);

  if (m_sBlackboardName == sBlackboardName)
    return;

  m_sBlackboardName = std::move(sBlackboardName);

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

bool xiiStateMachineComponent::SetState(xiiStringView sName)
{
  if (m_pStateMachineInstance != nullptr)
  {
    xiiHashedString sStateName;
    sStateName.Assign(sName);

    return m_pStateMachineInstance->SetState(sStateName).Succeeded();
  }

  return false;
}

xiiStringView xiiStateMachineComponent::GetCurrentState() const
{
  if (m_pStateMachineInstance != nullptr && m_pStateMachineInstance->GetCurrentState())
  {
    return m_pStateMachineInstance->GetCurrentState()->GetName();
  }

  return {};
}

void xiiStateMachineComponent::FireTransitionEvent(xiiStringView sEvent)
{
  if (m_pStateMachineInstance != nullptr)
  {
    m_pStateMachineInstance->FireTransitionEvent(sEvent);
  }
}

void xiiStateMachineComponent::SendStateChangedMsg(xiiMsgStateMachineStateChanged& msg, xiiTime delay)
{
  if (delay > xiiTime::MakeZero())
  {
    m_StateChangedSender.PostEventMessage(msg, this, GetOwner(), delay, xiiObjectMsgQueueType::NextFrame);
  }
  else
  {
    m_StateChangedSender.SendEventMessage(msg, this, GetOwner());
  }
}

void xiiStateMachineComponent::InstantiateStateMachine()
{
  m_pStateMachineInstance = nullptr;

  if (m_hResource.IsValid() == false)
    return;

  xiiResourceLock<xiiStateMachineResource> pStateMachineResource(m_hResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pStateMachineResource.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    xiiLog::Error("Failed to load state machine '{}'", GetResource().GetResourceID());
    return;
  }

  m_pStateMachineInstance = pStateMachineResource->CreateInstance(*this);
  m_pStateMachineInstance->SetBlackboard(xiiBlackboardComponent::FindBlackboard(GetOwner(), m_sBlackboardName.GetView()));
  m_pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
}

void xiiStateMachineComponent::Update()
{
  if (m_pStateMachineInstance != nullptr)
  {
    m_pStateMachineInstance->Update(GetWorld()->GetClock().GetTimeDiff());
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineComponent);
