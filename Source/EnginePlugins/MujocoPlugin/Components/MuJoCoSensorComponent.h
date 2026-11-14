/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <MujocoPlugin/MujocoPluginDLL.h>

#include <Core/World/ComponentManager.h>
#include <Foundation/Strings/String.h>
#include <MujocoPlugin/System/MuJoCoInspection.h>

using xiiMuJoCoSensorComponentManager = xiiComponentManager<class xiiMuJoCoSensorComponent, xiiBlockStorageType::Compact>;

/// Binds a scene object to one authored sensor without owning or copying its runtime measurements.
/// Reads require the world's write marker and follow the world module's borrowed-view contract.
class XII_MUJOCOPLUGIN_DLL xiiMuJoCoSensorComponent final : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMuJoCoSensorComponent, xiiComponent, xiiMuJoCoSensorComponentManager);

public:
  xiiMuJoCoSensorComponent();
  ~xiiMuJoCoSensorComponent() override;

  const char*        GetSensorName() const { return m_sSensorName.GetData(); }
  void               SetSensorName(const char* szName);
  [[nodiscard]] bool GetSensorInfo(xiiMuJoCoSensorInfo& out_info) const;
  [[nodiscard]] bool GetReading(xiiMuJoCoSensorReading& out_reading);
  [[nodiscard]] bool CopyReading(xiiArrayPtr<double> out_values, xiiMuJoCoStateStamp& out_stamp);

  void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  void OnSimulationStarted() override;
  void OnDeactivated() override;
  bool ResolveSensor() const;

  xiiString                  m_sSensorName;
  mutable xiiMuJoCoElementId m_SensorId;
  mutable xiiUInt64          m_uiResolvedGeneration = 0;
};
