/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/ArrayPtr.h>
#include <MujocoPlugin/System/MuJoCoState.h>

using xiiMuJoCoJointComponentManager = xiiComponentManager<class xiiMuJoCoJointComponent, xiiBlockStorageType::Compact>;

/// Resolves one model joint. State spans use its independent position and velocity dimensions.
/// Runtime operations require the world's write marker, matching the world module's contract.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoJointComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMuJoCoJointComponent, xiiComponent, xiiMuJoCoJointComponentManager);

public:
  xiiMuJoCoJointComponent();
  ~xiiMuJoCoJointComponent() override;

  const char* GetJointName() const { return m_sJointName.GetData(); }
  void        SetJointName(const char* szName);

  [[nodiscard]] bool      GetJointInfo(xiiMuJoCoJointInfo& out_info) const;
  [[nodiscard]] bool      GetPosition(xiiArrayPtr<double> out_values) const;
  [[nodiscard]] bool      GetVelocity(xiiArrayPtr<double> out_values) const;
  [[nodiscard]] xiiResult SetPosition(xiiArrayPtr<const double> values);
  [[nodiscard]] xiiResult SetVelocity(xiiArrayPtr<const double> values);
  [[nodiscard]] xiiResult AddForce(xiiArrayPtr<const double> values);
  [[nodiscard]] xiiResult ResetJoint();

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  void OnSimulationStarted() override;
  void OnDeactivated() override;
  bool ResolveJoint() const;

  xiiString                  m_sJointName;
  mutable xiiMuJoCoElementId m_JointId;
  mutable xiiUInt64          m_uiResolvedGeneration = 0;
};
