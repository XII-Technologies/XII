#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_Flies final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_Flies, xiiParticleBehaviorFactory);

public:
  xiiParticleBehaviorFactory_Flies();
  ~xiiParticleBehaviorFactory_Flies();

  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  float    m_fSpeed              = 0.2f;
  float    m_fPathLength         = 0.2f;
  float    m_fMaxEmitterDistance = 0.5f;
  xiiAngle m_MaxSteeringAngle    = xiiAngle::Degree(30);
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_Flies final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_Flies, xiiParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float    m_fSpeed              = 0.2f;
  float    m_fPathLength         = 0.2f;
  float    m_fMaxEmitterDistance = 0.5f;
  xiiAngle m_MaxSteeringAngle    = xiiAngle::Degree(30);

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamPosition = nullptr;
  xiiProcessingStream* m_pStreamVelocity = nullptr;

  xiiTime m_TimeToChangeDir;
};
