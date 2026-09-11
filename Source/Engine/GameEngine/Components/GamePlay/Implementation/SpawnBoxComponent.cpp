/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Components/Gameplay/SpawnBoxComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSpawnBoxComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(2.0f, 2.0f, 0.25f)), new xiiClampValueAttribute(xiiVec3(0), xiiVariant())),
    XII_RESOURCE_MEMBER_PROPERTY("Prefab", m_hPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    XII_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    XII_MEMBER_PROPERTY("MinSpawnCount", m_uiMinSpawnCount)->AddAttributes(new xiiDefaultValueAttribute(10)),
    XII_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange)->AddAttributes(new xiiDefaultValueAttribute(0)),
    XII_MEMBER_PROPERTY("Duration", m_SpawnDuration)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(5))),
    XII_MEMBER_PROPERTY("MaxRotationZ", m_MaxRotationZ),
    XII_MEMBER_PROPERTY("MaxTiltZ", m_MaxTiltZ),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnTriggered),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new xiiBoxVisualizerAttribute("HalfExtents", 2.0f),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.5f, xiiColorScheme::LightUI(xiiColorScheme::Lime)),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(StartSpawning),
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE;
// clang-format on

void xiiSpawnBoxComponent::SetHalfExtents(const xiiVec3& value)
{
  m_vHalfExtents = value.CompMax(xiiVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool xiiSpawnBoxComponent::GetSpawnAtStart() const
{
  return m_Flags.IsAnySet(xiiSpawnBoxComponentFlags::SpawnAtStart);
}

void xiiSpawnBoxComponent::SetSpawnAtStart(bool b)
{
  m_Flags.AddOrRemove(xiiSpawnBoxComponentFlags::SpawnAtStart, b);
}

bool xiiSpawnBoxComponent::GetSpawnContinuously() const
{
  return m_Flags.IsAnySet(xiiSpawnBoxComponentFlags::SpawnContinuously);
}

void xiiSpawnBoxComponent::SetSpawnContinuously(bool b)
{
  m_Flags.AddOrRemove(xiiSpawnBoxComponentFlags::SpawnContinuously, b);
}

void xiiSpawnBoxComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
  s << m_hPrefab;
  s << m_Flags;
  s << m_SpawnDuration;
  s << m_uiMinSpawnCount;
  s << m_uiSpawnCountRange;
  s << m_MaxRotationZ;
  s << m_MaxTiltZ;
}

void xiiSpawnBoxComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
  s >> m_hPrefab;
  s >> m_Flags;
  s >> m_SpawnDuration;
  s >> m_uiMinSpawnCount;
  s >> m_uiSpawnCountRange;
  s >> m_MaxRotationZ;
  s >> m_MaxTiltZ;
}

void xiiSpawnBoxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetSpawnAtStart())
  {
    StartSpawning();
  }
}

void xiiSpawnBoxComponent::StartSpawning()
{
  InternalStartSpawning(true);
}

void xiiSpawnBoxComponent::InternalStartSpawning(bool bFirstTime)
{

  m_uiSpawned      = 0;
  m_uiTotalToSpawn = m_uiMinSpawnCount;
  m_StartTime      = GetWorld()->GetClock().GetAccumulatedTime();

  if (m_uiSpawnCountRange > 0)
  {
    m_uiTotalToSpawn = GetWorld()->GetRandomNumberGenerator().IntMinMax(m_uiMinSpawnCount, m_uiSpawnCountRange);
  }

  if (m_uiTotalToSpawn == 0)
    return;

  if (m_SpawnDuration.IsZeroOrNegative())
  {
    Spawn(m_uiTotalToSpawn);
  }
  else
  {
    if (bFirstTime)
    {
      // this guarantees that next time OnTriggered() is called, one object gets spawned right away
      m_StartTime -= m_SpawnDuration / m_uiTotalToSpawn;
    }

    xiiMsgComponentInternalTrigger msg;
    PostMessage(msg, xiiTime::MakeZero());
  }
}

void xiiSpawnBoxComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  const xiiTime tNow    = GetWorld()->GetClock().GetAccumulatedTime();
  const xiiTime tActive = tNow - m_StartTime;
  const xiiTime tEnd    = m_StartTime + m_SpawnDuration;

  if (tNow >= tEnd)
  {
    if (m_uiSpawned < m_uiTotalToSpawn)
    {
      Spawn(m_uiTotalToSpawn - m_uiSpawned);
    }

    if (GetSpawnContinuously())
    {
      InternalStartSpawning(false);
    }

    return;
  }

  const auto uiTargetSpawnCount = xiiMath::Clamp<xiiUInt16>(static_cast<xiiUInt16>(((tActive.GetSeconds() / m_SpawnDuration.GetSeconds()) * m_uiTotalToSpawn)), 0, m_uiTotalToSpawn);

  if (m_uiSpawned < uiTargetSpawnCount)
  {
    Spawn(uiTargetSpawnCount - m_uiSpawned);
  }

  if (m_uiSpawned < m_uiTotalToSpawn)
  {
    // remaining time divided equally for the remaining spawns
    // this is to prevent a lot of unnecessary message sending at low spawn counts
    xiiTime tDelay = (tEnd - tNow) / (m_uiTotalToSpawn - m_uiSpawned);

    // prevent unnecessary high number of updates, rather spawn multiple objects within one frame
    tDelay = xiiMath::Max(tDelay, xiiTime::MakeFromMilliseconds(40)); // max 25 Hz

    xiiMsgComponentInternalTrigger msg;
    PostMessage(msg, tDelay);
  }
  else if (GetSpawnContinuously())
  {
    InternalStartSpawning(false);
  }
}

void xiiSpawnBoxComponent::Spawn(xiiUInt32 uiCount)
{
  if (uiCount == 0)
    return;

  m_uiSpawned += uiCount;

  if (!m_hPrefab.IsValid())
    return;

  xiiResourceLock<xiiPrefabResource> pResource(m_hPrefab, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pResource.GetAcquireResult() == xiiResourceAcquireResult::None)
    return;

  xiiPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  xiiRandom&         rnd    = GetWorld()->GetRandomNumberGenerator();
  const xiiTransform tOwner = GetOwner()->GetGlobalTransform();

  for (xiiUInt32 i = 0; i < uiCount; ++i)
  {
    xiiTransform tLocal  = xiiTransform::MakeIdentity();
    tLocal.m_vPosition.x = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.x, m_vHalfExtents.x));
    tLocal.m_vPosition.y = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.y, m_vHalfExtents.y));
    tLocal.m_vPosition.z = static_cast<float>(rnd.DoubleMinMax(-m_vHalfExtents.z, m_vHalfExtents.z));

    if (m_MaxRotationZ.GetRadian() > 0)
    {
      const xiiAngle rotationAngle = xiiAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(-m_MaxRotationZ.GetRadian(), +m_MaxRotationZ.GetRadian()));
      xiiQuat        qRot;
      qRot = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), rotationAngle);

      tLocal.m_qRotation = qRot;
    }

    if (m_MaxTiltZ.GetRadian() > 0)
    {
      const xiiAngle tiltTurnAngle = xiiAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, xiiMath::Pi<double>() * 2.0));
      xiiQuat        qTiltTurn;
      qTiltTurn = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), tiltTurnAngle);

      const xiiVec3 vTiltAxis = qTiltTurn * xiiVec3(1, 0, 0);

      const xiiAngle tiltAngle = xiiAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleMinMax(0.0, (double)m_MaxTiltZ.GetRadian()));
      xiiQuat        qTilt;
      qTilt = xiiQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);

      tLocal.m_qRotation = tLocal.m_qRotation * qTilt;
    }

    xiiTransform tGlobal = xiiTransform::MakeGlobalTransform(tOwner, tLocal);

    pResource->InstantiatePrefab(*GetWorld(), tGlobal, options);
  }
}
