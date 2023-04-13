#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Constraints/JoltGrabObjectComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Constraints/SixDOFConstraint.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgObjectGrabbed);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgObjectGrabbed, 1, xiiRTTIDefaultAllocator<xiiMsgObjectGrabbed>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    XII_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgReleaseObjectGrab);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgReleaseObjectGrab, 1, xiiRTTIDefaultAllocator<xiiMsgReleaseObjectGrab>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltGrabObjectComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MaxGrabPointDistance", m_fMaxGrabPointDistance)->AddAttributes(new xiiDefaultValueAttribute(2.0f)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),
    XII_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness)->AddAttributes(new xiiDefaultValueAttribute(2.0f), new xiiClampValueAttribute(1.0f, 60.0f)),
    XII_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("BreakDistance", m_fBreakDistance)->AddAttributes(new xiiDefaultValueAttribute(0.5f)),
    XII_ACCESSOR_PROPERTY("AttachTo", DummyGetter, SetAttachToReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_MEMBER_PROPERTY("GrabAnyObjectWithSize", m_fAllowGrabAnyObjectWithSize)->AddAttributes(new xiiDefaultValueAttribute(0.75f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(GrabNearbyObject),
    XII_SCRIPT_FUNCTION_PROPERTY(HasObjectGrabbed),
    XII_SCRIPT_FUNCTION_PROPERTY(DropGrabbedObject),
    XII_SCRIPT_FUNCTION_PROPERTY(ThrowGrabbedObject, In, "Direction"),
    XII_SCRIPT_FUNCTION_PROPERTY(BreakObjectGrab),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgReleaseObjectGrab, OnMsgReleaseObjectGrab),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Constraints"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiJoltGrabObjectComponent::xiiJoltGrabObjectComponent()  = default;
xiiJoltGrabObjectComponent::~xiiJoltGrabObjectComponent() = default;

void xiiJoltGrabObjectComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fBreakDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
  s << m_fMaxGrabPointDistance;
  s << m_uiCollisionLayer;
  s << m_fAllowGrabAnyObjectWithSize;

  stream.WriteGameObjectHandle(m_hAttachTo);
}

void xiiJoltGrabObjectComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fBreakDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;
  s >> m_fMaxGrabPointDistance;
  s >> m_uiCollisionLayer;
  s >> m_fAllowGrabAnyObjectWithSize;

  m_hAttachTo = stream.ReadGameObjectHandle();
}

bool xiiJoltGrabObjectComponent::FindNearbyObject(xiiGameObject*& out_pObject, xiiTransform& out_LocalGrabPoint) const
{
  const xiiPhysicsWorldModuleInterface* pPhysicsModule = GetWorld()->GetModuleReadOnly<xiiPhysicsWorldModuleInterface>();

  if (pPhysicsModule == nullptr)
    return false;

  auto pOwner = GetOwner();

  xiiPhysicsCastResult      hit;
  xiiPhysicsQueryParameters queryParam;
  queryParam.m_bIgnoreInitialOverlap = true;
  queryParam.m_uiCollisionLayer      = m_uiCollisionLayer;
  queryParam.m_ShapeTypes            = xiiPhysicsShapeType::Static | xiiPhysicsShapeType::Dynamic;

  if (!pPhysicsModule->Raycast(hit, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), m_fMaxGrabPointDistance * 5.0f, queryParam))
    return false;

  if (hit.m_fDistance > m_fMaxGrabPointDistance)
    return false;

  const xiiGameObject* pActorObj = nullptr;
  if (!GetWorld()->TryGetObject(hit.m_hActorObject, pActorObj))
    return false;

  const xiiJoltDynamicActorComponent* pActorComp = nullptr;
  if (!pActorObj->TryGetComponentOfBaseType(pActorComp))
    return false;

  if (pActorComp->GetKinematic())
    return false;

  if (DetermineGrabPoint(pActorComp, out_LocalGrabPoint).Failed())
    return false;

  out_pObject = const_cast<xiiGameObject*>(pActorObj);
  return true;
}

bool xiiJoltGrabObjectComponent::GrabObject(xiiGameObject* pObjectToGrab, const xiiTransform& localGrabPoint)
{
  if (m_pConstraint != nullptr || pObjectToGrab == nullptr)
    return false;

  const xiiTime curTime = GetWorld()->GetClock().GetAccumulatedTime();

  // a cooldown to grab something again after we stood on the held object
  if (m_LastValidTime > curTime)
    return false;

  xiiJoltDynamicActorComponent* pAttachToActor = GetAttachToActor();
  if (pAttachToActor == nullptr)
  {
    xiiLog::Error("Can't grab object, no target actor to attach it to is set.");
    return false;
  }

  xiiJoltDynamicActorComponent* pActorToGrab = nullptr;
  if (!pObjectToGrab->TryGetComponentOfBaseType(pActorToGrab))
    return false;

  if (pActorToGrab->GetKinematic())
    return false;

  if (IsCharacterStandingOnObject(pObjectToGrab->GetHandle()))
    return false;

  xiiJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    pController->SetObjectToIgnore(pActorToGrab->GetObjectFilterID());
  }

  m_ChildAnchorLocal = localGrabPoint;
  m_hGrabbedActor    = pActorToGrab->GetHandle();

  CreateJoint(pAttachToActor, pActorToGrab);

  xiiMsgObjectGrabbed msg;
  msg.m_bGotGrabbed = true;
  msg.m_hGrabbedBy  = GetOwner()->GetHandle();
  pActorToGrab->GetOwner()->SendMessage(msg);

  m_LastValidTime = curTime;

  return true;
}

bool xiiJoltGrabObjectComponent::GrabNearbyObject()
{
  xiiGameObject* pActorToGrab = nullptr;
  xiiTransform   localGrabPoint;
  if (!FindNearbyObject(pActorToGrab, localGrabPoint))
    return false;

  return GrabObject(pActorToGrab, localGrabPoint);
}

bool xiiJoltGrabObjectComponent::HasObjectGrabbed() const
{
  return m_pConstraint != nullptr;
}

void xiiJoltGrabObjectComponent::DropGrabbedObject()
{
  ReleaseGrabbedObject();
}

void xiiJoltGrabObjectComponent::ThrowGrabbedObject(const xiiVec3& vRelativeDir)
{
  xiiComponentHandle hActor = m_hGrabbedActor;
  ReleaseGrabbedObject();

  xiiJoltDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(hActor, pActor))
  {
    // TODO: normalize impulse with object mass (?)

    pActor->AddLinearImpulse(GetOwner()->GetGlobalRotation() * vRelativeDir);
  }
}

void xiiJoltGrabObjectComponent::BreakObjectGrab()
{
  ReleaseGrabbedObject();

  xiiMsgPhysicsJointBroke msg;
  msg.m_hJointObject = GetOwner()->GetHandle();

  GetOwner()->PostEventMessage(msg, this, xiiTime::Zero());
}

void xiiJoltGrabObjectComponent::SetAttachToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hAttachTo = resolver(szReference, GetHandle(), "AttachTo");
}

void xiiJoltGrabObjectComponent::ReleaseGrabbedObject()
{
  if (m_pConstraint == nullptr)
    return;

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  xiiJoltDynamicActorComponent* pGrabbedActor = nullptr;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    JPH::BodyLockWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), JPH::BodyID(pGrabbedActor->GetJoltBodyID()));
    if (bodyLock.Succeeded())
    {
      bodyLock.GetBody().GetMotionProperties()->SetInverseMass(m_fGrabbedActorMass);
      // TODO: this needs to be set as well : bodyLock.GetBody().GetMotionProperties()->SetInverseInertia(m_fGrabbedActorMass);
      bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_fGrabbedActorGravity);

      if (pModule->GetJoltSystem()->GetBodyInterfaceNoLock().IsAdded(JPH::BodyID(pGrabbedActor->GetJoltBodyID())))
      {
        pModule->GetJoltSystem()->GetBodyInterfaceNoLock().ActivateBody(JPH::BodyID(pGrabbedActor->GetJoltBodyID()));
      }
    }

    xiiMsgObjectGrabbed msg;
    msg.m_bGotGrabbed = false;
    msg.m_hGrabbedBy  = GetOwner()->GetHandle();
    pGrabbedActor->GetOwner()->SendMessage(msg);
  }

  xiiJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    pController->ClearObjectToIgnore();
  }

  pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

  m_pConstraint->Release();
  m_pConstraint = nullptr;

  m_hGrabbedActor.Invalidate();
}

xiiJoltDynamicActorComponent* xiiJoltGrabObjectComponent::GetAttachToActor()
{
  xiiJoltDynamicActorComponent* pActor  = nullptr;
  xiiGameObject*                pObject = nullptr;

  if (!GetWorld()->TryGetObject(m_hAttachTo, pObject))
    return nullptr;

  if (!pObject->TryGetComponentOfBaseType(pActor))
    return nullptr;

  if (!pActor->GetKinematic())
    return nullptr;

  return pActor;
}

xiiResult xiiJoltGrabObjectComponent::DetermineGrabPoint(const xiiComponent* pActorComp, xiiTransform& out_LocalGrabPoint) const
{
  out_LocalGrabPoint.SetIdentity();

  const xiiGameObject* pAttachToObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hAttachTo, pAttachToObject))
    return XII_FAILURE;

  const auto vAttachToPos = pAttachToObject->GetGlobalPosition();
  const auto vOwnerDir    = GetOwner()->GetGlobalDirForwards();
  const auto vOwnerUp     = GetOwner()->GetGlobalDirUp();
  const auto pActorObj    = pActorComp->GetOwner();

  const xiiTransform&                           actorTransform = pActorObj->GetGlobalTransform();
  xiiHybridArray<xiiGrabbableItemGrabPoint, 16> grabPoints;

  const xiiGrabbableItemComponent* pGrabbableItemComp = nullptr;
  if (pActorObj->TryGetComponentOfBaseType(pGrabbableItemComp) && !pGrabbableItemComp->m_GrabPoints.IsEmpty())
  {
    grabPoints = pGrabbableItemComp->m_GrabPoints;
  }
  else
  {
    xiiBoundingBoxSphere bounds = pActorComp->GetOwner()->GetLocalBounds();

    if (!bounds.IsValid())
    {
      bounds = xiiBoundingSphere(xiiVec3::ZeroVector(), 0.1f);
    }

    const auto&   box = bounds.GetBox();
    const xiiVec3 ext = box.GetExtents().CompMul(pActorComp->GetOwner()->GetGlobalScaling());

    if (ext.x <= m_fAllowGrabAnyObjectWithSize && ext.y <= m_fAllowGrabAnyObjectWithSize && ext.z <= m_fAllowGrabAnyObjectWithSize)
    {
      const xiiVec3  halfExt = box.GetHalfExtents().CompMul(pActorComp->GetOwner()->GetGlobalScaling()) * 0.5f;
      const xiiVec3& center  = box.GetCenter();

      grabPoints.SetCount(4);
      grabPoints[0].m_vLocalPosition.Set(-halfExt.x, 0, 0);
      grabPoints[0].m_qLocalRotation.SetShortestRotation(xiiVec3::UnitXAxis(), xiiVec3::UnitXAxis());
      grabPoints[1].m_vLocalPosition.Set(+halfExt.x, 0, 0);
      grabPoints[1].m_qLocalRotation.SetShortestRotation(xiiVec3::UnitXAxis(), -xiiVec3::UnitXAxis());
      grabPoints[2].m_vLocalPosition.Set(0, -halfExt.y, 0);
      grabPoints[2].m_qLocalRotation.SetShortestRotation(xiiVec3::UnitXAxis(), xiiVec3::UnitYAxis());
      grabPoints[3].m_vLocalPosition.Set(0, +halfExt.y, 0);
      grabPoints[3].m_qLocalRotation.SetShortestRotation(xiiVec3::UnitXAxis(), -xiiVec3::UnitYAxis());
      // grabPoints[4].m_vLocalPosition.Set(0, 0, -halfExt.z);
      // grabPoints[4].m_qLocalRotation.SetShortestRotation(xiiVec3::UnitXAxis(), xiiVec3::UnitZAxis());
      // grabPoints[5].m_vLocalPosition.Set(0, 0, +halfExt.z);
      // grabPoints[5].m_qLocalRotation.SetShortestRotation(xiiVec3::UnitXAxis(), -xiiVec3::UnitZAxis());

      for (xiiUInt32 i = 0; i < grabPoints.GetCount(); ++i)
      {
        grabPoints[i].m_vLocalPosition += center;
      }
    }
  }

  if (grabPoints.IsEmpty())
    return XII_FAILURE;

  const float fMaxDistSqr = xiiMath::Square(m_fMaxGrabPointDistance);

  xiiUInt32 uiBestPointIndex = xiiInvalidIndex;
  float     fBestScore       = -1000.0f;

  for (xiiUInt32 i = 0; i < grabPoints.GetCount(); ++i)
  {
    const xiiVec3 vGrabPointPos = actorTransform.TransformPosition(grabPoints[i].m_vLocalPosition);
    const xiiQuat qGrabPointRot = actorTransform.m_qRotation * grabPoints[i].m_qLocalRotation;
    const xiiVec3 vGrabPointDir = qGrabPointRot * xiiVec3(1, 0, 0);
    const xiiVec3 vGrabPointUp  = qGrabPointRot * xiiVec3(0, 0, 1);

    const float fLenSqr = (vGrabPointPos - vAttachToPos).GetLengthSquared();

    if (fLenSqr >= fMaxDistSqr)
      continue;

    float fScore = 1.0f - fLenSqr;
    fScore += vGrabPointDir.Dot(vOwnerDir);
    fScore += vGrabPointUp.Dot(vOwnerUp) * 0.5f; // up has less weight than forward

    if (fScore > fBestScore)
    {
      uiBestPointIndex = i;
      fBestScore       = fScore;
    }
  }

  if (uiBestPointIndex >= grabPoints.GetCount())
    return XII_FAILURE;

  out_LocalGrabPoint.m_vPosition = grabPoints[uiBestPointIndex].m_vLocalPosition;
  out_LocalGrabPoint.m_qRotation = grabPoints[uiBestPointIndex].m_qLocalRotation;

  return XII_SUCCESS;
}

void xiiJoltGrabObjectComponent::CreateJoint(xiiJoltDynamicActorComponent* pParent, xiiJoltDynamicActorComponent* pChild)
{
  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  JPH::BodyID             bodies[2] = {JPH::BodyID(pParent->GetJoltBodyID()), JPH::BodyID(pChild->GetJoltBodyID())};
  JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

  auto pBody0 = bodyLock.GetBody(0);
  auto pBody1 = bodyLock.GetBody(1);

  m_fGrabbedActorMass    = pBody1->GetMotionProperties()->GetInverseMass();
  m_fGrabbedActorGravity = pBody1->GetMotionProperties()->GetGravityFactor();

  pBody1->GetMotionProperties()->SetInverseMass(10.0f);
  pBody1->GetMotionProperties()->SetGravityFactor(0.0f);

  JPH::SixDOFConstraintSettings opt;

  {
    const auto diff0 = pBody0->GetPosition() - pBody0->GetCenterOfMassPosition();
    const auto diff1 = pBody1->GetPosition() - pBody1->GetCenterOfMassPosition();

    const JPH::Quat childRot = xiiJoltConversionUtils::ToQuat(m_ChildAnchorLocal.m_qRotation);

    opt.mDrawConstraintSize = 0.1f;
    opt.mSpace              = JPH::EConstraintSpace::LocalToBodyCOM;
    opt.mPosition1          = diff0;
    opt.mPosition2          = diff1 + xiiJoltConversionUtils::ToVec3(m_ChildAnchorLocal.m_vPosition);
    opt.mAxisX1             = JPH::Vec3::sAxisX();
    opt.mAxisY1             = JPH::Vec3::sAxisY();
    opt.mAxisX2             = childRot * JPH::Vec3::sAxisX();
    opt.mAxisY2             = childRot * JPH::Vec3::sAxisY();
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationX);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationY);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationZ);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationX);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationY);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationZ);

    for (int i = 0; i < 6; ++i)
    {
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mDamping   = m_fSpringDamping;
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mFrequency = m_fSpringStiffness;
    }
  }

  // xiiTransform tAnchor = m_ChildAnchorLocal;
  // tAnchor.m_vPosition = tAnchor.m_vPosition.CompMul(pChild->GetOwner()->GetGlobalScaling());
  // pJoint->SetActors(pParent->GetOwner()->GetHandle(), xiiTransform::IdentityTransform(), pChild->GetOwner()->GetHandle(), tAnchor);

  m_pConstraint = static_cast<JPH::SixDOFConstraint*>(opt.Create(*bodyLock.GetBody(0), *bodyLock.GetBody(1)));
  m_pConstraint->AddRef();

  for (int i = 0; i < 6; ++i)
  {
    m_pConstraint->SetMotorState((JPH::SixDOFConstraint::EAxis)(JPH::SixDOFConstraint::EAxis::TranslationX + i), JPH::EMotorState::Position);
  }

  pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
}

void xiiJoltGrabObjectComponent::DetectDistanceViolation(xiiJoltDynamicActorComponent* pGrabbedActor)
{
  if (m_fBreakDistance <= 0)
    return;

  xiiGameObject* pAnchor = nullptr;
  if (!GetWorld()->TryGetObject(m_hAttachTo, pAnchor))
    return;

  const xiiVec3 vAnchorPos = pGrabbedActor->GetOwner()->GetGlobalTransform().TransformPosition(m_ChildAnchorLocal.m_vPosition);
  const xiiVec3 vJointPos  = pAnchor->GetGlobalPosition();
  const float   fDistance  = (vAnchorPos - vJointPos).GetLength();

  if (fDistance < m_fBreakDistance)
  {
    m_LastValidTime = GetWorld()->GetClock().GetAccumulatedTime();
  }
  else if (fDistance > m_fMaxGrabPointDistance * 1.1f)
  {
    BreakObjectGrab();
    return;
  }
  else
  {
    // TODO: make this configurable?
    if (GetWorld()->GetClock().GetAccumulatedTime() - m_LastValidTime > xiiTime::Seconds(1.0))
    {
      BreakObjectGrab();
      return;
    }
  }
}

bool xiiJoltGrabObjectComponent::IsCharacterStandingOnObject(xiiGameObjectHandle hActorToGrab) const
{
  const xiiJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    // TODO
    // if (pController->GetStandingOnActor() == hActorToGrab)
    //{
    //  return true;
    //}
  }

  return false;
}

void xiiJoltGrabObjectComponent::OnMsgReleaseObjectGrab(xiiMsgReleaseObjectGrab& msg)
{
  if (!msg.m_hGrabbedObjectToRelease.IsInvalidated() && !m_hGrabbedActor.IsInvalidated())
  {
    xiiComponent* pComponent;
    if (GetOwner()->GetWorld()->TryGetComponent(m_hGrabbedActor, pComponent))
    {
      if (pComponent->GetOwner()->GetHandle() == msg.m_hGrabbedObjectToRelease)
      {
        DropGrabbedObject();
      }
    }
  }
}

void xiiJoltGrabObjectComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiGameObject* pObj = GetOwner();

  while (pObj)
  {
    xiiJoltCharacterControllerComponent* pController;
    if (pObj->TryGetComponentOfBaseType(pController))
    {
      m_hCharacterControllerComponent = pController->GetHandle();
    }

    pObj = pObj->GetParent();
  }
}

void xiiJoltGrabObjectComponent::OnDeactivated()
{
  ReleaseGrabbedObject();

  SUPER::OnDeactivated();
}

void xiiJoltGrabObjectComponent::Update()
{
  if (m_pConstraint == nullptr)
    return;

  xiiJoltDynamicActorComponent* pGrabbedActor;
  if (!GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    BreakObjectGrab();
    return;
  }

  DetectDistanceViolation(pGrabbedActor);

  if (IsCharacterStandingOnObject(pGrabbedActor->GetOwner()->GetHandle()))
  {
    // disallow grabbing something again until that time
    // to prevent grabbing an object in air that we just jumped off of
    m_LastValidTime = GetWorld()->GetClock().GetAccumulatedTime() + xiiTime::Milliseconds(400);
    BreakObjectGrab();
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltGrabObjectComponent);

