#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class xiiPhysicsWorldModuleInterface;

class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizerFactory_Volume final : public xiiParticleFinalizerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizerFactory_Volume, xiiParticleFinalizerFactory);

public:
  xiiParticleFinalizerFactory_Volume();
  ~xiiParticleFinalizerFactory_Volume();

  virtual const xiiRTTI* GetFinalizerType() const override;
  virtual void           CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const override;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizer_Volume final : public xiiParticleFinalizer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizer_Volume, xiiParticleFinalizer);

public:
  xiiParticleFinalizer_Volume();
  ~xiiParticleFinalizer_Volume();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream*       m_pStreamPosition = nullptr;
  const xiiProcessingStream* m_pStreamSize     = nullptr;
};
