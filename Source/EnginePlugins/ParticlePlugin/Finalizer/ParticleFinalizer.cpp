#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizerFactory, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleFinalizer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleFinalizer* xiiParticleFinalizerFactory::CreateFinalizer(xiiParticleSystemInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetFinalizerType();

  xiiParticleFinalizer* pFinalizer = pRtti->GetAllocator()->Allocate<xiiParticleFinalizer>();
  pFinalizer->Reset(pOwner);

  CopyFinalizerProperties(pFinalizer, true);
  pFinalizer->CreateRequiredStreams();

  return pFinalizer;
}

xiiParticleFinalizer::xiiParticleFinalizer()
{
  // run after the behaviors, before the types
  m_fPriority = +500.0f;
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer);

