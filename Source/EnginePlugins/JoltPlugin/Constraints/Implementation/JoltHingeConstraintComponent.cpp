#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Constraints/JoltHingeConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltHingeConstraintComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("LimitMode", xiiJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    XII_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitAngle, SetLowerLimitAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0), xiiAngle::Degree(180))),
    XII_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitAngle, SetUpperLimitAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(0), xiiAngle::Degree(180))),
    XII_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ENUM_ACCESSOR_PROPERTY("DriveMode", xiiJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    XII_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    XII_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiMinValueTextAttribute("Maximum")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.2f, xiiColor::BurlyWood)
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltHingeConstraintComponent::xiiJoltHingeConstraintComponent()  = default;
xiiJoltHingeConstraintComponent::~xiiJoltHingeConstraintComponent() = default;

void xiiJoltHingeConstraintComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_LimitMode;
  s << m_LowerLimit;
  s << m_UpperLimit;

  s << m_DriveMode;
  s << m_DriveTargetValue;
  s << m_fDriveStrength;

  s << m_fFriction;
}

void xiiJoltHingeConstraintComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_LimitMode;
  s >> m_LowerLimit;
  s >> m_UpperLimit;

  s >> m_DriveMode;
  s >> m_DriveTargetValue;
  s >> m_fDriveStrength;

  s >> m_fFriction;
}

void xiiJoltHingeConstraintComponent::SetLimitMode(xiiJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::SetLowerLimitAngle(xiiAngle f)
{
  m_LowerLimit = xiiMath::Clamp(f, xiiAngle(), xiiAngle::Degree(180));
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::SetUpperLimitAngle(xiiAngle f)
{
  m_UpperLimit = xiiMath::Clamp(f, xiiAngle(), xiiAngle::Degree(180));
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::SetFriction(float f)
{
  m_fFriction = xiiMath::Max(f, 0.0f);
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::SetDriveMode(xiiJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::SetDriveTargetValue(xiiAngle f)
{
  m_DriveTargetValue = f;
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = xiiMath::Max(f, 0.0f);
  QueueApplySettings();
}

void xiiJoltHingeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::HingeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace              = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1             = inv1 * xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2             = inv2 * xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHingeAxis1         = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3(1, 0, 0)));
  opt.mHingeAxis2         = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3(1, 0, 0)));
  opt.mNormalAxis1        = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3(0, 1, 0)));
  opt.mNormalAxis2        = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void xiiJoltHingeConstraintComponent::ApplySettings()
{
  xiiJoltConstraintComponent::ApplySettings();

  JPH::HingeConstraint* pConstraint = static_cast<JPH::HingeConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);

  if (m_LimitMode != xiiJoltConstraintLimitMode::NoLimit)
  {
    float low  = m_LowerLimit.GetRadian();
    float high = m_UpperLimit.GetRadian();

    const float fLowest = xiiAngle::Degree(1.0f).GetRadian();

    // there should be at least some slack
    if (low <= fLowest && high <= fLowest)
    {
      low  = fLowest;
      high = fLowest;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-JPH::JPH_PI, +JPH::JPH_PI);
  }

  // drive
  {
    if (m_DriveMode == xiiJoltConstraintDriveMode::NoDrive)
    {
      pConstraint->SetMotorState(JPH::EMotorState::Off);
    }
    else
    {
      if (m_DriveMode == xiiJoltConstraintDriveMode::DriveVelocity)
      {
        pConstraint->SetMotorState(JPH::EMotorState::Velocity);
        pConstraint->SetTargetAngularVelocity(m_DriveTargetValue.GetRadian());
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetAngle(m_DriveTargetValue.GetRadian());
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

      pConstraint->GetMotorSettings().mFrequency = 20.0f;
      pConstraint->GetMotorSettings().SetForceLimit(strength);
      pConstraint->GetMotorSettings().SetTorqueLimit(strength);
    }
  }

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltHingeConstraintComponent);

