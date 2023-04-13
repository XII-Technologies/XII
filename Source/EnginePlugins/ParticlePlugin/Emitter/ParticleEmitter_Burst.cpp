#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Burst.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitterFactory_Burst, 1, xiiRTTIDefaultAllocator<xiiParticleEmitterFactory_Burst>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Duration", m_Duration),
    XII_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    XII_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new xiiDefaultValueAttribute(10)),
    XII_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    XII_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitter_Burst, 1, xiiRTTIDefaultAllocator<xiiParticleEmitter_Burst>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEmitterFactory_Burst::xiiParticleEmitterFactory_Burst()
{
  m_uiSpawnCountMin   = 10;
  m_uiSpawnCountRange = 0;
}

const xiiRTTI* xiiParticleEmitterFactory_Burst::GetEmitterType() const
{
  return xiiGetStaticRTTI<xiiParticleEmitter_Burst>();
}

void xiiParticleEmitterFactory_Burst::CopyEmitterProperties(xiiParticleEmitter* pEmitter0, bool bFirstTime) const
{
  xiiParticleEmitter_Burst* pEmitter = static_cast<xiiParticleEmitter_Burst*>(pEmitter0);

  pEmitter->m_Duration   = m_Duration;
  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountMin           = (xiiUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange         = (xiiUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_sSpawnCountScaleParameter = xiiTempHashedString(m_sSpawnCountScaleParameter.GetData());
}


void xiiParticleEmitterFactory_Burst::QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs       = m_uiSpawnCountMin + m_uiSpawnCountRange;
  out_uiMaxParticlesPerSecond = 0;

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterBurstVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void xiiParticleEmitterFactory_Burst::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)EmitterBurstVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_Duration;
  stream << m_StartDelay;
  stream << m_uiSpawnCountMin;
  stream << m_uiSpawnCountRange;
  stream << m_sSpawnCountScaleParameter;
}

void xiiParticleEmitterFactory_Burst::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)EmitterBurstVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_Duration;
  stream >> m_StartDelay;
  stream >> m_uiSpawnCountMin;
  stream >> m_uiSpawnCountRange;
  stream >> m_sSpawnCountScaleParameter;
}

void xiiParticleEmitter_Burst::OnFinalize()
{
  float fSpawnFactor = 1.0f;

  const float spawnCountScale = xiiMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  xiiRandom& rng = GetRNG();

  m_uiSpawnCountLeft = (xiiUInt32)(rng.IntInRange(m_uiSpawnCountMin, 1 + m_uiSpawnCountRange) * fSpawnFactor);

  m_fSpawnAccu      = 0;
  m_fSpawnPerSecond = 0;

  if (!m_Duration.IsZero())
  {
    m_fSpawnPerSecond = m_uiSpawnCountLeft / (float)m_Duration.GetSeconds();
  }
}

xiiParticleEmitterState xiiParticleEmitter_Burst::IsFinished()
{
  return (m_uiSpawnCountLeft == 0) ? xiiParticleEmitterState::Finished : xiiParticleEmitterState::Active;
}

xiiUInt32 xiiParticleEmitter_Burst::ComputeSpawnCount(const xiiTime& tDiff)
{
  XII_PROFILE_SCOPE("PFX: Burst - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  xiiUInt32 uiSpawn = 0;

  if (m_Duration.IsZero())
  {
    uiSpawn            = m_uiSpawnCountLeft;
    m_uiSpawnCountLeft = 0;
  }
  else
  {
    m_fSpawnAccu += (float)tDiff.GetSeconds() * m_fSpawnPerSecond;
    uiSpawn = (xiiUInt32)m_fSpawnAccu;
    uiSpawn = xiiMath::Min(uiSpawn, m_uiSpawnCountLeft);

    m_fSpawnAccu -= uiSpawn;
    m_uiSpawnCountLeft -= uiSpawn;
  }

  return uiSpawn;
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Burst);

