#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltSwingTwistConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltSwingTwistConstraintComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SwingLimitY", GetSwingLimitY, SetSwingLimitY)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::Degree(175))),
    XII_ACCESSOR_PROPERTY("SwingLimitZ", GetSwingLimitZ, SetSwingLimitZ)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::Degree(175))),

    XII_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),

    XII_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(5), xiiAngle::Degree(175)), new xiiDefaultValueAttribute(xiiAngle::Degree(90))),
    XII_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new xiiClampValueAttribute(xiiAngle::Degree(5), xiiAngle::Degree(175)), new xiiDefaultValueAttribute(xiiAngle::Degree(90))),

    //XII_ENUM_ACCESSOR_PROPERTY("TwistDriveMode", xiiJoltConstraintDriveMode, GetTwistDriveMode, SetTwistDriveMode),
    //XII_ACCESSOR_PROPERTY("TwistDriveTargetValue", GetTwistDriveTargetValue, SetTwistDriveTargetValue),
    //XII_ACCESSOR_PROPERTY("TwistDriveStrength", GetTwistDriveStrength, SetTwistDriveStrength)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiMinValueTextAttribute("Maximum"))
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiConeVisualizerAttribute(xiiBasisAxis::PositiveX, "SwingLimitY", 0.3f, nullptr)
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltSwingTwistConstraintComponent::xiiJoltSwingTwistConstraintComponent()  = default;
xiiJoltSwingTwistConstraintComponent::~xiiJoltSwingTwistConstraintComponent() = default;

void xiiJoltSwingTwistConstraintComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_SwingLimitY;
  s << m_SwingLimitZ;

  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;

  s << m_fFriction;

  // s << m_TwistDriveMode;
  // s << m_TwistDriveTargetValue;
  // s << m_fTwistDriveStrength;
}

void xiiJoltSwingTwistConstraintComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_SwingLimitY;
  s >> m_SwingLimitZ;

  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;

  s >> m_fFriction;

  // s >> m_TwistDriveMode;
  // s >> m_TwistDriveTargetValue;
  // s >> m_fTwistDriveStrength;
}

void xiiJoltSwingTwistConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SwingTwistConstraintSettings opt;
  opt.mDrawConstraintSize  = 0.1f;
  opt.mSpace               = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPosition1           = inv1 * xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPosition2           = inv2 * xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mPlaneHalfConeAngle  = m_SwingLimitY.GetRadian() * 0.5f;
  opt.mNormalHalfConeAngle = m_SwingLimitZ.GetRadian() * 0.5f;
  opt.mMaxFrictionTorque   = m_fFriction;
  opt.mTwistAxis1          = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3::UnitXAxis()));
  opt.mTwistAxis2          = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3::UnitXAxis()));
  opt.mTwistMinAngle       = -m_LowerTwistLimit.GetRadian();
  opt.mTwistMaxAngle       = m_UpperTwistLimit.GetRadian();
  opt.mPlaneAxis1          = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3::UnitYAxis()));
  opt.mPlaneAxis2          = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3::UnitYAxis()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void xiiJoltSwingTwistConstraintComponent::ApplySettings()
{
  xiiJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::SwingTwistConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);
  pConstraint->SetPlaneHalfConeAngle(m_SwingLimitY.GetRadian() * 0.5f);
  pConstraint->SetNormalHalfConeAngle(m_SwingLimitZ.GetRadian() * 0.5f);
  pConstraint->SetTwistMinAngle(-m_LowerTwistLimit.GetRadian());
  pConstraint->SetTwistMaxAngle(m_UpperTwistLimit.GetRadian());

  // drive
  //{
  //  if (m_TwistDriveMode == xiiJoltConstraintDriveMode::NoDrive)
  //  {
  //    pConstraint->SetTwistMotorState(JPH::EMotorState::Off);
  //  }
  //  else
  //  {
  //    if (m_TwistDriveMode == xiiJoltConstraintDriveMode::DriveVelocity)
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Velocity);
  //      pConstraint->SetTargetAngularVelocityCS(JPH::Vec3::sReplicate(m_TwistDriveTargetValue.GetRadian()));
  //    }
  //    else
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Position);
  //      //pConstraint->SetTargetOrientationCS(m_TwistDriveTargetValue.GetRadian());
  //    }

  //    const float strength = (m_fTwistDriveStrength == 0) ? FLT_MAX : m_fTwistDriveStrength;

  //    pConstraint->GetTwistMotorSettings().mFrequency = 2.0f;
  //    pConstraint->GetTwistMotorSettings().SetForceLimit(strength);
  //    pConstraint->GetTwistMotorSettings().SetTorqueLimit(strength);
  //  }
  //}

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

void xiiJoltSwingTwistConstraintComponent::SetSwingLimitZ(xiiAngle f)
{
  m_SwingLimitZ = f;
  QueueApplySettings();
}

void xiiJoltSwingTwistConstraintComponent::SetSwingLimitY(xiiAngle f)
{
  m_SwingLimitY = f;
  QueueApplySettings();
}

void xiiJoltSwingTwistConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void xiiJoltSwingTwistConstraintComponent::SetLowerTwistLimit(xiiAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void xiiJoltSwingTwistConstraintComponent::SetUpperTwistLimit(xiiAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

// void xiiJoltSwingTwistConstraintComponent::SetTwistDriveMode(xiiJoltConstraintDriveMode::Enum mode)
//{
//   m_TwistDriveMode = mode;
//   QueueApplySettings();
// }
//
// void xiiJoltSwingTwistConstraintComponent::SetTwistDriveTargetValue(xiiAngle f)
//{
//   m_TwistDriveTargetValue = f;
//   QueueApplySettings();
// }
//
// void xiiJoltSwingTwistConstraintComponent::SetTwistDriveStrength(float f)
//{
//   m_fTwistDriveStrength = f;
//   QueueApplySettings();
// }


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltSwingTwistConstraintComponent);

