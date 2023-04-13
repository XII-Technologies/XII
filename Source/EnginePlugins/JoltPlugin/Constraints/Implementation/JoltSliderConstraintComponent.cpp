#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <JoltPlugin/Constraints/JoltSliderConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltSliderConstraintComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_ACCESSOR_PROPERTY("LimitMode", xiiJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    XII_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitDistance, SetLowerLimitDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitDistance, SetUpperLimitDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ENUM_ACCESSOR_PROPERTY("DriveMode", xiiJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    XII_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    XII_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiMinValueTextAttribute("Maximum")),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 1.0f, xiiColor::Orange, nullptr, "UpperLimit"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::NegativeX, 1.0f, xiiColor::Teal, nullptr, "LowerLimit"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltSliderConstraintComponent::xiiJoltSliderConstraintComponent()  = default;
xiiJoltSliderConstraintComponent::~xiiJoltSliderConstraintComponent() = default;

void xiiJoltSliderConstraintComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fLowerLimitDistance;
  s << m_fUpperLimitDistance;
  s << m_fFriction;
  s << m_LimitMode;

  s << m_DriveMode;
  s << m_fDriveTargetValue;
  s << m_fDriveStrength;
}

void xiiJoltSliderConstraintComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fLowerLimitDistance;
  s >> m_fUpperLimitDistance;
  s >> m_fFriction;
  s >> m_LimitMode;

  s >> m_DriveMode;
  s >> m_fDriveTargetValue;
  s >> m_fDriveStrength;
}

void xiiJoltSliderConstraintComponent::SetLimitMode(xiiJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void xiiJoltSliderConstraintComponent::SetLowerLimitDistance(float f)
{
  m_fLowerLimitDistance = f;
  QueueApplySettings();
}

void xiiJoltSliderConstraintComponent::SetUpperLimitDistance(float f)
{
  m_fUpperLimitDistance = f;
  QueueApplySettings();
}

void xiiJoltSliderConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void xiiJoltSliderConstraintComponent::SetDriveMode(xiiJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void xiiJoltSliderConstraintComponent::SetDriveTargetValue(float f)
{
  m_fDriveTargetValue = f;
  QueueApplySettings();
}

void xiiJoltSliderConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = xiiMath::Max(f, 0.0f);
  QueueApplySettings();
}


void xiiJoltSliderConstraintComponent::ApplySettings()
{
  xiiJoltConstraintComponent::ApplySettings();

  JPH::SliderConstraint* pConstraint = static_cast<JPH::SliderConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionForce(m_fFriction);

  if (m_LimitMode != xiiJoltConstraintLimitMode::NoLimit)
  {
    float low  = m_fLowerLimitDistance;
    float high = m_fUpperLimitDistance;

    if (low == high) // both zero
    {
      high = low + 0.01f;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-FLT_MAX, FLT_MAX);
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
        pConstraint->SetTargetVelocity(m_fDriveTargetValue);
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetPosition(m_fDriveTargetValue);
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

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

void xiiJoltSliderConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SliderConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace              = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1             = inv1 * xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2             = inv2 * xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mSliderAxis1        = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3(1, 0, 0)));
  opt.mSliderAxis2        = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3(1, 0, 0)));
  opt.mNormalAxis1        = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3(0, 1, 0)));
  opt.mNormalAxis2        = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltSliderConstraintComponent);

