#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/ResetTransformComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiResetTransformComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("ResetPositionX", m_bResetLocalPositionX)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ResetPositionY", m_bResetLocalPositionY)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("ResetPositionZ", m_bResetLocalPositionZ)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    XII_MEMBER_PROPERTY("ResetRotation", m_bResetLocalRotation)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation)->AddAttributes(new xiiDefaultValueAttribute(xiiQuat::MakeIdentity())),
    XII_MEMBER_PROPERTY("ResetScaling", m_bResetLocalScaling)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("LocalScaling", m_vLocalScaling)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1))),
    XII_MEMBER_PROPERTY("LocalUniformScaling", m_fLocalUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiResetTransformComponent::xiiResetTransformComponent()  = default;
xiiResetTransformComponent::~xiiResetTransformComponent() = default;

void xiiResetTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  xiiVec3 vLocalPos = GetOwner()->GetLocalPosition();

  if (m_bResetLocalPositionX)
    vLocalPos.x = m_vLocalPosition.x;
  if (m_bResetLocalPositionY)
    vLocalPos.y = m_vLocalPosition.y;
  if (m_bResetLocalPositionZ)
    vLocalPos.z = m_vLocalPosition.z;

  GetOwner()->SetLocalPosition(vLocalPos);

  if (m_bResetLocalRotation)
  {
    GetOwner()->SetLocalRotation(m_qLocalRotation);
  }

  if (m_bResetLocalScaling)
  {
    GetOwner()->SetLocalScaling(m_vLocalScaling);
    GetOwner()->SetLocalUniformScaling(m_fLocalUniformScaling);
  }

  // update the global transform right away
  GetOwner()->UpdateGlobalTransform();
}

void xiiResetTransformComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_vLocalPosition;
  s << m_qLocalRotation;
  s << m_vLocalScaling;
  s << m_bResetLocalPositionX;
  s << m_bResetLocalPositionY;
  s << m_bResetLocalPositionZ;
  s << m_bResetLocalRotation;
  s << m_bResetLocalScaling;
  s << m_fLocalUniformScaling;
}

void xiiResetTransformComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_vLocalPosition;
  s >> m_qLocalRotation;
  s >> m_vLocalScaling;
  s >> m_bResetLocalPositionX;
  s >> m_bResetLocalPositionY;
  s >> m_bResetLocalPositionZ;
  s >> m_bResetLocalRotation;
  s >> m_bResetLocalScaling;
  s >> m_fLocalUniformScaling;
}
