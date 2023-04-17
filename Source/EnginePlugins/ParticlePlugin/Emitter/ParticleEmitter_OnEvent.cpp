#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_OnEvent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitterFactory_OnEvent, 1, xiiRTTIDefaultAllocator<xiiParticleEmitterFactory_OnEvent>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("EventName", m_sEventName),
    XII_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new xiiDefaultValueAttribute(1)),
    XII_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    XII_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitter_OnEvent, 1, xiiRTTIDefaultAllocator<xiiParticleEmitter_OnEvent>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEmitterFactory_OnEvent::xiiParticleEmitterFactory_OnEvent()  = default;
xiiParticleEmitterFactory_OnEvent::~xiiParticleEmitterFactory_OnEvent() = default;

const xiiRTTI* xiiParticleEmitterFactory_OnEvent::GetEmitterType() const
{
  return xiiGetStaticRTTI<xiiParticleEmitter_OnEvent>();
}

void xiiParticleEmitterFactory_OnEvent::CopyEmitterProperties(xiiParticleEmitter* pEmitter0, bool bFirstTime) const
{
  xiiParticleEmitter_OnEvent* pEmitter = static_cast<xiiParticleEmitter_OnEvent*>(pEmitter0);

  pEmitter->m_sEventName = xiiTempHashedString(m_sEventName.GetData());

  pEmitter->m_uiSpawnCountMin   = (xiiUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (xiiUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = xiiTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void xiiParticleEmitterFactory_OnEvent::QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs       = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 16; // some wild guess

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterOnEventVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void xiiParticleEmitterFactory_OnEvent::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)EmitterOnEventVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventName;

  // Version 2
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void xiiParticleEmitterFactory_OnEvent::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)EmitterOnEventVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sEventName;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiSpawnCountMin;
    inout_stream >> m_uiSpawnCountRange;
    inout_stream >> m_sSpawnCountScaleParameter;
  }
}

xiiParticleEmitterState xiiParticleEmitter_OnEvent::IsFinished()
{
  return xiiParticleEmitterState::OnlyReacting;
}

xiiUInt32 xiiParticleEmitter_OnEvent::ComputeSpawnCount(const xiiTime& tDiff)
{
  if (!m_bSpawn)
    return 0;

  m_bSpawn = false;

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = xiiMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  xiiRandom& rng = GetRNG();

  return static_cast<xiiUInt32>((m_uiSpawnCountMin + GetRNG().UIntInRange(1 + m_uiSpawnCountRange)) * fSpawnFactor);
}

void xiiParticleEmitter_OnEvent::ProcessEventQueue(xiiParticleEventQueue queue)
{
  if (m_bSpawn)
    return;

  for (const xiiParticleEvent& e : queue)
  {
    if (e.m_EventType == m_sEventName) // this is the event type we are waiting for!
    {
      m_bSpawn = true;
      return;
    }
  }
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
