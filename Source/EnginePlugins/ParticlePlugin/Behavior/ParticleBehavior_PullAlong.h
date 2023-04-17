#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class xiiPhysicsWorldModuleInterface;

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_PullAlong final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_PullAlong, xiiParticleBehaviorFactory);

public:
  xiiParticleBehaviorFactory_PullAlong();

  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  float m_fStrength;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_PullAlong final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_PullAlong, xiiParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fStrength = 0.5;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;
  virtual void StepParticleSystem(const xiiTime& tDiff, xiiUInt32 uiNumNewParticles) override;

  bool                 m_bFirstTime = true;
  xiiVec3              m_vLastEmitterPosition;
  xiiVec3              m_vApplyPull;
  xiiProcessingStream* m_pStreamPosition;
};
