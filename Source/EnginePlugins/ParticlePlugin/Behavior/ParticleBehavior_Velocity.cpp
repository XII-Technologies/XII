#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Velocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_Velocity, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_Velocity>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RiseSpeed", m_fRiseSpeed),
    XII_MEMBER_PROPERTY("Friction", m_fFriction)->AddAttributes(new xiiClampValueAttribute(0.0f, 100.0f)),
    XII_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_Velocity, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_Velocity>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleBehaviorFactory_Velocity::xiiParticleBehaviorFactory_Velocity()  = default;
xiiParticleBehaviorFactory_Velocity::~xiiParticleBehaviorFactory_Velocity() = default;

const xiiRTTI* xiiParticleBehaviorFactory_Velocity::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_Velocity>();
}

void xiiParticleBehaviorFactory_Velocity::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_Velocity* pBehavior = static_cast<xiiParticleBehavior_Velocity*>(pObject);

  pBehavior->m_fRiseSpeed     = m_fRiseSpeed;
  pBehavior->m_fFriction      = m_fFriction;
  pBehavior->m_fWindInfluence = m_fWindInfluence;


  pBehavior->m_pPhysicsModule = (xiiPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(xiiGetStaticRTTI<xiiPhysicsWorldModuleInterface>());
}

enum class BehaviorVelocityVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added rise speed and acceleration
  Version_3, // added wind influence

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void xiiParticleBehaviorFactory_Velocity::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = (int)BehaviorVelocityVersion::Version_Current;
  stream << uiVersion;

  stream << m_fRiseSpeed;
  stream << m_fFriction;

  // Version 3
  stream << m_fWindInfluence;
}

void xiiParticleBehaviorFactory_Velocity::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion <= (int)BehaviorVelocityVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fRiseSpeed;
  stream >> m_fFriction;

  if (uiVersion >= 3)
  {
    stream >> m_fWindInfluence;
  }
}

void xiiParticleBehaviorFactory_Velocity::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
}

void xiiParticleBehavior_Velocity::CreateRequiredStreams()
{
  CreateStream("Position", xiiProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void xiiParticleBehavior_Velocity::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Velocity");

  const float   tDiff = (float)m_TimeDiff.GetSeconds();
  const xiiVec3 vDown = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity().GetNormalized() : xiiVec3(0.0f, 0.0f, -1.0f);
  const xiiVec3 vRise = vDown * tDiff * -m_fRiseSpeed;

  auto    pOwner = GetOwnerEffect();
  xiiVec3 vWind(0);

  if (m_iWindSampleIdx >= 0)
  {
    xiiVec3 vCurWind = pOwner->GetWindSampleResult(m_iWindSampleIdx);
    vCurWind         = xiiMath::Lerp(m_vLastWind, vCurWind, tDiff);

    vWind = vCurWind * m_fWindInfluence * tDiff;

    m_vLastWind = vCurWind;

    m_iWindSampleIdx = -1;
  }

  if (m_fWindInfluence > 0)
  {
    m_iWindSampleIdx = pOwner->AddWindSampleLocation(GetOwnerSystem()->GetTransform().m_vPosition);
  }

  const xiiVec3 vAddPos0 = vRise + vWind;

  xiiSimdVec4f vAddPos;
  vAddPos.Load<3>(&vAddPos0.x);

  const float fFriction       = xiiMath::Clamp(m_fFriction, 0.0f, 100.0f);
  const float fFrictionFactor = xiiMath::Pow(0.5f, tDiff * fFriction);

  xiiProcessingStreamIterator<xiiSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  xiiProcessingStreamIterator<xiiVec3>      itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += vAddPos;
    itVelocity.Current() *= fFrictionFactor;

    itPosition.Advance();
    itVelocity.Advance();
  }
}

void xiiParticleBehavior_Velocity::RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<xiiPhysicsWorldModuleInterface>();
  pParticleModule->CacheWorldModule<xiiWindWorldModuleInterface>();
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Velocity);
