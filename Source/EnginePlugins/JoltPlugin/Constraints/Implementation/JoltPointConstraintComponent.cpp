#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <JoltPlugin/Constraints/JoltPointConstraintComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiJoltPointConstraintComponent, 1, xiiComponentMode::Static)
{
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiJoltPointConstraintComponent::xiiJoltPointConstraintComponent()  = default;
xiiJoltPointConstraintComponent::~xiiJoltPointConstraintComponent() = default;

void xiiJoltPointConstraintComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  // auto& s = stream.GetStream();
}

void xiiJoltPointConstraintComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}

void xiiJoltPointConstraintComponent::ApplySettings()
{
  SUPER::ApplySettings();
}

void xiiJoltPointConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::PointConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  opt.mSpace  = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * xiiJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * xiiJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltPointConstraintComponent);
