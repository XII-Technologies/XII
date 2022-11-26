#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehaviorFactory, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleBehavior, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;


xiiParticleBehavior* xiiParticleBehaviorFactory::CreateBehavior(xiiParticleSystemInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetBehaviorType();

  xiiParticleBehavior* pBehavior = pRtti->GetAllocator()->Allocate<xiiParticleBehavior>();
  pBehavior->Reset(pOwner);

  CopyBehaviorProperties(pBehavior, true);
  pBehavior->CreateRequiredStreams();

  return pBehavior;
}

xiiParticleBehavior::xiiParticleBehavior()
{
  // run after the initializers, before the types
  m_fPriority = 0.0f;
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior);
