#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Distance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitterFactory_Distance, 1, xiiRTTIDefaultAllocator<xiiParticleEmitterFactory_Distance>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("DistanceThreshold", m_fDistanceThreshold)->AddAttributes(new xiiDefaultValueAttribute(0.1f), new xiiClampValueAttribute(0.01f, 100.0f)),
    XII_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new xiiDefaultValueAttribute(1)),
    XII_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    XII_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitter_Distance, 1, xiiRTTIDefaultAllocator<xiiParticleEmitter_Distance>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEmitterFactory_Distance::xiiParticleEmitterFactory_Distance() = default;

const xiiRTTI* xiiParticleEmitterFactory_Distance::GetEmitterType() const
{
  return xiiGetStaticRTTI<xiiParticleEmitter_Distance>();
}

void xiiParticleEmitterFactory_Distance::CopyEmitterProperties(xiiParticleEmitter* pEmitter0, bool bFirstTime) const
{
  xiiParticleEmitter_Distance* pEmitter = static_cast<xiiParticleEmitter_Distance*>(pEmitter0);

  pEmitter->m_fDistanceThresholdSQR = xiiMath::Square(m_fDistanceThreshold);

  pEmitter->m_uiSpawnCountMin   = (xiiUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (xiiUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = xiiTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void xiiParticleEmitterFactory_Distance::QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs       = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 10; // assume that this won't fire more than 10 times per second

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterDistanceVersion
{
  Version_1 = 1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void xiiParticleEmitterFactory_Distance::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)EmitterDistanceVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_fDistanceThreshold;
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void xiiParticleEmitterFactory_Distance::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)EmitterDistanceVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fDistanceThreshold;
  inout_stream >> m_uiSpawnCountMin;
  inout_stream >> m_uiSpawnCountRange;
  inout_stream >> m_sSpawnCountScaleParameter;
}

void xiiParticleEmitter_Distance::CreateRequiredStreams() {}
void xiiParticleEmitter_Distance::InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) {}

bool xiiParticleEmitter_Distance::IsContinuous() const
{
  return true;
}

void xiiParticleEmitter_Distance::OnFinalize()
{
  // do not use the System transform, because then this would not work with local space simulation
  m_vLastSpawnPosition = GetOwnerEffect()->GetTransform().m_vPosition;
  m_bFirstUpdate       = true;

  if (GetOwnerEffect()->IsSharedEffect())
  {
    xiiLog::Warning("Particle emitters of type 'Distance' do not work for shared particle effect instances.");
  }
}

xiiParticleEmitterState xiiParticleEmitter_Distance::IsFinished()
{
  return xiiParticleEmitterState::Active;
}

xiiUInt32 xiiParticleEmitter_Distance::ComputeSpawnCount(const xiiTime& tDiff)
{
  const xiiVec3 vCurPos = GetOwnerEffect()->GetTransform().m_vPosition;

  if ((m_vLastSpawnPosition - vCurPos).GetLengthSquared() < m_fDistanceThresholdSQR)
    return 0;

  m_vLastSpawnPosition = vCurPos;

  if (m_bFirstUpdate)
  {
    m_bFirstUpdate = false;
    return 0;
  }

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = xiiMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  xiiUInt32 uiSpawn = m_uiSpawnCountMin;

  if (m_uiSpawnCountRange > 0)
    uiSpawn += GetRNG().UIntInRange(m_uiSpawnCountRange);

  uiSpawn = static_cast<xiiUInt32>((float)uiSpawn * fSpawnFactor);

  return uiSpawn;
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Distance);
