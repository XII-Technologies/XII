/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltCharacterGroundState, 1)
  XII_ENUM_CONSTANTS(xiiJoltCharacterGroundState::OnGround, xiiJoltCharacterGroundState::OnSteepGround, xiiJoltCharacterGroundState::NotSupported, xiiJoltCharacterGroundState::InAir)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiJoltCharacterControllerComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(0.35f), new xiiClampValueAttribute(0.01f, 1000.0f)),
    XII_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new xiiDefaultValueAttribute(1.8f), new xiiClampValueAttribute(0.02f, 2000.0f)),
    XII_ACCESSOR_PROPERTY("CollisionLayer", GetCollisionLayer, SetCollisionLayer)->AddAttributes(new xiiDefaultValueAttribute(0), new xiiDynamicEnumAttribute("JoltCollisionLayer")),
    XII_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new xiiDefaultValueAttribute(70.0f), new xiiClampValueAttribute(0.0f, 100000.0f)),
    XII_ACCESSOR_PROPERTY("MaxStrength", GetMaxStrength, SetMaxStrength)->AddAttributes(new xiiDefaultValueAttribute(100.0f), new xiiClampValueAttribute(0.0f, 1000000.0f)),
    XII_ACCESSOR_PROPERTY("MaxSlope", GetMaxSlope, SetMaxSlope)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(50.0f))),
    XII_ACCESSOR_PROPERTY("StepUpHeight", GetStepUpHeight, SetStepUpHeight)->AddAttributes(new xiiDefaultValueAttribute(0.35f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_ACCESSOR_PROPERTY("StepDownDistance", GetStepDownDistance, SetStepDownDistance)->AddAttributes(new xiiDefaultValueAttribute(0.35f), new xiiClampValueAttribute(0.0f, 10.0f)),
    XII_ACCESSOR_PROPERTY("PenetrationRecoverySpeed", GetPenetrationRecoverySpeed, SetPenetrationRecoverySpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_ACCESSOR_PROPERTY("ContactTolerance", GetContactTolerance, SetContactTolerance)->AddAttributes(new xiiDefaultValueAttribute(0.001f), new xiiClampValueAttribute(0.00001f, 0.1f)),
    XII_ACCESSOR_PROPERTY("ContactPadding", GetContactPadding, SetContactPadding)->AddAttributes(new xiiDefaultValueAttribute(0.02f), new xiiClampValueAttribute(0.0001f, 1.0f)),
    XII_ACCESSOR_PROPERTY("MaxCollisionIterations", GetMaxCollisionIterations, SetMaxCollisionIterations)->AddAttributes(new xiiDefaultValueAttribute(5U), new xiiClampValueAttribute(1U, 32U)),
    XII_ACCESSOR_PROPERTY("GravityFactor", GetGravityFactor, SetGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_ACCESSOR_PROPERTY("SimulationEnabled", GetSimulationEnabled, SetSimulationEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetMovementVelocity, In, "Velocity"),
    XII_SCRIPT_FUNCTION_PROPERTY(SetLocalMovementVelocity, In, "Velocity"),
    XII_SCRIPT_FUNCTION_PROPERTY(Jump, In, "Speed"),
    XII_SCRIPT_FUNCTION_PROPERTY(StopMovement),
    XII_SCRIPT_FUNCTION_PROPERTY(Teleport, In, "Position"),
    XII_SCRIPT_FUNCTION_PROPERTY(IsGrounded),
    XII_SCRIPT_FUNCTION_PROPERTY(GetPosition),
    XII_SCRIPT_FUNCTION_PROPERTY(GetVelocity),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGroundNormal),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGroundVelocity),
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

xiiJoltCharacterControllerComponent::xiiJoltCharacterControllerComponent()  = default;
xiiJoltCharacterControllerComponent::~xiiJoltCharacterControllerComponent() = default;

void xiiJoltCharacterControllerComponent::SetRadius(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value = xiiMath::Clamp(value, 0.01f, 1000.0f);
  if (m_fRadius == value)
    return;
  m_fRadius = value;
  QueueCharacterUpdate();
}

void xiiJoltCharacterControllerComponent::SetHeight(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value = xiiMath::Clamp(value, 0.02f, 2000.0f);
  if (m_fHeight == value)
    return;
  m_fHeight = value;
  QueueCharacterUpdate();
}

void xiiJoltCharacterControllerComponent::SetCollisionLayer(xiiUInt8 value)
{
  if (m_uiCollisionLayer == value)
    return;
  m_uiCollisionLayer = value;
  QueueCharacterUpdate();
}

void xiiJoltCharacterControllerComponent::SetMass(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value   = xiiMath::Clamp(value, 0.0f, 100000.0f);
  m_fMass = value;
}

void xiiJoltCharacterControllerComponent::SetMaxStrength(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value          = xiiMath::Clamp(value, 0.0f, 1000000.0f);
  m_fMaxStrength = value;
}

void xiiJoltCharacterControllerComponent::SetMaxSlope(xiiAngle value)
{
  if (!xiiMath::IsFinite(value.GetRadian()))
    return;
  value      = xiiAngle::MakeFromDegree(xiiMath::Clamp(value.GetDegree(), 0.1f, 89.9f));
  m_MaxSlope = value;
}

void xiiJoltCharacterControllerComponent::SetStepUpHeight(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value           = xiiMath::Clamp(value, 0.0f, 10.0f);
  m_fStepUpHeight = value;
}

void xiiJoltCharacterControllerComponent::SetStepDownDistance(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value               = xiiMath::Clamp(value, 0.0f, 10.0f);
  m_fStepDownDistance = value;
}

void xiiJoltCharacterControllerComponent::SetPenetrationRecoverySpeed(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value                       = xiiMath::Clamp(value, 0.0f, 1.0f);
  m_fPenetrationRecoverySpeed = value;
}

void xiiJoltCharacterControllerComponent::SetContactTolerance(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value = xiiMath::Clamp(value, 0.00001f, 0.1f);
  if (m_fContactTolerance == value)
    return;
  m_fContactTolerance = value;
  QueueCharacterUpdate();
}

void xiiJoltCharacterControllerComponent::SetContactPadding(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value = xiiMath::Clamp(value, 0.0001f, 1.0f);
  if (m_fContactPadding == value)
    return;
  m_fContactPadding = value;
  QueueCharacterUpdate();
}

void xiiJoltCharacterControllerComponent::SetMaxCollisionIterations(xiiUInt32 value)
{
  value = xiiMath::Clamp(value, 1U, 32U);
  if (m_uiMaxCollisionIterations == value)
    return;
  m_uiMaxCollisionIterations = value;
  QueueCharacterUpdate();
}

void xiiJoltCharacterControllerComponent::SetGravityFactor(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  value            = xiiMath::Clamp(value, 0.0f, 100.0f);
  m_fGravityFactor = value;
}

void xiiJoltCharacterControllerComponent::SetMovementVelocity(const xiiVec3& vVelocity)
{
  if (vVelocity.IsValid() && xiiMath::IsFinite(vVelocity.GetLengthSquared()))
    m_vDesiredVelocity.Set(vVelocity.x, vVelocity.y, 0.0f);
}

void xiiJoltCharacterControllerComponent::SetLocalMovementVelocity(const xiiVec3& vVelocity)
{
  if (!vVelocity.IsValid() || !GetOwner())
    return;
  GetOwner()->UpdateGlobalTransform();
  SetMovementVelocity(GetOwner()->GetGlobalRotation() * vVelocity);
}

void xiiJoltCharacterControllerComponent::Jump(float fSpeed)
{
  if (xiiMath::IsFinite(fSpeed * fSpeed) && fSpeed > 0.0f && m_bSimulationEnabled)
    m_fJumpSpeed = fSpeed;
}

void xiiJoltCharacterControllerComponent::StopMovement()
{
  m_vDesiredVelocity.SetZero();
  m_fJumpSpeed = 0.0f;
}

void xiiJoltCharacterControllerComponent::Teleport(const xiiVec3& vPosition)
{
  if (!vPosition.IsValid())
    return;

  m_vTeleportPosition = vPosition;
  m_bTeleportPending  = true;
  m_fJumpSpeed        = 0.0f;
  if (auto* pOwner = GetOwner())
  {
    pOwner->UpdateGlobalTransform();
    pOwner->SetGlobalPosition(vPosition);
  }
}

void xiiJoltCharacterControllerComponent::SetSimulationEnabled(bool bEnabled)
{
  if (m_bSimulationEnabled == bEnabled)
    return;
  m_bSimulationEnabled = bEnabled;
  m_fJumpSpeed         = 0.0f;
  QueueCharacterUpdate();
}

bool xiiJoltCharacterControllerComponent::TryGetState(State& out_state) const
{
  if (!m_bSimulationEnabled || m_bTeleportPending)
    return false;

  const auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>();
  return pModule != nullptr && pModule->GetCharacterState(GetHandle(), out_state);
}

bool xiiJoltCharacterControllerComponent::IsGrounded() const
{
  return GetGroundState() == xiiJoltCharacterGroundState::OnGround;
}

xiiJoltCharacterGroundState::Enum xiiJoltCharacterControllerComponent::GetGroundState() const
{
  State state;
  return TryGetState(state) ? static_cast<xiiJoltCharacterGroundState::Enum>(state.m_GroundState.GetValue()) : xiiJoltCharacterGroundState::InAir;
}

xiiVec3 xiiJoltCharacterControllerComponent::GetVelocity() const
{
  State state;
  return TryGetState(state) ? state.m_vVelocity : xiiVec3::MakeZero();
}

xiiVec3 xiiJoltCharacterControllerComponent::GetPosition() const
{
  State state;
  if (TryGetState(state))
    return state.m_vPosition;
  return GetOwner() ? GetOwner()->GetGlobalPosition() : xiiVec3::MakeZero();
}

xiiVec3 xiiJoltCharacterControllerComponent::GetGroundNormal() const
{
  State state;
  return TryGetState(state) ? state.m_vGroundNormal : xiiVec3::MakeZero();
}

xiiVec3 xiiJoltCharacterControllerComponent::GetGroundVelocity() const
{
  State state;
  return TryGetState(state) ? state.m_vGroundVelocity : xiiVec3::MakeZero();
}

void xiiJoltCharacterControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->QueueCharacterUpdate(GetHandle());
}

void xiiJoltCharacterControllerComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
    pModule->RemoveCharacter(GetHandle());
  StopMovement();
  m_bTeleportPending = false;
  SUPER::OnDeactivated();
}

void xiiJoltCharacterControllerComponent::QueueCharacterUpdate()
{
  if (IsActiveAndSimulating())
    GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->QueueCharacterUpdate(GetHandle());
}

void xiiJoltCharacterControllerComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& stream = inout_stream.GetStream();
  stream << m_fRadius << m_fHeight << m_uiCollisionLayer << m_fMass << m_fMaxStrength << m_MaxSlope << m_fStepUpHeight << m_fStepDownDistance << m_fPenetrationRecoverySpeed << m_fContactTolerance << m_fContactPadding << m_uiMaxCollisionIterations << m_fGravityFactor << m_bSimulationEnabled;
}

void xiiJoltCharacterControllerComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto&     stream                    = inout_stream.GetStream();
  float     fRadius                   = m_fRadius;
  float     fHeight                   = m_fHeight;
  xiiUInt8  uiCollisionLayer          = m_uiCollisionLayer;
  float     fMass                     = m_fMass;
  float     fMaxStrength              = m_fMaxStrength;
  xiiAngle  MaxSlope                  = m_MaxSlope;
  float     fStepUpHeight             = m_fStepUpHeight;
  float     fStepDownDistance         = m_fStepDownDistance;
  float     fPenetrationRecoverySpeed = m_fPenetrationRecoverySpeed;
  float     fContactTolerance         = m_fContactTolerance;
  float     fContactPadding           = m_fContactPadding;
  xiiUInt32 uiMaxCollisionIterations  = m_uiMaxCollisionIterations;
  float     fGravityFactor            = m_fGravityFactor;
  stream >> fRadius >> fHeight >> uiCollisionLayer >> fMass >> fMaxStrength >> MaxSlope >> fStepUpHeight >> fStepDownDistance >> fPenetrationRecoverySpeed >> fContactTolerance >> fContactPadding >> uiMaxCollisionIterations >> fGravityFactor >> m_bSimulationEnabled;
  SetRadius(fRadius);
  SetHeight(fHeight);
  SetCollisionLayer(uiCollisionLayer);
  SetMass(fMass);
  SetMaxStrength(fMaxStrength);
  SetMaxSlope(MaxSlope);
  SetStepUpHeight(fStepUpHeight);
  SetStepDownDistance(fStepDownDistance);
  SetPenetrationRecoverySpeed(fPenetrationRecoverySpeed);
  SetContactTolerance(fContactTolerance);
  SetContactPadding(fContactPadding);
  SetMaxCollisionIterations(uiMaxCollisionIterations);
  SetGravityFactor(fGravityFactor);
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltCharacterControllerComponent);
