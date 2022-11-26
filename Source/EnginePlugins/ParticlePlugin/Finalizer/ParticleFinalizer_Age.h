#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class xiiPhysicsWorldModuleInterface;

class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizerFactory_Age final : public xiiParticleFinalizerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizerFactory_Age, xiiParticleFinalizerFactory);

public:
  xiiParticleFinalizerFactory_Age();

  virtual const xiiRTTI* GetFinalizerType() const override;
  virtual void           CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const override;

  xiiVarianceTypeTime m_LifeTime;
  xiiString           m_sOnDeathEvent;
  xiiString           m_sLifeScaleParameter;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizer_Age final : public xiiParticleFinalizer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizer_Age, xiiParticleFinalizer);

public:
  xiiParticleFinalizer_Age();
  ~xiiParticleFinalizer_Age();

  virtual void CreateRequiredStreams() override;

  xiiVarianceTypeTime m_LifeTime;
  xiiTempHashedString m_sOnDeathEvent;
  xiiTempHashedString m_sLifeScaleParameter;

protected:
  friend class xiiParticleFinalizerFactory_Age;

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;
  virtual void Process(xiiUInt64 uiNumElements) override;
  void         OnParticleDeath(const xiiStreamGroupElementRemovedEvent& e);

  bool                 m_bHasOnDeathEventHandler = false;
  xiiProcessingStream* m_pStreamLifeTime         = nullptr;
  xiiProcessingStream* m_pStreamPosition         = nullptr;
  xiiProcessingStream* m_pStreamVelocity         = nullptr;
};
