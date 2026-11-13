/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <MujocoPlugin/Components/MuJoCoModelComponent.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMuJoCoModelComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_ACCESSOR_PROPERTY("Model", GetModel, SetModel)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_MuJoCoModel", xiiDependencyFlags::Package)),
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

xiiMuJoCoModelComponent::xiiMuJoCoModelComponent()  = default;
xiiMuJoCoModelComponent::~xiiMuJoCoModelComponent() = default;

void xiiMuJoCoModelComponent::SetModel(const xiiMuJoCoModelResourceHandle& hModel)
{
  if (m_hModel == hModel)
    return;

  m_hModel = hModel;
  if (IsActiveAndSimulating())
    UpdateModel();
}

void xiiMuJoCoModelComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  UpdateModel();
}

void xiiMuJoCoModelComponent::OnDeactivated()
{
  if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>())
    pModule->ReleaseModel(GetHandle());

  SUPER::OnDeactivated();
}

void xiiMuJoCoModelComponent::UpdateModel()
{
  if (m_hModel.IsValid())
  {
    GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>()->RequestModel(GetHandle(), m_hModel);
  }
  else if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>())
  {
    pModule->ReleaseModel(GetHandle());
  }
}

void xiiMuJoCoModelComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_hModel;
}

void xiiMuJoCoModelComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_hModel;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Components_Implementation_MuJoCoModelComponent);
