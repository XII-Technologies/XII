/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>

using xiiMuJoCoModelComponentManager = xiiComponentManager<class xiiMuJoCoModelComponent, xiiBlockStorageType::Compact>;

/// Contributes an instance to the world's MuJoCo simulation. The owning object's world
/// transform places the model and its hierarchy scopes scene bindings.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoModelComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMuJoCoModelComponent, xiiComponent, xiiMuJoCoModelComponentManager);

public:
  xiiMuJoCoModelComponent();
  ~xiiMuJoCoModelComponent() override;

  const xiiMuJoCoModelResourceHandle& GetModel() const { return m_hModel; }
  void                                SetModel(const xiiMuJoCoModelResourceHandle& hModel);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  void OnActivated() override;
  void OnSimulationStarted() override;
  void OnDeactivated() override;
  void UpdateModel();

  xiiMuJoCoModelResourceHandle m_hModel;
};
