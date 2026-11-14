/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <JoltPlugin/Actors/JoltKinematicActorComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltKinematicActorComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetTargetTransform, In, "Transform"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetVelocity, In, "LinearVelocity", In, "AngularVelocity"),
    XII_SCRIPT_FUNCTION_PROPERTY(Teleport, In, "Transform"),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiJoltKinematicActorComponent::xiiJoltKinematicActorComponent()  = default;
xiiJoltKinematicActorComponent::~xiiJoltKinematicActorComponent() = default;

void xiiJoltKinematicActorComponent::SetTargetTransform(const xiiTransform& transform)
{
  if (!transform.m_vPosition.IsValid() || !transform.m_qRotation.IsValid())
    return;

  m_bUseVelocity = false;
  auto* pOwner   = GetOwner();
  if (!pOwner)
    return;
  pOwner->UpdateGlobalTransform();
  auto target        = pOwner->GetGlobalTransform();
  target.m_vPosition = transform.m_vPosition;
  target.m_qRotation = transform.m_qRotation;
  pOwner->SetGlobalTransform(target);
}

void xiiJoltKinematicActorComponent::SetVelocity(const xiiVec3& vLinear, const xiiVec3& vAngular)
{
  if (!vLinear.IsValid() || !vAngular.IsValid() || !xiiMath::IsFinite(vLinear.GetLengthSquared()) || !xiiMath::IsFinite(vAngular.GetLengthSquared()))
    return;

  m_vLinearVelocity  = vLinear;
  m_vAngularVelocity = vAngular;
  m_bUseVelocity     = true;
}

void xiiJoltKinematicActorComponent::Teleport(const xiiTransform& transform)
{
  if (!transform.m_vPosition.IsValid() || !transform.m_qRotation.IsValid())
    return;

  SetTargetTransform(transform);
  m_vTeleportPosition = transform.m_vPosition;
  m_qTeleportRotation = transform.m_qRotation;
  m_bTeleportPending  = true;
}

bool xiiJoltKinematicActorComponent::TryGetBodyState(BodyState& out_state) const
{
  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  return pModule != nullptr && pModule->GetKinematicState(*this, out_state);
}

void xiiJoltKinematicActorComponent::OnDeactivated()
{
  m_bUseVelocity     = false;
  m_bTeleportPending = false;
  SUPER::OnDeactivated();
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltKinematicActorComponent);
