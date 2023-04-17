#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_PullAlong.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_PullAlong, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_PullAlong>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_PullAlong, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_PullAlong>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleBehaviorFactory_PullAlong::xiiParticleBehaviorFactory_PullAlong() {}

const xiiRTTI* xiiParticleBehaviorFactory_PullAlong::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_PullAlong>();
}

void xiiParticleBehaviorFactory_PullAlong::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_PullAlong* pBehavior = static_cast<xiiParticleBehavior_PullAlong*>(pObject);

  pBehavior->m_fStrength = xiiMath::Clamp(m_fStrength, 0.0f, 1.0f);
}

enum class BehaviorPullAlongVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleBehaviorFactory_PullAlong::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)BehaviorPullAlongVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fStrength;
}

void xiiParticleBehaviorFactory_PullAlong::Load(xiiStreamReader& inout_stream)
{
  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)BehaviorPullAlongVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fStrength;
}

void xiiParticleBehavior_PullAlong::CreateRequiredStreams()
{
  m_bFirstTime = true;
  m_vApplyPull.SetZero();

  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void xiiParticleBehavior_PullAlong::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: PullAlong");

  if (m_vApplyPull.IsZero())
    return;

  xiiProcessingStreamIterator<xiiSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  xiiSimdVec4f                              pull;
  pull.Load<3>(&m_vApplyPull.x);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += pull;

    itPosition.Advance();
  }
}

void xiiParticleBehavior_PullAlong::StepParticleSystem(const xiiTime& tDiff, xiiUInt32 uiNumNewParticles)
{
  const xiiVec3 vPos = GetOwnerSystem()->GetTransform().m_vPosition;

  if (!m_bFirstTime)
  {
    m_vApplyPull = (vPos - m_vLastEmitterPosition) * m_fStrength;
  }
  else
  {
    m_bFirstTime = false;
    m_vApplyPull.SetZero();
  }

  m_vLastEmitterPosition = vPos;
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_PullAlong);
