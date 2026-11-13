/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <MujocoPlugin/Components/MuJoCoJointComponent.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMuJoCoJointComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("JointName", GetJointName, SetJointName),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Physics/MuJoCo"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

xiiMuJoCoJointComponent::xiiMuJoCoJointComponent()  = default;
xiiMuJoCoJointComponent::~xiiMuJoCoJointComponent() = default;

void xiiMuJoCoJointComponent::SetJointName(const char* szName)
{
  if (m_sJointName == szName)
    return;
  m_sJointName           = szName;
  m_JointId              = {};
  m_uiResolvedGeneration = 0;
}

void xiiMuJoCoJointComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>();
  ResolveJoint();
}

void xiiMuJoCoJointComponent::OnDeactivated()
{
  m_JointId              = {};
  m_uiResolvedGeneration = 0;
  SUPER::OnDeactivated();
}

bool xiiMuJoCoJointComponent::ResolveJoint() const
{
  if (!IsActiveAndSimulating())
    return false;
  const auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>();
  if (pModule == nullptr)
    return false;
  const xiiUInt64 uiGeneration = pModule->GetModelGeneration();
  if (uiGeneration != m_uiResolvedGeneration)
  {
    m_JointId              = {};
    m_uiResolvedGeneration = uiGeneration;
    if (uiGeneration != 0 && !m_sJointName.IsEmpty())
    {
      m_JointId = pModule->ResolveElement(xiiMuJoCoElementType::Joint, m_sJointName);
      if (m_JointId.m_iIndex < 0)
        xiiLog::Warning("MuJoCo joint '{}' was not found in world '{}'.", m_sJointName, GetWorld()->GetName());
    }
  }
  return m_JointId.m_iIndex >= 0;
}

bool xiiMuJoCoJointComponent::GetJointInfo(xiiMuJoCoJointInfo& out_info) const
{
  return ResolveJoint() && GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->GetJointInfo(m_JointId, out_info);
}

bool xiiMuJoCoJointComponent::GetPosition(xiiArrayPtr<double> out_values) const
{
  return ResolveJoint() && GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->GetJointPosition(m_JointId, out_values);
}

bool xiiMuJoCoJointComponent::GetVelocity(xiiArrayPtr<double> out_values) const
{
  return ResolveJoint() && GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->GetJointVelocity(m_JointId, out_values);
}

xiiResult xiiMuJoCoJointComponent::SetPosition(xiiArrayPtr<const double> values)
{
  return ResolveJoint() ? GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->SetJointPosition(m_JointId, values) : XII_FAILURE;
}

xiiResult xiiMuJoCoJointComponent::SetVelocity(xiiArrayPtr<const double> values)
{
  return ResolveJoint() ? GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->SetJointVelocity(m_JointId, values) : XII_FAILURE;
}

xiiResult xiiMuJoCoJointComponent::AddForce(xiiArrayPtr<const double> values)
{
  return ResolveJoint() ? GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->AddJointForce(m_JointId, values) : XII_FAILURE;
}

xiiResult xiiMuJoCoJointComponent::ResetJoint()
{
  return ResolveJoint() ? GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->ResetJoint(m_JointId) : XII_FAILURE;
}

void xiiMuJoCoJointComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_sJointName;
}

void xiiMuJoCoJointComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_sJointName;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Components_Implementation_MuJoCoJointComponent);
