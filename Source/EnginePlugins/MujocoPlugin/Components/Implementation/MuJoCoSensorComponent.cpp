/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <MujocoPlugin/Components/MuJoCoSensorComponent.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMuJoCoSensorComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("SensorName", GetSensorName, SetSensorName),
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

xiiMuJoCoSensorComponent::xiiMuJoCoSensorComponent()  = default;
xiiMuJoCoSensorComponent::~xiiMuJoCoSensorComponent() = default;

void xiiMuJoCoSensorComponent::SetSensorName(const char* szName)
{
  if (m_sSensorName == szName)
    return;
  m_sSensorName          = szName;
  m_SensorId             = {};
  m_uiResolvedGeneration = 0;
}

void xiiMuJoCoSensorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>();
  ResolveSensor();
}

void xiiMuJoCoSensorComponent::OnDeactivated()
{
  m_SensorId             = {};
  m_uiResolvedGeneration = 0;
  SUPER::OnDeactivated();
}

bool xiiMuJoCoSensorComponent::ResolveSensor() const
{
  if (!IsActiveAndSimulating())
    return false;
  const auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>();
  if (pModule == nullptr)
    return false;
  const xiiUInt64 uiGeneration = pModule->GetModelGeneration();
  if (uiGeneration != m_uiResolvedGeneration)
  {
    m_SensorId = {};
    if (uiGeneration != 0 && !pModule->IsStateAccessible())
      return false;
    m_uiResolvedGeneration = uiGeneration;
    if (uiGeneration != 0 && !m_sSensorName.IsEmpty())
    {
      m_SensorId = pModule->ResolveElement(xiiMuJoCoElementType::Sensor, m_sSensorName);
      if (m_SensorId.m_iIndex < 0)
        xiiLog::Warning("MuJoCo sensor '{}' was not found in world '{}'.", m_sSensorName, GetWorld()->GetName());
    }
  }
  return m_SensorId.m_iIndex >= 0;
}

bool xiiMuJoCoSensorComponent::GetSensorInfo(xiiMuJoCoSensorInfo& out_info) const
{
  return ResolveSensor() && GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->GetSensorInfo(m_SensorId, out_info);
}

bool xiiMuJoCoSensorComponent::GetReading(xiiMuJoCoSensorReading& out_reading)
{
  out_reading = {};
  return ResolveSensor() && GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->GetSensorReading(m_SensorId, out_reading);
}

bool xiiMuJoCoSensorComponent::CopyReading(xiiArrayPtr<double> out_values, xiiMuJoCoStateStamp& out_stamp)
{
  return ResolveSensor() && GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>()->CopySensorReading(m_SensorId, out_values, out_stamp);
}

void xiiMuJoCoSensorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_sSensorName;
}

void xiiMuJoCoSensorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  inout_stream.GetStream() >> m_sSensorName;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Components_Implementation_MuJoCoSensorComponent);
