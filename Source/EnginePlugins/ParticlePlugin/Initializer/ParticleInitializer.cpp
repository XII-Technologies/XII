#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializerFactory, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleInitializer, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleInitializer* xiiParticleInitializerFactory::CreateInitializer(xiiParticleSystemInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetInitializerType();

  xiiParticleInitializer* pInitializer = pRtti->GetAllocator()->Allocate<xiiParticleInitializer>();
  pInitializer->Reset(pOwner);

  CopyInitializerProperties(pInitializer, true);
  pInitializer->CreateRequiredStreams();

  return pInitializer;
}

float xiiParticleInitializerFactory::GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const
{
  return 1.0f;
}

xiiParticleInitializer::xiiParticleInitializer()
{
  // run these early, but after the stream default initializers
  m_fPriority = -500.0f;
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer);
