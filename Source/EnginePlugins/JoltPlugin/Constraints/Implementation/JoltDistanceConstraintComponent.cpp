#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <JoltPlugin/Constraints/JoltDistanceConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltDistanceConstraintComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("MinDistance", GetMinDistance, SetMinDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("MaxDistance", GetMaxDistance, SetMaxDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant()), new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Frequency", GetFrequency, SetFrequency)->AddAttributes(new xiiClampValueAttribute(0.0f, 120.0f), new xiiDefaultValueAttribute(2.0f)),
    XII_ACCESSOR_PROPERTY("Damping", GetDamping, SetDamping)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f), new xiiDefaultValueAttribute(0.5f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereVisualizerAttribute("MinDistance", xiiColor::IndianRed),
    new xiiSphereVisualizerAttribute("MaxDistance", xiiColor::LightSkyBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltDistanceConstraintComponent::xiiJoltDistanceConstraintComponent()  = default;
xiiJoltDistanceConstraintComponent::~xiiJoltDistanceConstraintComponent() = default;

void xiiJoltDistanceConstraintComponent::SetMinDistance(float value)
{
  m_fMinDistance = value;
  QueueApplySettings();
}

void xiiJoltDistanceConstraintComponent::SetMaxDistance(float value)
{
  m_fMaxDistance = value;
  QueueApplySettings();
}

void xiiJoltDistanceConstraintComponent::SetFrequency(float value)
{
  m_fFrequency = value;
  QueueApplySettings();
}

void xiiJoltDistanceConstraintComponent::SetDamping(float value)
{
  m_fDamping = value;
  QueueApplySettings();
}

void xiiJoltDistanceConstraintComponent::ApplySettings()
{
  xiiJoltConstraintComponent::ApplySettings();

  JPH::DistanceConstraint* pConstraint = static_cast<JPH::DistanceConstraint*>(m_pConstraint);

  pConstraint->SetFrequency(m_fFrequency);
  pConstraint->SetDamping(m_fDamping);

  const float fMin = xiiMath::Max(0.0f, m_fMinDistance);
  const float fMax = xiiMath::Max(fMin, m_fMaxDistance);
  pConstraint->SetDistance(fMin, fMax);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    xiiJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool xiiJoltDistanceConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::DistanceConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition() >= m_fBreakForce)
      {
        return true;
      }
    }
  }

  return false;
}

void xiiJoltDistanceConstraintComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fMinDistance;
  s << m_fMaxDistance;
  s << m_fFrequency;
  s << m_fDamping;
}

void xiiJoltDistanceConstraintComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fMinDistance;
  s >> m_fMaxDistance;
  s >> m_fFrequency;
  s >> m_fDamping;
}

void xiiJoltDistanceConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::DistanceConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mMinDistance        = 0;
  opt.mMaxDistance        = 1;
  opt.mSpace              = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1             = inv1 * xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2             = inv2 * xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mDamping            = m_fDamping;
  opt.mFrequency          = m_fFrequency;

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltDistanceConstraintComponent);
