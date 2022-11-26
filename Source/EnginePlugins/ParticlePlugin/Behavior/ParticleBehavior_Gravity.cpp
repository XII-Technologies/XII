#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Gravity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory_Gravity, 1, xiiRTTIDefaultAllocator<xiiParticleBehaviorFactory_Gravity>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("GravityFactor", m_fGravityFactor)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior_Gravity, 1, xiiRTTIDefaultAllocator<xiiParticleBehavior_Gravity>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleBehaviorFactory_Gravity::xiiParticleBehaviorFactory_Gravity()
{
  m_fGravityFactor = 1.0f;
}

const xiiRTTI* xiiParticleBehaviorFactory_Gravity::GetBehaviorType() const
{
  return xiiGetStaticRTTI<xiiParticleBehavior_Gravity>();
}

void xiiParticleBehaviorFactory_Gravity::CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const
{
  xiiParticleBehavior_Gravity* pBehavior = static_cast<xiiParticleBehavior_Gravity*>(pObject);

  pBehavior->m_fGravityFactor = m_fGravityFactor;

  pBehavior->m_pPhysicsModule = (xiiPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(xiiGetStaticRTTI<xiiPhysicsWorldModuleInterface>());
}

void xiiParticleBehaviorFactory_Gravity::Save(xiiStreamWriter& stream) const
{
  const xiiUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_fGravityFactor;
}

void xiiParticleBehaviorFactory_Gravity::Load(xiiStreamReader& stream)
{
  xiiUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_fGravityFactor;
}

void xiiParticleBehaviorFactory_Gravity::QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(xiiGetStaticRTTI<xiiParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void xiiParticleBehavior_Gravity::CreateRequiredStreams()
{
  CreateStream("Velocity", xiiProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void xiiParticleBehavior_Gravity::Process(xiiUInt64 uiNumElements)
{
  XII_PROFILE_SCOPE("PFX: Gravity");

  const xiiVec3 vGravity = m_pPhysicsModule != nullptr ? m_pPhysicsModule->GetGravity() : xiiVec3(0.0f, 0.0f, -10.0f);

  const float   tDiff      = (float)m_TimeDiff.GetSeconds();
  const xiiVec3 addGravity = vGravity * m_fGravityFactor * tDiff;

  xiiProcessingStreamIterator<xiiVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  while (!itVelocity.HasReachedEnd())
  {
    itVelocity.Current() += addGravity;

    itVelocity.Advance();
  }
}

void xiiParticleBehavior_Gravity::RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<xiiPhysicsWorldModuleInterface>();
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Gravity);
