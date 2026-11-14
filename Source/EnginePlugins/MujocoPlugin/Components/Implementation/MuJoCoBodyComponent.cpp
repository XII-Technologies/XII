/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <MujocoPlugin/Components/MuJoCoBodyComponent.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMuJoCoBodyComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("BodyName", GetBodyName, SetBodyName),
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

xiiMuJoCoBodyComponent::xiiMuJoCoBodyComponent()  = default;
xiiMuJoCoBodyComponent::~xiiMuJoCoBodyComponent() = default;

void xiiMuJoCoBodyComponent::SetBodyName(const char* szName)
{
  if (m_sBodyName == szName)
    return;

  m_sBodyName = szName;
  if (IsActiveAndSimulating())
    GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>()->RegisterBody(GetHandle(), m_sBodyName);
  else if (IsActiveAndInitialized())
    if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>(); pModule && pModule->m_bScenePreview)
      pModule->RegisterBody(GetHandle(), m_sBodyName);
}

void xiiMuJoCoBodyComponent::OnActivated()
{
  SUPER::OnActivated();
  if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>(); pModule && pModule->m_bScenePreview)
    pModule->RegisterBody(GetHandle(), m_sBodyName);
}

void xiiMuJoCoBodyComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>()->RegisterBody(GetHandle(), m_sBodyName);
}

void xiiMuJoCoBodyComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>())
    pModule->UnregisterBody(GetHandle());

  SUPER::OnDeactivated();
}

void xiiMuJoCoBodyComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_sBodyName;
}

void xiiMuJoCoBodyComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_sBodyName;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Components_Implementation_MuJoCoBodyComponent);
