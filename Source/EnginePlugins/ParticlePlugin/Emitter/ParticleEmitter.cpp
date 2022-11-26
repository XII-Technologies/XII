#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitterFactory, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleEmitter, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiParticleEmitter* xiiParticleEmitterFactory::CreateEmitter(xiiParticleSystemInstance* pOwner) const
{
  const xiiRTTI* pRtti = GetEmitterType();

  xiiParticleEmitter* pEmitter = pRtti->GetAllocator()->Allocate<xiiParticleEmitter>();
  pEmitter->Reset(pOwner);

  CopyEmitterProperties(pEmitter, true);
  pEmitter->CreateRequiredStreams();

  return pEmitter;
}

bool xiiParticleEmitter::IsContinuous() const
{
  return false;
}

void xiiParticleEmitter::Process(xiiUInt64 uiNumElements) {}
void xiiParticleEmitter::ProcessEventQueue(xiiParticleEventQueue queue) {}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter);
