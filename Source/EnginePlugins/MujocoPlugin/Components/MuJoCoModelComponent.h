/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>

using xiiMuJoCoModelComponentManager = xiiComponentManager<class xiiMuJoCoModelComponent, xiiBlockStorageType::Compact>;

/// Controls the single scene-authored simulation in the world. Model coordinates are world coordinates.
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
  void OnSimulationStarted() override;
  void OnDeactivated() override;
  void UpdateModel();

  xiiMuJoCoModelResourceHandle m_hModel;
};
