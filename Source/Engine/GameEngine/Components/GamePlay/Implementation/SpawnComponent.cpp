/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Components/Gameplay/SpawnComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSpawnComponent, 3, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Prefab")),
    XII_ACCESSOR_PROPERTY("AttachAsChild", GetAttachAsChild, SetAttachAsChild),
    XII_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    XII_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    XII_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant()), new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(1.0))),
    XII_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant())),
    XII_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new xiiClampValueAttribute(xiiAngle(), xiiAngle::MakeFromDegree(179.0))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.5f, xiiColorScheme::LightUI(xiiColorScheme::Lime)),
    new xiiConeVisualizerAttribute(xiiBasisAxis::PositiveX, "Deviation", 0.5f, nullptr, xiiColorScheme::LightUI(xiiColorScheme::Lime)),
    new xiiConeAngleManipulatorAttribute("Deviation", 0.5f),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnTriggered),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(CanTriggerManualSpawn),
    XII_SCRIPT_FUNCTION_PROPERTY(TriggerManualSpawn, In, "IgnoreSpawnDelay", In, "LocalOffset"),
    XII_SCRIPT_FUNCTION_PROPERTY(ScheduleSpawn),
  }
  XII_END_FUNCTIONS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSpawnComponent::xiiSpawnComponent()  = default;
xiiSpawnComponent::~xiiSpawnComponent() = default;

void xiiSpawnComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::SpawnAtStart))
  {
    ScheduleSpawn();
  }
}


void xiiSpawnComponent::OnDeactivated()
{
  m_SpawnFlags.Remove(xiiSpawnComponentFlags::SpawnInFlight);

  SUPER::OnDeactivated();
}

bool xiiSpawnComponent::SpawnOnce(const xiiVec3& vLocalOffset)
{
  if (m_hPrefab.IsValid())
  {
    xiiTransform tLocalSpawn;
    tLocalSpawn.SetIdentity();
    tLocalSpawn.m_vPosition = vLocalOffset;

    if (m_MaxDeviation.GetRadian() > 0)
    {
      const xiiVec3 vTiltAxis = xiiVec3(0, 1, 0);
      const xiiVec3 vTurnAxis = xiiVec3(1, 0, 0);

      const xiiAngle tiltAngle = xiiAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, (double)m_MaxDeviation.GetRadian()));
      const xiiAngle turnAngle = xiiAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, xiiMath::Pi<double>() * 2.0));

      xiiQuat qTilt, qTurn, qDeviate;
      qTilt    = xiiQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);
      qTurn    = xiiQuat::MakeFromAxisAndAngle(vTurnAxis, turnAngle);
      qDeviate = qTurn * qTilt;

      tLocalSpawn.m_qRotation = qDeviate;
    }

    DoSpawn(tLocalSpawn);

    return true;
  }

  return false;
}


void xiiSpawnComponent::DoSpawn(const xiiTransform& tLocalSpawn)
{
  xiiResourceLock<xiiPrefabResource> pResource(m_hPrefab, xiiResourceAcquireMode::AllowLoadingFallback);

  xiiPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  if (m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::AttachAsChild))
  {
    options.m_hParent = GetOwner()->GetHandle();

    pResource->InstantiatePrefab(*GetWorld(), tLocalSpawn, options, &m_Parameters);
  }
  else
  {
    xiiTransform tGlobalSpawn = xiiTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), tLocalSpawn);

    pResource->InstantiatePrefab(*GetWorld(), tGlobalSpawn, options, &m_Parameters);
  }
}

void xiiSpawnComponent::ScheduleSpawn()
{
  if (m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::SpawnInFlight))
    return;

  xiiMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("scheduled_spawn");

  m_SpawnFlags.Add(xiiSpawnComponentFlags::SpawnInFlight);

  xiiWorld* pWorld = GetWorld();

  const xiiTime tKill = xiiTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleMinMax(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);
}

void xiiSpawnComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_SpawnFlags.GetValue();
  s << m_hPrefab;

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_MaxDeviation;
  s << m_LastManualSpawn;

  xiiPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void xiiSpawnComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const xiiUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  xiiSpawnComponentFlags::StorageType flags;
  s >> flags;
  m_SpawnFlags.SetValue(flags);

  s >> m_hPrefab;

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_MaxDeviation;
  s >> m_LastManualSpawn;

  if (uiVersion >= 3)
  {
    xiiPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
  }
}

bool xiiSpawnComponent::CanTriggerManualSpawn() const
{
  const xiiTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  return tNow - m_LastManualSpawn >= m_MinDelay;
}

bool xiiSpawnComponent::TriggerManualSpawn(bool bIgnoreSpawnDelay /*= false*/, const xiiVec3& vLocalOffset /*= xiiVec3::MakeZero()*/)
{
  const xiiTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (bIgnoreSpawnDelay == false && tNow - m_LastManualSpawn < m_MinDelay)
    return false;

  m_LastManualSpawn = tNow;
  return SpawnOnce(vLocalOffset);
}

bool xiiSpawnComponent::GetSpawnAtStart() const
{
  return m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::SpawnAtStart);
}

void xiiSpawnComponent::SetSpawnAtStart(bool b)
{
  m_SpawnFlags.AddOrRemove(xiiSpawnComponentFlags::SpawnAtStart, b);
}

bool xiiSpawnComponent::GetSpawnContinuously() const
{
  return m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::SpawnContinuously);
}

void xiiSpawnComponent::SetSpawnContinuously(bool b)
{
  m_SpawnFlags.AddOrRemove(xiiSpawnComponentFlags::SpawnContinuously, b);
}

bool xiiSpawnComponent::GetAttachAsChild() const
{
  return m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::AttachAsChild);
}

void xiiSpawnComponent::SetAttachAsChild(bool b)
{
  m_SpawnFlags.AddOrRemove(xiiSpawnComponentFlags::AttachAsChild, b);
}

void xiiSpawnComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == xiiTempHashedString("scheduled_spawn"))
  {
    m_SpawnFlags.Remove(xiiSpawnComponentFlags::SpawnInFlight);

    SpawnOnce(xiiVec3::MakeZero());

    // do it all again
    if (m_SpawnFlags.IsAnySet(xiiSpawnComponentFlags::SpawnContinuously))
    {
      ScheduleSpawn();
    }
  }
  else if (msg.m_sMessage == xiiTempHashedString("spawn"))
  {
    TriggerManualSpawn();
  }
}

const xiiRangeView<xiiStringView, xiiUInt32> xiiSpawnComponent::GetParameters() const
{
  return xiiRangeView<xiiStringView, xiiUInt32>([]() -> xiiUInt32 { return 0; }, [this]() -> xiiUInt32 { return m_Parameters.GetCount(); }, [](xiiUInt32& ref_uiIt) { ++ref_uiIt; }, [this](const xiiUInt32& uiIt) -> xiiStringView { return m_Parameters.GetKey(uiIt).GetString().GetView(); });
}

void xiiSpawnComponent::SetParameter(xiiStringView sKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(sKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void xiiSpawnComponent::RemoveParameter(xiiStringView sKey)
{
  m_Parameters.RemoveAndCopy(sKey);
}

bool xiiSpawnComponent::GetParameter(xiiStringView sKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(sKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiSpawnComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiSpawnComponentPatch_1_2() :
    xiiGraphPatch("xiiSpawnComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Attach as Child", "AttachAsChild");
    pNode->RenameProperty("Spawn at Start", "SpawnAtStart");
    pNode->RenameProperty("Spawn Continuously", "SpawnContinuously");
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
  }
};

xiiSpawnComponentPatch_1_2 g_xiiSpawnComponentPatch_1_2;

XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnComponent);
