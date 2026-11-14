/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <Foundation/Strings/String.h>

using xiiMuJoCoBodyComponentManager = xiiComponentManager<class xiiMuJoCoBodyComponent, xiiBlockStorageType::Compact>;

/// Drives the owning object's world position and orientation from a named MuJoCo body.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoBodyComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMuJoCoBodyComponent, xiiComponent, xiiMuJoCoBodyComponentManager);

public:
  xiiMuJoCoBodyComponent();
  ~xiiMuJoCoBodyComponent() override;

  const char* GetBodyName() const { return m_sBodyName.GetData(); }
  void        SetBodyName(const char* szName);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  void OnActivated() override;
  void OnSimulationStarted() override;
  void OnDeactivated() override;

  xiiString m_sBodyName;
};
