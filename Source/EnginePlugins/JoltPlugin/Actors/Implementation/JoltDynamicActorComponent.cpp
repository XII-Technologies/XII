/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Math.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

#include <Jolt/Jolt.h>

#include <Jolt/Physics/Body/BodyInterface.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltDynamicActorComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new xiiDefaultValueAttribute(1000.0f), new xiiClampValueAttribute(0.001f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("LinearDamping", GetLinearDamping, SetLinearDamping)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("AngularDamping", GetAngularDamping, SetAngularDamping)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("InitialLinearVelocity", GetInitialLinearVelocity, SetInitialLinearVelocity)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3::MakeZero())),
    XII_ACCESSOR_PROPERTY("InitialAngularVelocity", GetInitialAngularVelocity, SetInitialAngularVelocity)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3::MakeZero())),
    XII_ACCESSOR_PROPERTY("AllowSleeping", GetAllowSleeping, SetAllowSleeping)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("StartAsleep", GetStartAsleep, SetStartAsleep)->AddAttributes(new xiiDefaultValueAttribute(false)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltDynamicActorComponent::xiiJoltDynamicActorComponent()  = default;
xiiJoltDynamicActorComponent::~xiiJoltDynamicActorComponent() = default;

void xiiJoltDynamicActorComponent::SetMass(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue < 0.0f)
  {
    xiiLog::Warning("Invalid Jolt actor Mass.");
    return;
  }

  if (m_fMass != fValue)
  {
    m_fMass = fValue;
    QueueBodyUpdate();
  }
}

void xiiJoltDynamicActorComponent::SetDensity(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue <= 0.0f)
  {
    xiiLog::Warning("Invalid Jolt actor Density.");
    return;
  }

  if (m_fDensity != fValue)
  {
    m_fDensity = fValue;
    QueueBodyUpdate();
  }
}

void xiiJoltDynamicActorComponent::SetGravityFactor(float fValue)
{
  if (!xiiMath::IsFinite(fValue))
  {
    xiiLog::Warning("Invalid Jolt actor GravityFactor.");
    return;
  }

  if (m_fGravityFactor != fValue)
  {
    m_fGravityFactor = fValue;
    QueueBodyUpdate();
  }
}

void xiiJoltDynamicActorComponent::SetLinearDamping(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue < 0.0f)
  {
    xiiLog::Warning("Invalid Jolt actor LinearDamping.");
    return;
  }

  if (m_fLinearDamping != fValue)
  {
    m_fLinearDamping = fValue;
    QueueBodyUpdate();
  }
}

void xiiJoltDynamicActorComponent::SetAngularDamping(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue < 0.0f)
  {
    xiiLog::Warning("Invalid Jolt actor AngularDamping.");
    return;
  }

  if (m_fAngularDamping != fValue)
  {
    m_fAngularDamping = fValue;
    QueueBodyUpdate();
  }
}

void xiiJoltDynamicActorComponent::SetAllowSleeping(bool bAllowSleeping)
{
  if (m_bAllowSleeping != bAllowSleeping)
  {
    m_bAllowSleeping = bAllowSleeping;
    QueueBodyUpdate();
  }
}

void xiiJoltDynamicActorComponent::SetInitialLinearVelocity(const xiiVec3& vVelocity)
{
  if (vVelocity.IsValid())
    m_vInitialLinearVelocity = vVelocity;
  else
    xiiLog::Warning("Jolt initial linear velocity must be finite.");
}

void xiiJoltDynamicActorComponent::SetInitialAngularVelocity(const xiiVec3& vVelocity)
{
  if (vVelocity.IsValid())
    m_vInitialAngularVelocity = vVelocity;
  else
    xiiLog::Warning("Jolt initial angular velocity must be finite.");
}

bool xiiJoltDynamicActorComponent::TryGetBodyState(BodyState& out_state) const
{
  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  const auto* pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  if (pBodies == nullptr)
    return false;

  JPH::RVec3 position;
  JPH::Quat  rotation;
  JPH::Vec3  linearVelocity, angularVelocity;
  pBodies->GetPositionAndRotation(bodyId, position, rotation);
  pBodies->GetLinearAndAngularVelocity(bodyId, linearVelocity, angularVelocity);
  out_state.m_vPosition        = xiiVec3(static_cast<float>(position.GetX()), static_cast<float>(position.GetY()), static_cast<float>(position.GetZ()));
  out_state.m_qRotation        = xiiQuat(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
  out_state.m_vLinearVelocity  = xiiVec3(linearVelocity.GetX(), linearVelocity.GetY(), linearVelocity.GetZ());
  out_state.m_vAngularVelocity = xiiVec3(angularVelocity.GetX(), angularVelocity.GetY(), angularVelocity.GetZ());
  out_state.m_bActive          = pBodies->IsActive(bodyId);
  return true;
}

xiiResult xiiJoltDynamicActorComponent::SetLinearVelocity(const xiiVec3& vVelocity)
{
  if (!vVelocity.IsValid())
    return XII_FAILURE;

  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  auto*       pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  if (pBodies == nullptr)
    return XII_FAILURE;

  pBodies->SetLinearVelocity(bodyId, JPH::Vec3(vVelocity.x, vVelocity.y, vVelocity.z));
  return XII_SUCCESS;
}

xiiResult xiiJoltDynamicActorComponent::SetAngularVelocity(const xiiVec3& vVelocity)
{
  if (!vVelocity.IsValid())
    return XII_FAILURE;

  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  auto*       pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  if (pBodies == nullptr)
    return XII_FAILURE;

  pBodies->SetAngularVelocity(bodyId, JPH::Vec3(vVelocity.x, vVelocity.y, vVelocity.z));
  return XII_SUCCESS;
}

xiiResult xiiJoltDynamicActorComponent::ActivateBody()
{
  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  auto*       pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  if (pBodies == nullptr)
    return XII_FAILURE;

  pBodies->ActivateBody(bodyId);
  return XII_SUCCESS;
}

xiiResult xiiJoltDynamicActorComponent::AddForce(const xiiVec3& vForce)
{
  if (!vForce.IsValid())
    return XII_FAILURE;

  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  auto*       pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  if (pBodies == nullptr)
    return XII_FAILURE;

  pBodies->AddForce(bodyId, JPH::Vec3(vForce.x, vForce.y, vForce.z));
  return XII_SUCCESS;
}

xiiResult xiiJoltDynamicActorComponent::AddImpulse(const xiiVec3& vImpulse)
{
  if (!vImpulse.IsValid())
    return XII_FAILURE;

  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  auto*       pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  if (pBodies == nullptr)
    return XII_FAILURE;

  pBodies->AddImpulse(bodyId, JPH::Vec3(vImpulse.x, vImpulse.y, vImpulse.z));
  return XII_SUCCESS;
}

bool xiiJoltDynamicActorComponent::IsBodyActive() const
{
  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  JPH::BodyID bodyId;
  const auto* pBodies = pModule != nullptr ? pModule->FindDynamicBody(GetHandle(), bodyId) : nullptr;
  return pBodies != nullptr && pBodies->IsActive(bodyId);
}

void xiiJoltDynamicActorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& stream = inout_stream.GetStream();
  stream << m_fMass << m_fDensity << m_fGravityFactor << m_fLinearDamping << m_fAngularDamping;
  stream << m_vInitialLinearVelocity << m_vInitialAngularVelocity << m_bAllowSleeping << m_bStartAsleep;
}

void xiiJoltDynamicActorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&   stream = inout_stream.GetStream();
  float   fMass = m_fMass, fDensity = m_fDensity, fGravityFactor = m_fGravityFactor;
  float   fLinearDamping = m_fLinearDamping, fAngularDamping = m_fAngularDamping;
  xiiVec3 vLinearVelocity = m_vInitialLinearVelocity, vAngularVelocity = m_vInitialAngularVelocity;
  stream >> fMass >> fDensity >> fGravityFactor >> fLinearDamping >> fAngularDamping;
  stream >> vLinearVelocity >> vAngularVelocity >> m_bAllowSleeping >> m_bStartAsleep;
  SetMass(fMass);
  SetDensity(fDensity);
  SetGravityFactor(fGravityFactor);
  SetLinearDamping(fLinearDamping);
  SetAngularDamping(fAngularDamping);
  SetInitialLinearVelocity(vLinearVelocity);
  SetInitialAngularVelocity(vAngularVelocity);
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltDynamicActorComponent);
