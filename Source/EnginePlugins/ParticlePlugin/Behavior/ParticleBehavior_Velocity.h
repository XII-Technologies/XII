#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class xiiPhysicsWorldModuleInterface;
class xiiWindWorldModuleInterface;

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_Velocity final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_Velocity, xiiParticleBehaviorFactory);

public:
  xiiParticleBehaviorFactory_Velocity();
  ~xiiParticleBehaviorFactory_Velocity();

  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const override;

  float m_fRiseSpeed     = 0;
  float m_fFriction      = 0;
  float m_fWindInfluence = 0;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_Velocity final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_Velocity, xiiParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fRiseSpeed     = 0;
  float m_fFriction      = 0;
  float m_fWindInfluence = 0;

protected:
  friend class xiiParticleBehaviorFactory_Velocity;

  virtual void Process(xiiUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule) override;

  // used to rise/fall along the gravity vector
  xiiPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;
  //xiiWindWorldModuleInterface* m_pWindModule = nullptr;
  xiiInt32 m_iWindSampleIdx = -1;

  xiiProcessingStream* m_pStreamPosition;
  xiiProcessingStream* m_pStreamVelocity;

  xiiVec3 m_vLastWind = xiiVec3::ZeroVector();
};
