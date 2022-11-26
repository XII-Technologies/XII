#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_Continuous.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitterFactory_Continuous, 1, xiiRTTIDefaultAllocator<xiiParticleEmitterFactory_Continuous>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("StartDelay", m_StartDelay),

    XII_MEMBER_PROPERTY("SpawnCountPerSec", m_uiSpawnCountPerSec)->AddAttributes(new xiiDefaultValueAttribute(10)),
    XII_MEMBER_PROPERTY("SpawnCountPerSecRange", m_uiSpawnCountPerSecRange),
    XII_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),

    XII_ACCESSOR_PROPERTY("CountCurve", GetCountCurveFile, SetCountCurveFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    XII_MEMBER_PROPERTY("CurveDuration", m_CurveDuration)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::Seconds(10.0))),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitter_Continuous, 1, xiiRTTIDefaultAllocator<xiiParticleEmitter_Continuous>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleEmitterFactory_Continuous::xiiParticleEmitterFactory_Continuous()
{
  m_uiSpawnCountPerSec      = 10;
  m_uiSpawnCountPerSecRange = 0;

  m_CurveDuration = xiiTime::Seconds(10.0);
}


const xiiRTTI* xiiParticleEmitterFactory_Continuous::GetEmitterType() const
{
  return xiiGetStaticRTTI<xiiParticleEmitter_Continuous>();
}

void xiiParticleEmitterFactory_Continuous::CopyEmitterProperties(xiiParticleEmitter* pEmitter0, bool bFirstTime) const
{
  xiiParticleEmitter_Continuous* pEmitter = static_cast<xiiParticleEmitter_Continuous*>(pEmitter0);

  pEmitter->m_StartDelay = m_StartDelay;

  pEmitter->m_uiSpawnCountPerSec      = (xiiUInt32)(m_uiSpawnCountPerSec * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountPerSecRange = (xiiUInt32)(m_uiSpawnCountPerSecRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = xiiTempHashedString(m_sSpawnCountScaleParameter.GetData());

  pEmitter->m_hCountCurve   = m_hCountCurve;
  pEmitter->m_CurveDuration = xiiMath::Max(m_CurveDuration, xiiTime::Seconds(1.0));
}

void xiiParticleEmitterFactory_Continuous::QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs       = 0;
  out_uiMaxParticlesPerSecond = m_uiSpawnCountPerSec + (m_uiSpawnCountPerSecRange * 3 / 4); // don't be too pessimistic

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterContinuousVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added emitter start delay
  Version_5, // added spawn count scale param
  Version_6, // removed duration, switched to particles per second

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void xiiParticleEmitterFactory_Continuous::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)EmitterContinuousVersion::Version_Current;
  stream << uiVersion;

  // Version 4
  stream << m_StartDelay;

  // Version 6
  stream << m_uiSpawnCountPerSec;
  stream << m_uiSpawnCountPerSecRange;

  // Version 2
  stream << m_hCountCurve;
  stream << m_CurveDuration;

  // Version 5
  stream << m_sSpawnCountScaleParameter;
}

void xiiParticleEmitterFactory_Continuous::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)EmitterContinuousVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 3 && uiVersion < 6)
  {
    xiiTime duraton;
    stream >> duraton;
  }

  if (uiVersion >= 4)
  {
    stream >> m_StartDelay;
  }

  stream >> m_uiSpawnCountPerSec;
  stream >> m_uiSpawnCountPerSecRange;

  if (uiVersion < 6)
  {
    xiiVarianceTypeFloat interval;
    stream >> interval.m_Value;
    stream >> interval.m_fVariance;
  }

  if (uiVersion >= 2)
  {
    stream >> m_hCountCurve;
    stream >> m_CurveDuration;
  }

  if (uiVersion >= 5)
  {
    stream >> m_sSpawnCountScaleParameter;
  }
}

void xiiParticleEmitterFactory_Continuous::SetCountCurveFile(const char* szFile)
{
  xiiCurve1DResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiCurve1DResource>(szFile);
  }

  m_hCountCurve = hResource;
}

const char* xiiParticleEmitterFactory_Continuous::GetCountCurveFile() const
{
  if (!m_hCountCurve.IsValid())
    return "";

  return m_hCountCurve.GetResourceID();
}

void xiiParticleEmitter_Continuous::OnFinalize()
{
  m_CountCurveTime.SetZero();
  m_fCurSpawnPerSec = (float)GetRNG().DoubleInRange(m_uiSpawnCountPerSec, m_uiSpawnCountPerSecRange);
  m_TimeSinceRandom.SetZero();
  m_fCurSpawnCounter = 0;
}

xiiParticleEmitterState xiiParticleEmitter_Continuous::IsFinished()
{
  return xiiParticleEmitterState::Active;
}

xiiUInt32 xiiParticleEmitter_Continuous::ComputeSpawnCount(const xiiTime& tDiff)
{
  XII_PROFILE_SCOPE("PFX: Continuous - Spawn Count ");

  // delay before the emitter becomes active
  if (m_StartDelay.IsPositive())
  {
    m_StartDelay -= tDiff;
    return 0;
  }

  m_TimeSinceRandom += tDiff;
  m_CountCurveTime += tDiff;

  if (m_TimeSinceRandom >= xiiTime::Milliseconds(200))
  {
    m_TimeSinceRandom.SetZero();
    m_fCurSpawnPerSec = (float)GetRNG().DoubleInRange(m_uiSpawnCountPerSec, m_uiSpawnCountPerSecRange);
  }


  float fSpawnFactor = 1.0f;

  if (m_hCountCurve.IsValid())
  {
    xiiResourceLock<xiiCurve1DResource> pCurve(m_hCountCurve, xiiResourceAcquireMode::BlockTillLoaded);

    if (!pCurve->GetDescriptor().m_Curves.IsEmpty())
    {
      while (m_CountCurveTime > m_CurveDuration)
        m_CountCurveTime -= m_CurveDuration;

      const auto& curve = pCurve->GetDescriptor().m_Curves[0];

      const double normPos = (float)(m_CountCurveTime.GetSeconds() / m_CurveDuration.GetSeconds());
      const double evalPos = curve.ConvertNormalizedPos(normPos);

      fSpawnFactor = (float)xiiMath::Max(0.0, curve.Evaluate(evalPos));
    }
  }

  const float spawnCountScale = xiiMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;


  m_fCurSpawnCounter += fSpawnFactor * m_fCurSpawnPerSec * (float)tDiff.GetSeconds();

  const xiiUInt32 uiSpawn = (xiiUInt32)m_fCurSpawnCounter;
  m_fCurSpawnCounter -= uiSpawn;

  return uiSpawn;
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_Continuous);
