#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizerFactory_LastPosition final : public xiiParticleFinalizerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizerFactory_LastPosition, xiiParticleFinalizerFactory);

public:
  xiiParticleFinalizerFactory_LastPosition();

  virtual const xiiRTTI* GetFinalizerType() const override;
  virtual void           CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const override;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizer_LastPosition final : public xiiParticleFinalizer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizer_LastPosition, xiiParticleFinalizer);

public:
  xiiParticleFinalizer_LastPosition();
  ~xiiParticleFinalizer_LastPosition();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamPosition     = nullptr;
  xiiProcessingStream* m_pStreamLastPosition = nullptr;
};
