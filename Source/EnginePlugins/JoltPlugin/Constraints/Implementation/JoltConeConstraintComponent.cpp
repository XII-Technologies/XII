#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltConeConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltConeConstraintComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ConeAngle", GetConeAngle, SetConeAngle)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::Degree(175))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiConeVisualizerAttribute(xiiBasisAxis::PositiveX, "ConeAngle", 0.3f, nullptr)
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltConeConstraintComponent::xiiJoltConeConstraintComponent()  = default;
xiiJoltConeConstraintComponent::~xiiJoltConeConstraintComponent() = default;

void xiiJoltConeConstraintComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_ConeAngle;
}

void xiiJoltConeConstraintComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_ConeAngle;
}

void xiiJoltConeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::ConeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace              = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1             = inv1 * xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2             = inv2 * xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHalfConeAngle      = m_ConeAngle.GetRadian() * 0.5f;
  opt.mTwistAxis1         = inv1.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * xiiVec3::UnitXAxis()));
  opt.mTwistAxis2         = inv2.Multiply3x3(xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * xiiVec3::UnitXAxis()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void xiiJoltConeConstraintComponent::ApplySettings()
{
  xiiJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint);
  pConstraint->SetHalfConeAngle(m_ConeAngle.GetRadian() * 0.5f);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool xiiJoltConeConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition().ReduceMax() >= m_fBreakForce)
      {
        return true;
      }
    }

    if (m_fBreakTorque > 0)
    {
      if (pConstraint->GetTotalLambdaRotation() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void xiiJoltConeConstraintComponent::SetConeAngle(xiiAngle f)
{
  m_ConeAngle = f;
  QueueApplySettings();
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltConeConstraintComponent);
