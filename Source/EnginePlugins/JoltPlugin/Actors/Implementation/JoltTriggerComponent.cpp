#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

xiiJoltTriggerComponentManager::xiiJoltTriggerComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiJoltTriggerComponent, xiiBlockStorageType::FreeList>(pWorld)
{
}

xiiJoltTriggerComponentManager::~xiiJoltTriggerComponentManager() = default;

void xiiJoltTriggerComponentManager::UpdateMovingTriggers()
{
  XII_PROFILE_SCOPE("MovingTriggers");

  xiiJoltWorldModule* pModule       = GetWorld()->GetModule<xiiJoltWorldModule>();
  auto&               bodyInterface = pModule->GetJoltSystem()->GetBodyInterface();

  for (auto pTrigger : m_MovingTriggers)
  {
    JPH::BodyID bodyId(pTrigger->m_uiJoltBodyID);

    xiiSimdTransform trans = pTrigger->GetOwner()->GetGlobalTransformSimd();

    bodyInterface.SetPositionAndRotation(bodyId, xiiJoltConversionUtils::ToVec3(trans.m_Position), xiiJoltConversionUtils::ToQuat(trans.m_Rotation), JPH::EActivation::Activate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltTriggerComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage)
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGESENDERS
  {
    XII_MESSAGE_SENDER(m_TriggerEventSender)
  }
  XII_END_MESSAGESENDERS
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltTriggerComponent::xiiJoltTriggerComponent()  = default;
xiiJoltTriggerComponent::~xiiJoltTriggerComponent() = default;

void xiiJoltTriggerComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_sTriggerMessage;
}

void xiiJoltTriggerComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sTriggerMessage;
}

void xiiJoltTriggerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  xiiJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, nullptr).Failed())
  {
    xiiLog::Error("Jolt trigger actor component has no valid shape.");
    return;
  }

  const xiiSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  bodyCfg.mIsSensor    = true;
  bodyCfg.mPosition    = xiiJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation    = xiiJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType  = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Trigger);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<xiiUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID   = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);

  if (GetOwner()->IsDynamic())
  {
    xiiJoltTriggerComponentManager* pManager = static_cast<xiiJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Insert(this);
  }
}

void xiiJoltTriggerComponent::OnDeactivated()
{
  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  xiiJoltContactListener* pContactListener = pModule->GetContactListener();
  pContactListener->RemoveTrigger(this);

  if (GetOwner()->IsDynamic())
  {
    xiiJoltTriggerComponentManager* pManager = static_cast<xiiJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Remove(this);
  }

  SUPER::OnDeactivated();
}

void xiiJoltTriggerComponent::PostTriggerMessage(const xiiGameObjectHandle& hOtherObject, xiiTriggerState::Enum triggerState) const
{
  xiiMsgTriggerTriggered msg;

  msg.m_TriggerState      = triggerState;
  msg.m_sMessage          = m_sTriggerMessage;
  msg.m_hTriggeringObject = hOtherObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), xiiTime::Zero(), xiiObjectMsgQueueType::PostTransform);
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltTriggerComponent);
