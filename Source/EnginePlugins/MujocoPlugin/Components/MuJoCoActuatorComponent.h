/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <Foundation/Strings/String.h>
#include <MujocoPlugin/System/MuJoCoState.h>

using xiiMuJoCoActuatorComponentManager = xiiComponentManager<class xiiMuJoCoActuatorComponent, xiiBlockStorageType::Compact>;

/// Supplies a persistent scalar command to an existing model actuator.
/// Multi-channel actuators can be controlled through the world module's array API.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoActuatorComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMuJoCoActuatorComponent, xiiComponent, xiiMuJoCoActuatorComponentManager);

public:
  xiiMuJoCoActuatorComponent();
  ~xiiMuJoCoActuatorComponent() override;

  const char* GetActuatorName() const { return m_sActuatorName.GetData(); }
  void        SetActuatorName(const char* szName);
  double      GetControlValue() const { return m_fControlValue; }
  void        SetControlValue(double fValue);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  friend class xiiMuJoCoPhysicsWorldModule;

  void OnSimulationStarted() override;
  void OnDeactivated() override;
  void ApplyControl();
  void ReleaseControl();

  xiiString          m_sActuatorName;
  double             m_fControlValue = 0.0;
  xiiMuJoCoElementId m_ActuatorId;
  xiiUInt64          m_uiResolvedGeneration = 0;
};
