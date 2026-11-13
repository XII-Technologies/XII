/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Math.h>
#include <MujocoPlugin/Components/MuJoCoActuatorComponent.h>
#include <MujocoPlugin/System/MuJoCoPhysicsWorldModule.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiMuJoCoActuatorComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ActuatorName", GetActuatorName, SetActuatorName),
    XII_ACCESSOR_PROPERTY("ControlValue", GetControlValue, SetControlValue)->AddAttributes(new xiiDefaultValueAttribute(0.0)),
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

xiiMuJoCoActuatorComponent::xiiMuJoCoActuatorComponent()  = default;
xiiMuJoCoActuatorComponent::~xiiMuJoCoActuatorComponent() = default;

void xiiMuJoCoActuatorComponent::SetActuatorName(const char* szName)
{
  if (m_sActuatorName == szName)
    return;

  ReleaseControl();
  m_sActuatorName = szName;
  if (IsActiveAndSimulating())
    ApplyControl();
}

void xiiMuJoCoActuatorComponent::SetControlValue(double fValue)
{
  if (!xiiMath::IsFinite(fValue))
  {
    xiiLog::Warning("MuJoCo actuator control values must be finite.");
    return;
  }
  if (m_fControlValue == fValue)
    return;

  m_fControlValue = fValue;
  if (IsActiveAndSimulating())
    ApplyControl();
}

void xiiMuJoCoActuatorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
  GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>()->RegisterActuator(GetHandle());
  ApplyControl();
}

void xiiMuJoCoActuatorComponent::OnDeactivated()
{
  ReleaseControl();
  if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>())
    pModule->UnregisterActuator(GetHandle());
  SUPER::OnDeactivated();
}

void xiiMuJoCoActuatorComponent::ApplyControl()
{
  auto*           pModule      = GetWorld()->GetOrCreateModule<xiiMuJoCoPhysicsWorldModule>();
  const xiiUInt64 uiGeneration = pModule->GetModelGeneration();
  if (uiGeneration != m_uiResolvedGeneration)
  {
    m_ActuatorId           = {};
    m_uiResolvedGeneration = uiGeneration;
    if (uiGeneration != 0 && !m_sActuatorName.IsEmpty())
    {
      m_ActuatorId = pModule->ResolveElement(xiiMuJoCoElementType::Actuator, m_sActuatorName);
      xiiMuJoCoActuatorInfo info;
      if (!pModule->GetActuatorInfo(m_ActuatorId, info))
        xiiLog::Warning("MuJoCo actuator '{}' was not found in world '{}'.", m_sActuatorName, GetWorld()->GetName());
      else if (info.m_uiControlCount != 1)
      {
        xiiLog::Warning("MuJoCo actuator '{}' has {} control channels. The actuator component requires one scalar channel.", m_sActuatorName, info.m_uiControlCount);
        m_ActuatorId = {};
      }
    }
  }
  if (m_ActuatorId.m_iIndex >= 0)
  {
    if (pModule->SetActuatorControls(m_ActuatorId, xiiArrayPtr<const double>(&m_fControlValue, 1)).Failed())
      xiiLog::Warning("Could not set MuJoCo actuator '{}' control.", m_sActuatorName);
  }
}

void xiiMuJoCoActuatorComponent::ReleaseControl()
{
  if (m_ActuatorId.m_iIndex >= 0)
  {
    if (auto* pModule = GetWorld()->GetModule<xiiMuJoCoPhysicsWorldModule>())
      pModule->ResetActuatorControls(m_ActuatorId).IgnoreResult(); // Replacement may already have invalidated the identifier.
  }
  m_ActuatorId           = {};
  m_uiResolvedGeneration = 0;
}

void xiiMuJoCoActuatorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  inout_stream.GetStream() << m_sActuatorName << m_fControlValue;
}

void xiiMuJoCoActuatorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  double fValue;
  inout_stream.GetStream() >> m_sActuatorName >> fValue;
  SetControlValue(fValue);
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Components_Implementation_MuJoCoActuatorComponent);
