#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/OffsetCenterOfMassShape.h>

xiiJoltDynamicActorComponentManager::xiiJoltDynamicActorComponentManager(xiiWorld* pWorld) :
  xiiComponentManager<xiiJoltDynamicActorComponent, xiiBlockStorageType::FreeList>(pWorld)
{
}

xiiJoltDynamicActorComponentManager::~xiiJoltDynamicActorComponentManager() = default;

void xiiJoltDynamicActorComponentManager::UpdateDynamicActors()
{
  XII_PROFILE_SCOPE("UpdateDynamicActors");

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();

  for (auto& itActor : pModule->GetActiveActors())
  {
    xiiJoltDynamicActorComponent* pActor = itActor;

    JPH::BodyID bodyId(pActor->GetJoltBodyID());

    JPH::BodyLockRead bodyLock(pSystem->GetBodyLockInterface(), bodyId);
    if (!bodyLock.Succeeded())
      continue;

    const JPH::Body& body = bodyLock.GetBody();

    if (!body.IsDynamic())
      continue;

    xiiSimdTransform trans = pActor->GetOwner()->GetGlobalTransformSimd();

    trans.m_Position = xiiJoltConversionUtils::ToSimdVec3(body.GetPosition());
    trans.m_Rotation = xiiJoltConversionUtils::ToSimdQuat(body.GetRotation());

    pActor->GetOwner()->SetGlobalTransform(trans);
  }
}

void xiiJoltDynamicActorComponentManager::UpdateKinematicActors(xiiTime deltaTime)
{
  XII_PROFILE_SCOPE("UpdateKinematicActors");

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();
  auto*               pBodies = &pSystem->GetBodyInterface();

  const float tDiff = deltaTime.AsFloatInSeconds();

  for (auto pKinematicActorComponent : m_KinematicActorComponents)
  {
    JPH::BodyID bodyId(pKinematicActorComponent->m_uiJoltBodyID);

    if (bodyId.IsInvalid())
      continue;

    xiiGameObject* pObject = pKinematicActorComponent->GetOwner();

    pObject->UpdateGlobalTransform();

    const xiiSimdVec4f pos = pObject->GetGlobalPositionSimd();
    const xiiSimdQuat  rot = pObject->GetGlobalRotationSimd();

    pBodies->MoveKinematic(bodyId, xiiJoltConversionUtils::ToVec3(pos), xiiJoltConversionUtils::ToQuat(rot).Normalized(), tDiff);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltDynamicActorComponent, 3, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
      XII_ACCESSOR_PROPERTY("Kinematic", GetKinematic, SetKinematic),
      XII_MEMBER_PROPERTY("StartAsleep", m_bStartAsleep),
      XII_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new xiiSuffixAttribute(" kg"), new xiiClampValueAttribute(0.0f, xiiVariant())),
      XII_MEMBER_PROPERTY("Density", m_fDensity)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiSuffixAttribute(" kg/m^3")),
      XII_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Surface", xiiDependencyFlags::Package)),
      XII_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
      XII_MEMBER_PROPERTY("LinearDamping", m_fLinearDamping)->AddAttributes(new xiiDefaultValueAttribute(0.2f)),
      XII_MEMBER_PROPERTY("AngularDamping", m_fAngularDamping)->AddAttributes(new xiiDefaultValueAttribute(0.2f)),
      XII_MEMBER_PROPERTY("ContinuousCollisionDetection", m_bCCD),
      XII_BITFLAGS_MEMBER_PROPERTY("OnContact", xiiOnJoltContact, m_OnContact),
      XII_ACCESSOR_PROPERTY("CustomCenterOfMass", GetUseCustomCoM, SetUseCustomCoM),
      XII_MEMBER_PROPERTY("CenterOfMass", m_vCenterOfMass),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
      XII_MESSAGE_HANDLER(xiiMsgPhysicsAddForce, AddForceAtPos),
      XII_MESSAGE_HANDLER(xiiMsgPhysicsAddImpulse, AddImpulseAtPos),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(AddLinearForce, In, "vForce"),
    XII_SCRIPT_FUNCTION_PROPERTY(AddLinearImpulse, In, "vImpulse"),
    XII_SCRIPT_FUNCTION_PROPERTY(AddAngularForce, In, "vForce"),
    XII_SCRIPT_FUNCTION_PROPERTY(AddAngularImpulse, In, "vImpulse"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiTransformManipulatorAttribute("CenterOfMass")
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiJoltDynamicActorComponent::xiiJoltDynamicActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

xiiJoltDynamicActorComponent::~xiiJoltDynamicActorComponent() = default;

void xiiJoltDynamicActorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_bKinematic;
  s << m_bCCD;
  s << m_fLinearDamping;
  s << m_fAngularDamping;
  s << m_fDensity;
  s << m_fMass;
  s << m_fGravityFactor;
  s << m_hSurface;
  s << m_OnContact;
  s << GetUseCustomCoM();
  s << m_vCenterOfMass;
  s << m_bStartAsleep;
}

void xiiJoltDynamicActorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_bKinematic;
  s >> m_bCCD;
  s >> m_fLinearDamping;
  s >> m_fAngularDamping;
  s >> m_fDensity;
  s >> m_fMass;
  s >> m_fGravityFactor;
  s >> m_hSurface;
  s >> m_OnContact;

  if (uiVersion >= 2)
  {
    bool com;
    s >> com;
    SetUseCustomCoM(com);
    s >> m_vCenterOfMass;
  }

  if (uiVersion >= 3)
  {
    s >> m_bStartAsleep;
  }
}

void xiiJoltDynamicActorComponent::SetKinematic(bool b)
{
  if (m_bKinematic == b)
    return;

  m_bKinematic = b;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (m_bKinematic && !bodyId.IsInvalid())
  {
    // do not insert this, until we actually have an actor pointer
    GetWorld()->GetOrCreateComponentManager<xiiJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
  else
  {
    GetWorld()->GetOrCreateComponentManager<xiiJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  if (bodyId.IsInvalid())
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  auto*               pSystem = pModule->GetJoltSystem();

  {
    JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

    if (bodyLock.Succeeded())
    {
      JPH::Body& body = bodyLock.GetBody();
      body.SetMotionType(m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
    }
  }

  if (!m_bKinematic && pSystem->GetBodyInterface().IsAdded(bodyId))
  {
    pSystem->GetBodyInterface().ActivateBody(bodyId);
  }
}

void xiiJoltDynamicActorComponent::SetGravityFactor(float fFactor)
{
  if (m_fGravityFactor == fFactor)
    return;

  m_fGravityFactor = fFactor;

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (bodyId.IsInvalid())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>()->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pSystem->GetBodyLockInterface(), bodyId);

  if (bodyLock.Succeeded())
  {
    bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_fGravityFactor);

    if (pSystem->GetBodyInterfaceNoLock().IsAdded(bodyId))
    {
      pSystem->GetBodyInterfaceNoLock().ActivateBody(bodyId);
    }
  }
}

void xiiJoltDynamicActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiJoltWorldModule*    pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  const xiiSimdTransform trans   = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem   = pModule->GetJoltSystem();
  auto* pBodies   = &pSystem->GetBodyInterface();
  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;

  if (CreateShape(&bodyCfg, m_fDensity, pMaterial).Failed())
  {
    xiiLog::Error("Jolt dynamic actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  if (pMaterial == nullptr)
    pMaterial = xiiJoltCore::GetDefaultMaterial();

  xiiJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex          = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  bodyCfg.mPosition                     = xiiJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation                     = xiiJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType                   = m_bKinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
  bodyCfg.mObjectLayer                  = xiiJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, xiiJoltBroadphaseLayer::Dynamic);
  bodyCfg.mMotionQuality                = m_bCCD ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
  bodyCfg.mLinearDamping                = m_fLinearDamping;
  bodyCfg.mAngularDamping               = m_fAngularDamping;
  bodyCfg.mMassPropertiesOverride.mMass = m_fMass;
  bodyCfg.mOverrideMassProperties       = m_fMass > 0.0f ? JPH::EOverrideMassProperties::CalculateInertia : JPH::EOverrideMassProperties::CalculateMassAndInertia;
  bodyCfg.mGravityFactor                = m_fGravityFactor;
  bodyCfg.mRestitution                  = pMaterial->m_fRestitution;
  bodyCfg.mFriction                     = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<xiiUInt64>(pUserData);

  if (GetUseCustomCoM())
  {
    const xiiVec3 vGlobalScale       = GetOwner()->GetGlobalScaling();
    const float   scale              = xiiMath::Min(vGlobalScale.x, vGlobalScale.y, vGlobalScale.z);
    auto          vLocalCenterOfMass = xiiSimdVec4f(scale * m_vCenterOfMass.x, scale * m_vCenterOfMass.y, scale * m_vCenterOfMass.z);
    auto          vPrevCoM           = xiiJoltConversionUtils::ToSimdVec3(bodyCfg.GetShape()->GetCenterOfMass());

    auto vComShift = vLocalCenterOfMass - vPrevCoM;

    JPH::OffsetCenterOfMassShapeSettings com;
    com.mOffset        = xiiJoltConversionUtils::ToVec3(vComShift);
    com.mInnerShapePtr = bodyCfg.GetShape();

    bodyCfg.SetShape(com.Create().Get());
  }

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID   = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, !m_bStartAsleep);

  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<xiiJoltDynamicActorComponentManager>()->m_KinematicActorComponents.PushBack(this);
  }
}

void xiiJoltDynamicActorComponent::OnDeactivated()
{
  if (m_bKinematic)
  {
    GetWorld()->GetOrCreateComponentManager<xiiJoltDynamicActorComponentManager>()->m_KinematicActorComponents.RemoveAndSwap(this);
  }

  xiiDynamicArray<xiiComponentHandle> allConstraints;
  allConstraints.Swap(m_Constraints);

  xiiJoltMsgDisconnectConstraints msg;
  msg.m_pActor       = this;
  msg.m_uiJoltBodyID = GetJoltBodyID();

  xiiWorld* pWorld = GetWorld();

  for (xiiComponentHandle hConstraint : allConstraints)
  {
    pWorld->SendMessage(hConstraint, msg);
  }

  SUPER::OnDeactivated();
}

void xiiJoltDynamicActorComponent::AddLinearForce(const xiiVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == xiiInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddForce(JPH::BodyID(m_uiJoltBodyID), xiiJoltConversionUtils::ToVec3(vForce));
}

void xiiJoltDynamicActorComponent::AddLinearImpulse(const xiiVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == xiiInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddImpulse(JPH::BodyID(m_uiJoltBodyID), xiiJoltConversionUtils::ToVec3(vImpulse));
}

void xiiJoltDynamicActorComponent::AddAngularForce(const xiiVec3& vForce)
{
  if (m_bKinematic || m_uiJoltBodyID == xiiInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddTorque(JPH::BodyID(m_uiJoltBodyID), xiiJoltConversionUtils::ToVec3(vForce));
}

void xiiJoltDynamicActorComponent::AddAngularImpulse(const xiiVec3& vImpulse)
{
  if (m_bKinematic || m_uiJoltBodyID == xiiInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddAngularImpulse(JPH::BodyID(m_uiJoltBodyID), xiiJoltConversionUtils::ToVec3(vImpulse));
}

void xiiJoltDynamicActorComponent::AddConstraint(xiiComponentHandle hComponent)
{
  m_Constraints.PushBack(hComponent);
}

void xiiJoltDynamicActorComponent::RemoveConstraint(xiiComponentHandle hComponent)
{
  m_Constraints.RemoveAndSwap(hComponent);
}

void xiiJoltDynamicActorComponent::AddForceAtPos(xiiMsgPhysicsAddForce& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == xiiInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddForce(JPH::BodyID(m_uiJoltBodyID), xiiJoltConversionUtils::ToVec3(ref_msg.m_vForce), xiiJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
}

void xiiJoltDynamicActorComponent::AddImpulseAtPos(xiiMsgPhysicsAddImpulse& ref_msg)
{
  if (m_bKinematic || m_uiJoltBodyID == xiiInvalidIndex)
    return;

  auto pBodies = &GetWorld()->GetModule<xiiJoltWorldModule>()->GetJoltSystem()->GetBodyInterface();
  pBodies->AddImpulse(JPH::BodyID(m_uiJoltBodyID), xiiJoltConversionUtils::ToVec3(ref_msg.m_vImpulse), xiiJoltConversionUtils::ToVec3(ref_msg.m_vGlobalPosition));
}

const xiiJoltMaterial* xiiJoltDynamicActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    xiiResourceLock<xiiSurfaceResource> pSurface(m_hSurface, xiiResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<xiiJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}

void xiiJoltDynamicActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    xiiResourceManager::PreloadResource(m_hSurface);
}

const char* xiiJoltDynamicActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltDynamicActorComponent);
