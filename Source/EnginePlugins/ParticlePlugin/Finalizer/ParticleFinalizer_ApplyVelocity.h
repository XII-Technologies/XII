#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizerFactory_ApplyVelocity final : public xiiParticleFinalizerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizerFactory_ApplyVelocity, xiiParticleFinalizerFactory);

public:
  xiiParticleFinalizerFactory_ApplyVelocity();

  virtual const xiiRTTI* GetFinalizerType() const override;
  virtual void           CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const override;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizer_ApplyVelocity final : public xiiParticleFinalizer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizer_ApplyVelocity, xiiParticleFinalizer);

public:
  xiiParticleFinalizer_ApplyVelocity();
  ~xiiParticleFinalizer_ApplyVelocity();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamPosition = nullptr;
  xiiProcessingStream* m_pStreamVelocity = nullptr;
};
