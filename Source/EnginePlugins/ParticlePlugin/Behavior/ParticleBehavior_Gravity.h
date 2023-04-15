#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class xiiPhysicsWorldModuleInterface;

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_Gravity final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_Gravity, xiiParticleBehaviorFactory);

public:
  xiiParticleBehaviorFactory_Gravity();

  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

public:
  float m_fGravityFactor;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_Gravity final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_Gravity, xiiParticleBehavior);

public:
  float m_fGravityFactor;

  virtual void CreateRequiredStreams() override;

protected:
  friend class xiiParticleBehaviorFactory_Gravity;

  virtual void Process(xiiUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule) override;

  xiiPhysicsWorldModuleInterface* m_pPhysicsModule;

  xiiProcessingStream* m_pStreamVelocity;
};
