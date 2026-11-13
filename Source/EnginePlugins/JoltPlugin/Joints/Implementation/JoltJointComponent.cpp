/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Math.h>
#include <JoltPlugin/Joints/JoltJointComponent.h>
#include <JoltPlugin/Shapes/JoltAuthoringAttributes.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiJoltJointMotorMode, 1)
  XII_ENUM_CONSTANTS(xiiJoltJointMotorMode::Off, xiiJoltJointMotorMode::Velocity, xiiJoltJointMotorMode::Position)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiJoltJointComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ActorA", GetActorReference, SetActorAReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("ActorB", GetActorReference, SetActorBReference)->AddAttributes(new xiiGameObjectReferenceAttribute()),
    XII_ACCESSOR_PROPERTY("LocalFrameA", GetLocalFrameA, SetLocalFrameA),
    XII_ACCESSOR_PROPERTY("LocalFrameB", GetLocalFrameB, SetLocalFrameB),
    XII_ACCESSOR_PROPERTY("BreakForce", GetBreakForce, SetBreakForce)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
}
XII_END_ABSTRACT_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltFixedJointComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Joints"),
    new xiiJoltJointManipulatorAttribute("LocalFrameA", "ActorA"),
    new xiiJoltJointManipulatorAttribute("LocalFrameB", "ActorB"),
    new xiiJoltVisualizerAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltPointJointComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Joints"),
    new xiiJoltJointManipulatorAttribute("LocalFrameA", "ActorA"),
    new xiiJoltJointManipulatorAttribute("LocalFrameB", "ActorB"),
    new xiiJoltVisualizerAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltHingeJointComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("LimitsEnabled", GetLimitsEnabled, SetLimitsEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimit, SetLowerLimit)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(-90.0f))),
    XII_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimit, SetUpperLimit)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(90.0f))),
    XII_ENUM_ACCESSOR_PROPERTY("MotorMode", xiiJoltJointMotorMode, GetMotorMode, SetMotorMode)->AddAttributes(new xiiDefaultValueAttribute(static_cast<xiiInt32>(xiiJoltJointMotorMode::Off))),
    XII_ACCESSOR_PROPERTY("TargetVelocity", GetTargetVelocity, SetTargetVelocity)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_ACCESSOR_PROPERTY("TargetAngle", GetTargetAngle, SetTargetAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromRadian(0.0f))),
    XII_ACCESSOR_PROPERTY("MaxMotorTorque", GetMaxMotorTorque, SetMaxMotorTorque)->AddAttributes(new xiiDefaultValueAttribute(100.0f)),
    XII_ACCESSOR_PROPERTY("MotorFrequency", GetMotorFrequency, SetMotorFrequency)->AddAttributes(new xiiDefaultValueAttribute(2.0f)),
    XII_ACCESSOR_PROPERTY("MotorDamping", GetMotorDamping, SetMotorDamping)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Joints"),
    new xiiJoltJointManipulatorAttribute("LocalFrameA", "ActorA"),
    new xiiJoltJointManipulatorAttribute("LocalFrameB", "ActorB"),
    new xiiJoltVisualizerAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltSliderJointComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BreakTorque", GetBreakTorque, SetBreakTorque)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("LimitsEnabled", GetLimitsEnabled, SetLimitsEnabled)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimit, SetLowerLimit)->AddAttributes(new xiiDefaultValueAttribute(-1.0f)),
    XII_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimit, SetUpperLimit)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ENUM_ACCESSOR_PROPERTY("MotorMode", xiiJoltJointMotorMode, GetMotorMode, SetMotorMode)->AddAttributes(new xiiDefaultValueAttribute(static_cast<xiiInt32>(xiiJoltJointMotorMode::Off))),
    XII_ACCESSOR_PROPERTY("TargetVelocity", GetTargetVelocity, SetTargetVelocity)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_ACCESSOR_PROPERTY("TargetPosition", GetTargetPosition, SetTargetPosition)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_ACCESSOR_PROPERTY("MaxMotorForce", GetMaxMotorForce, SetMaxMotorForce)->AddAttributes(new xiiDefaultValueAttribute(100.0f)),
    XII_ACCESSOR_PROPERTY("MotorFrequency", GetMotorFrequency, SetMotorFrequency)->AddAttributes(new xiiDefaultValueAttribute(2.0f)),
    XII_ACCESSOR_PROPERTY("MotorDamping", GetMotorDamping, SetMotorDamping)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Joints"),
    new xiiJoltJointManipulatorAttribute("LocalFrameA", "ActorA"),
    new xiiJoltJointManipulatorAttribute("LocalFrameB", "ActorB"),
    new xiiJoltVisualizerAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiJoltDistanceJointComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MinDistance", GetMinDistance, SetMinDistance)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("MaxDistance", GetMaxDistance, SetMaxDistance)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("SpringFrequency", GetSpringFrequency, SetSpringFrequency)->AddAttributes(new xiiDefaultValueAttribute(0.0f)),
    XII_ACCESSOR_PROPERTY("SpringDamping", GetSpringDamping, SetSpringDamping)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/Jolt/Joints"),
    new xiiJoltJointManipulatorAttribute("LocalFrameA", "ActorA"),
    new xiiJoltJointManipulatorAttribute("LocalFrameB", "ActorB"),
    new xiiJoltVisualizerAttribute(),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

void xiiJoltJointComponent::SetJointConfig(const xiiPhysicsWorldModuleInterface::JointConfig& config)
{
  if (m_Config.m_hActorA == config.m_hActorA && m_Config.m_hActorB == config.m_hActorB &&
      m_Config.m_LocalFrameA == config.m_LocalFrameA && m_Config.m_LocalFrameB == config.m_LocalFrameB)
    return;
  m_Config = config;
  QueueSettingsUpdate(true);
}
void xiiJoltJointComponent::SetActorA(xiiGameObjectHandle hActor)
{
  if (m_Config.m_hActorA == hActor)
    return;
  m_Config.m_hActorA = hActor;
  QueueSettingsUpdate(true);
}
void xiiJoltJointComponent::SetActorB(xiiGameObjectHandle hActor)
{
  if (m_Config.m_hActorB == hActor)
    return;
  m_Config.m_hActorB = hActor;
  QueueSettingsUpdate(true);
}
void xiiJoltJointComponent::SetActorAReference(const char* szReference)
{
  const auto& resolver = GetWorld()->GetGameObjectReferenceResolver();
  if (resolver.IsValid())
    SetActorA(resolver(szReference, GetHandle(), "ActorA"));
}
void xiiJoltJointComponent::SetActorBReference(const char* szReference)
{
  const auto& resolver = GetWorld()->GetGameObjectReferenceResolver();
  if (resolver.IsValid())
    SetActorB(resolver(szReference, GetHandle(), "ActorB"));
}
void xiiJoltJointComponent::SetLocalFrameA(const xiiTransform& frame)
{
  if (m_Config.m_LocalFrameA == frame)
    return;
  m_Config.m_LocalFrameA = frame;
  QueueSettingsUpdate(true);
}
void xiiJoltJointComponent::SetLocalFrameB(const xiiTransform& frame)
{
  if (m_Config.m_LocalFrameB == frame)
    return;
  m_Config.m_LocalFrameB = frame;
  QueueSettingsUpdate(true);
}
xiiResult xiiJoltJointComponent::CaptureJointFrames()
{
  if (m_Config.m_hActorB.IsInvalidated() || m_Config.m_hActorB.GetInternalID().m_WorldIndex != GetWorld()->GetIndex() ||
      (!m_Config.m_hActorA.IsInvalidated() && m_Config.m_hActorA.GetInternalID().m_WorldIndex != GetWorld()->GetIndex()))
    return XII_FAILURE;
  xiiGameObject* pActorB = nullptr;
  xiiGameObject* pActorA = nullptr;
  if (!GetWorld()->TryGetObject(m_Config.m_hActorB, pActorB) ||
      (!m_Config.m_hActorA.IsInvalidated() && !GetWorld()->TryGetObject(m_Config.m_hActorA, pActorA)))
    return XII_FAILURE;

  GetOwner()->UpdateGlobalTransform();
  auto frame     = GetOwner()->GetGlobalTransform();
  frame.m_vScale = xiiVec3(1.0f);
  if (!frame.m_vPosition.IsValid() || !frame.m_qRotation.IsValid())
    return XII_FAILURE;
  auto       config  = m_Config;
  const auto capture = [&frame](xiiGameObject* pActor, xiiTransform& out_local) -> xiiResult {
    if (pActor == nullptr)
      out_local = frame;
    else
    {
      pActor->UpdateGlobalTransform();
      const auto& actorPose = pActor->GetGlobalTransform();
      if (!actorPose.m_vPosition.IsValid() || !actorPose.m_qRotation.IsValid() || !actorPose.m_vScale.IsValid() ||
          actorPose.m_vScale.x == 0 || actorPose.m_vScale.y == 0 || actorPose.m_vScale.z == 0)
        return XII_FAILURE;
      out_local          = xiiTransform::MakeLocalTransform(actorPose, frame);
      out_local.m_vScale = xiiVec3(1.0f);
    }
    return out_local.m_vPosition.IsValid() && out_local.m_qRotation.IsValid() ? XII_SUCCESS : XII_FAILURE;
  };
  if (capture(pActorA, config.m_LocalFrameA).Failed() || capture(pActorB, config.m_LocalFrameB).Failed())
    return XII_FAILURE;
  SetJointConfig(config);
  return XII_SUCCESS;
}
void xiiJoltJointComponent::SetBreakForce(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue < 0.0f || m_fBreakForce == fValue)
    return;
  m_fBreakForce = fValue;
  QueueSettingsUpdate();
}
void xiiJoltJointComponent::SetBreakTorqueValue(float fValue)
{
  if (!xiiMath::IsFinite(fValue) || fValue < 0.0f || m_fBreakTorque == fValue)
    return;
  m_fBreakTorque = fValue;
  QueueSettingsUpdate();
}
void xiiJoltJointComponent::RecreateJoint()
{
  m_bBroken = false;
  QueueSettingsUpdate(true);
}
void xiiJoltJointComponent::QueueSettingsUpdate(bool bRecreate)
{
  if (IsActiveAndSimulating())
    GetWorld()->GetOrCreateModule<xiiJoltPhysicsWorldModule>()->QueueJointUpdate(GetHandle(), bRecreate);
}
void xiiJoltJointComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  m_bBroken = false;
  QueueSettingsUpdate(true);
}
void xiiJoltJointComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiJoltPhysicsWorldModule>())
    pModule->RemoveJoint(GetHandle());
  SUPER::OnDeactivated();
}
void xiiJoltJointComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.WriteGameObjectHandle(m_Config.m_hActorA);
  inout_stream.WriteGameObjectHandle(m_Config.m_hActorB);
  inout_stream.GetStream() << m_Config.m_LocalFrameA << m_Config.m_LocalFrameB << m_fBreakForce << m_fBreakTorque;
}
void xiiJoltJointComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  m_Config.m_hActorA = inout_stream.ReadGameObjectHandle();
  m_Config.m_hActorB = inout_stream.ReadGameObjectHandle();
  inout_stream.GetStream() >> m_Config.m_LocalFrameA >> m_Config.m_LocalFrameB >> m_fBreakForce >> m_fBreakTorque;
}

xiiJoltFixedJointComponent::xiiJoltFixedJointComponent()  = default;
xiiJoltFixedJointComponent::~xiiJoltFixedJointComponent() = default;

xiiJoltPointJointComponent::xiiJoltPointJointComponent()  = default;
xiiJoltPointJointComponent::~xiiJoltPointJointComponent() = default;

xiiJoltHingeJointComponent::xiiJoltHingeJointComponent()  = default;
xiiJoltHingeJointComponent::~xiiJoltHingeJointComponent() = default;
void xiiJoltHingeJointComponent::SetLimitsEnabled(bool value)
{
  if (m_bLimitsEnabled == value)
    return;
  m_bLimitsEnabled = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetLowerLimit(xiiAngle value)
{
  if (!xiiMath::IsFinite(value.GetRadian()))
    return;
  if (m_LowerLimit == value)
    return;
  m_LowerLimit = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetUpperLimit(xiiAngle value)
{
  if (!xiiMath::IsFinite(value.GetRadian()))
    return;
  if (m_UpperLimit == value)
    return;
  m_UpperLimit = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetMotorMode(xiiEnum<xiiJoltJointMotorMode> value)
{
  if (m_MotorMode == value)
    return;
  m_MotorMode = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetTargetVelocity(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  if (m_fTargetVelocity == value)
    return;
  m_fTargetVelocity = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetTargetAngle(xiiAngle value)
{
  if (!xiiMath::IsFinite(value.GetRadian()))
    return;
  if (m_TargetAngle == value)
    return;
  m_TargetAngle = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetMaxMotorTorque(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMaxMotorTorque == value)
    return;
  m_fMaxMotorTorque = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetMotorFrequency(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMotorFrequency == value)
    return;
  m_fMotorFrequency = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SetMotorDamping(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMotorDamping == value)
    return;
  m_fMotorDamping = value;
  QueueSettingsUpdate();
}
void xiiJoltHingeJointComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_bLimitsEnabled << m_LowerLimit << m_UpperLimit << m_MotorMode << m_fTargetVelocity << m_TargetAngle << m_fMaxMotorTorque << m_fMotorFrequency << m_fMotorDamping;
}
void xiiJoltHingeJointComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_bLimitsEnabled >> m_LowerLimit >> m_UpperLimit >> m_MotorMode >> m_fTargetVelocity >> m_TargetAngle >> m_fMaxMotorTorque >> m_fMotorFrequency >> m_fMotorDamping;
}

xiiJoltSliderJointComponent::xiiJoltSliderJointComponent()  = default;
xiiJoltSliderJointComponent::~xiiJoltSliderJointComponent() = default;
void xiiJoltSliderJointComponent::SetLimitsEnabled(bool value)
{
  if (m_bLimitsEnabled == value)
    return;
  m_bLimitsEnabled = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetLowerLimit(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  if (m_fLowerLimit == value)
    return;
  m_fLowerLimit = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetUpperLimit(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  if (m_fUpperLimit == value)
    return;
  m_fUpperLimit = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetMotorMode(xiiEnum<xiiJoltJointMotorMode> value)
{
  if (m_MotorMode == value)
    return;
  m_MotorMode = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetTargetVelocity(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  if (m_fTargetVelocity == value)
    return;
  m_fTargetVelocity = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetTargetPosition(float value)
{
  if (!xiiMath::IsFinite(value))
    return;
  if (m_fTargetPosition == value)
    return;
  m_fTargetPosition = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetMaxMotorForce(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMaxMotorForce == value)
    return;
  m_fMaxMotorForce = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetMotorFrequency(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMotorFrequency == value)
    return;
  m_fMotorFrequency = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SetMotorDamping(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMotorDamping == value)
    return;
  m_fMotorDamping = value;
  QueueSettingsUpdate();
}
void xiiJoltSliderJointComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_bLimitsEnabled << m_fLowerLimit << m_fUpperLimit << m_MotorMode << m_fTargetVelocity << m_fTargetPosition << m_fMaxMotorForce << m_fMotorFrequency << m_fMotorDamping;
}
void xiiJoltSliderJointComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_bLimitsEnabled >> m_fLowerLimit >> m_fUpperLimit >> m_MotorMode >> m_fTargetVelocity >> m_fTargetPosition >> m_fMaxMotorForce >> m_fMotorFrequency >> m_fMotorDamping;
}

xiiJoltDistanceJointComponent::xiiJoltDistanceJointComponent()  = default;
xiiJoltDistanceJointComponent::~xiiJoltDistanceJointComponent() = default;
void xiiJoltDistanceJointComponent::SetMinDistance(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMinDistance == value)
    return;
  m_fMinDistance = value;
  QueueSettingsUpdate();
}
void xiiJoltDistanceJointComponent::SetMaxDistance(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fMaxDistance == value)
    return;
  m_fMaxDistance = value;
  QueueSettingsUpdate();
}
void xiiJoltDistanceJointComponent::SetSpringFrequency(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fSpringFrequency == value)
    return;
  m_fSpringFrequency = value;
  QueueSettingsUpdate();
}
void xiiJoltDistanceJointComponent::SetSpringDamping(float value)
{
  if (!xiiMath::IsFinite(value) || value < 0.0f)
    return;
  if (m_fSpringDamping == value)
    return;
  m_fSpringDamping = value;
  QueueSettingsUpdate();
}
void xiiJoltDistanceJointComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_fMinDistance << m_fMaxDistance << m_fSpringFrequency << m_fSpringDamping;
}
void xiiJoltDistanceJointComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_fMinDistance >> m_fMaxDistance >> m_fSpringFrequency >> m_fSpringDamping;
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Joints_Implementation_JoltJointComponent);
