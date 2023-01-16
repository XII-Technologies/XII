#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltConstraintComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    //XII_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce),
    //XII_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque),
    XII_ACCESSOR_PROPERTY("PairCollision", GetPairCollision, SetPairCollision)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("ParentActor", DummyGetter, SetParentActorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("ChildActor", DummyGetter, SetChildActorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("ChildActorAnchor", DummyGetter, SetChildActorAnchorReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Constraints"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltConstraintLimitMode, 1)
  XII_ENUM_CONSTANTS(xiiJoltConstraintLimitMode::NoLimit, xiiJoltConstraintLimitMode::HardLimit/*, xiiJoltConstraintLimitMode::SoftLimit*/)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltConstraintDriveMode, 1)
  XII_ENUM_CONSTANTS(xiiJoltConstraintDriveMode::NoDrive, xiiJoltConstraintDriveMode::DriveVelocity, xiiJoltConstraintDriveMode::DrivePosition)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiJoltConstraintComponent::xiiJoltConstraintComponent()  = default;
xiiJoltConstraintComponent::~xiiJoltConstraintComponent() = default;

// void xiiJoltConstraintComponent::SetBreakForce(float value)
//{
//   m_fBreakForce = value;
//   QueueApplySettings();
// }
//
// void xiiJoltConstraintComponent::SetBreakTorque(float value)
//{
//   m_fBreakTorque = value;
//   QueueApplySettings();
// }

void xiiJoltConstraintComponent::SetPairCollision(bool value)
{
  m_bPairCollision = value;
  QueueApplySettings();
}

void xiiJoltConstraintComponent::OnSimulationStarted()
{
  xiiUInt32 uiBodyIdA = xiiInvalidIndex;
  xiiUInt32 uiBodyIdB = xiiInvalidIndex;

  if (FindParentBody(uiBodyIdA).Failed())
    return;

  if (FindChildBody(uiBodyIdB).Failed())
    return;

  if (uiBodyIdB == xiiInvalidIndex)
    return;

  if (uiBodyIdA == uiBodyIdB)
  {
    xiiLog::Error("Constraint can't be linked to the same body twice");
    return;
  }

  m_LocalFrameA.m_qRotation.Normalize();
  m_LocalFrameB.m_qRotation.Normalize();

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();

  {
    JPH::BodyID             bodies[2] = {JPH::BodyID(uiBodyIdA), JPH::BodyID(uiBodyIdB)};
    JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

    if (uiBodyIdB != xiiInvalidIndex && bodyLock.GetBody(1) != nullptr)
    {
      if (uiBodyIdA != xiiInvalidIndex && bodyLock.GetBody(0) != nullptr)
      {
        CreateContstraintType(bodyLock.GetBody(0), bodyLock.GetBody(1));

        pModule->EnableJoinedBodiesCollisions(bodyLock.GetBody(0)->GetCollisionGroup().GetGroupID(), bodyLock.GetBody(1)->GetCollisionGroup().GetGroupID(), m_bPairCollision);
      }
      else
      {
        CreateContstraintType(&JPH::Body::sFixedToWorld, bodyLock.GetBody(1));
      }
    }
  }

  if (m_pConstraint)
  {
    m_pConstraint->AddRef();
    pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
    ApplySettings();
  }
}

void xiiJoltConstraintComponent::OnDeactivated()
{
  if (m_pConstraint != nullptr)
  {
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

    // pModule->m_BreakableConstraints.Remove(m_pConstraint->getConstraint());

    m_pConstraint->Release();
    m_pConstraint = nullptr;
  }

  SUPER::OnDeactivated();
}

void xiiJoltConstraintComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // s << m_fBreakForce;
  // s << m_fBreakTorque;
  s << m_bPairCollision;

  stream.WriteGameObjectHandle(m_hActorA);
  stream.WriteGameObjectHandle(m_hActorB);

  s << m_LocalFrameA;
  s << m_LocalFrameB;

  stream.WriteGameObjectHandle(m_hActorBAnchor);
}

void xiiJoltConstraintComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = stream.GetStream();

  // s >> m_fBreakForce;
  // s >> m_fBreakTorque;
  s >> m_bPairCollision;

  m_hActorA = stream.ReadGameObjectHandle();
  m_hActorB = stream.ReadGameObjectHandle();

  s >> m_LocalFrameA;
  s >> m_LocalFrameB;

  m_hActorBAnchor = stream.ReadGameObjectHandle();
}

void xiiJoltConstraintComponent::SetParentActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetParentActor(resolver(szReference, GetHandle(), "ParentActor"));
}

void xiiJoltConstraintComponent::SetChildActorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetChildActor(resolver(szReference, GetHandle(), "ChildActor"));
}

void xiiJoltConstraintComponent::SetChildActorAnchorReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = resolver(szReference, GetHandle(), "ChildActorAnchor");
}

void xiiJoltConstraintComponent::SetParentActor(xiiGameObjectHandle hActor)
{
  SetUserFlag(0, false); // local frame A is not valid
  m_hActorA = hActor;
}

void xiiJoltConstraintComponent::SetChildActor(xiiGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorB = hActor;
}

void xiiJoltConstraintComponent::SetChildActorAnchor(xiiGameObjectHandle hActor)
{
  SetUserFlag(1, false); // local frame B is not valid
  m_hActorBAnchor = hActor;
}

void xiiJoltConstraintComponent::SetActors(xiiGameObjectHandle hActorA, const xiiTransform& localFrameA, xiiGameObjectHandle hActorB, const xiiTransform& localFrameB)
{
  m_hActorA = hActorA;
  m_hActorB = hActorB;

  // prevent FindParentBody() and FindChildBody() from overwriting the local frames
  // local frame A and B are already valid
  SetUserFlag(0, true);
  SetUserFlag(1, true);

  m_LocalFrameA = localFrameA;
  m_LocalFrameB = localFrameB;
}

void xiiJoltConstraintComponent::ApplySettings()
{
  SetUserFlag(2, false);

  // const float fBreakForce = m_fBreakForce <= 0.0f ? xiiMath::MaxValue<float>() : m_fBreakForce;
  // const float fBreakTorque = m_fBreakTorque <= 0.0f ? xiiMath::MaxValue<float>() : m_fBreakTorque;
  // m_pConstraint->setBreakForce(fBreakForce, fBreakTorque);

  // if (m_fBreakForce > 0.0f || m_fBreakTorque > 0.0f)
  //{
  //   xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  //   pModule->m_BreakableConstraints[m_pConstraint->getConstraint()] = GetHandle();
  // }

  // m_pConstraint->setConstraintFlag(PxConstraintFlag::eCOLLISION_ENABLED, m_bPairCollision);

  // JoltRigidActor* pActor0 = nullptr;
  // JoltRigidActor* pActor1 = nullptr;
  // m_pConstraint->getActors(pActor0, pActor1);

  // if (pActor0 && pActor0->is<PxRigidDynamic>() && !static_cast<PxRigidDynamic*>(pActor0)->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
  //   static_cast<PxRigidDynamic*>(pActor0)->wakeUp();
  // if (pActor1 && pActor1->is<PxRigidDynamic>() && !static_cast<PxRigidDynamic*>(pActor1)->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC))
  //   static_cast<PxRigidDynamic*>(pActor1)->wakeUp();
}

xiiResult xiiJoltConstraintComponent::FindParentBody(xiiUInt32& out_uiJoltBodyID)
{
  xiiGameObject*                pObject = nullptr;
  xiiJoltDynamicActorComponent* pRbComp = nullptr;

  if (!m_hActorA.IsInvalidated())
  {
    if (!GetWorld()->TryGetObject(m_hActorA, pObject) || !pObject->IsActive())
    {
      xiiLog::Error("{0} '{1}' parent reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
      return XII_FAILURE;
    }

    if (!pObject->TryGetComponentOfBaseType(pRbComp))
    {
      xiiLog::Error("{0} '{1}' parent reference is an object without a xiiJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
                    GetOwner()->GetName());
      return XII_FAILURE;
    }
  }
  else
  {
    pObject = GetOwner();

    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pRbComp))
        break;

      pObject = pObject->GetParent();
    }

    if (pRbComp == nullptr)
    {
      out_uiJoltBodyID = xiiInvalidIndex;

      if (GetUserFlag(0) == false)
      {
        // m_localFrameA is now valid
        SetUserFlag(0, true);
        m_LocalFrameA = GetOwner()->GetGlobalTransform();
      }
      return XII_SUCCESS;
    }
    else
    {
      if (GetUserFlag(0) == true)
      {
        xiiTransform globalFrame = m_LocalFrameA;

        // m_localFrameA is already valid
        // assume it was in global space and move it into local space of the found parent
        m_LocalFrameA.SetLocalTransform(pRbComp->GetOwner()->GetGlobalTransform(), globalFrame);
        m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
      }
    }
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == xiiInvalidIndex)
  {
    xiiLog::Error("{0} '{1}' parent reference is an object with an invalid xiiJoltDynamicActorComponent. Constraint is ignored.",
                  GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return XII_FAILURE;
  }

  m_hActorA = pObject->GetHandle();

  if (GetUserFlag(0) == false)
  {
    // m_localFrameA is now valid
    SetUserFlag(0, true);
    m_LocalFrameA.SetLocalTransform(pObject->GetGlobalTransform(), GetOwner()->GetGlobalTransform());
    m_LocalFrameA.m_vPosition = m_LocalFrameA.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return XII_SUCCESS;
}

xiiResult xiiJoltConstraintComponent::FindChildBody(xiiUInt32& out_uiJoltBodyID)
{
  xiiGameObject*                pObject = nullptr;
  xiiJoltDynamicActorComponent* pRbComp = nullptr;

  if (m_hActorB.IsInvalidated())
  {
    xiiLog::Error("{0} '{1}' has no child reference. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return XII_FAILURE;
  }

  if (!GetWorld()->TryGetObject(m_hActorB, pObject) || !pObject->IsActive())
  {
    xiiLog::Error("{0} '{1}' child reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return XII_FAILURE;
  }

  if (!pObject->TryGetComponentOfBaseType(pRbComp))
  {
    // this makes it possible to link the Constraint to a prefab, because it may skip the top level hierarchy of the prefab
    pObject = pObject->SearchForChildByNameSequence("/", xiiGetStaticRTTI<xiiJoltDynamicActorComponent>());

    if (pObject == nullptr)
    {
      xiiLog::Error("{0} '{1}' child reference is an object without a xiiJoltDynamicActorComponent. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(),
                    GetOwner()->GetName());
      return XII_FAILURE;
    }

    pObject->TryGetComponentOfBaseType(pRbComp);
  }

  pRbComp->EnsureSimulationStarted();
  out_uiJoltBodyID = pRbComp->GetJoltBodyID();

  if (out_uiJoltBodyID == xiiInvalidIndex)
  {
    xiiLog::Error("{0} '{1}' child reference is an object with an invalid xiiJoltDynamicActorComponent. Constraint is ignored.",
                  GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
    return XII_FAILURE;
  }

  m_hActorB = pObject->GetHandle();

  if (GetUserFlag(1) == false)
  {
    xiiGameObject* pAnchorObject = GetOwner();

    if (!m_hActorBAnchor.IsInvalidated())
    {
      if (!GetWorld()->TryGetObject(m_hActorBAnchor, pAnchorObject))
      {
        xiiLog::Error("{0} '{1}' anchor reference is a non-existing object. Constraint is ignored.", GetDynamicRTTI()->GetTypeName(), GetOwner()->GetName());
        return XII_FAILURE;
      }
    }

    // m_localFrameB is now valid
    SetUserFlag(1, true);
    m_LocalFrameB.SetLocalTransform(pObject->GetGlobalTransform(), pAnchorObject->GetGlobalTransform());
    m_LocalFrameB.m_vPosition = m_LocalFrameB.m_vPosition.CompMul(pObject->GetGlobalScaling());
  }

  return XII_SUCCESS;
}

xiiTransform xiiJoltConstraintComponent::ComputeParentBodyGlobalFrame() const
{
  if (!m_hActorA.IsInvalidated())
  {
    const xiiGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorA, pObject))
    {
      xiiTransform res;
      res.SetGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameA);
      return res;
    }
  }

  return m_LocalFrameA;
}

xiiTransform xiiJoltConstraintComponent::ComputeChildBodyGlobalFrame() const
{
  if (!m_hActorB.IsInvalidated())
  {
    const xiiGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hActorB, pObject))
    {
      xiiTransform res;
      res.SetGlobalTransform(pObject->GetGlobalTransform(), m_LocalFrameB);
      return res;
    }
  }

  return m_LocalFrameB;
}

void xiiJoltConstraintComponent::QueueApplySettings()
{
  if (m_pConstraint == nullptr)
    return;

  // already in queue ?
  if (GetUserFlag(2))
    return;

  SetUserFlag(2, true);

  xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
  pModule->m_RequireUpdate.PushBack(GetHandle());
}
