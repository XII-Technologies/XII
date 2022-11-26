#pragma once

#include <Foundation/Strings/String.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class xiiPhysicsWorldModuleInterface;

struct XII_PARTICLEPLUGIN_DLL xiiParticleRaycastHitReaction
{
  typedef xiiUInt8 StorageType;

  enum Enum
  {
    Bounce,
    Die,
    Stop,

    Default = Bounce
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_PARTICLEPLUGIN_DLL, xiiParticleRaycastHitReaction);

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_Raycast final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_Raycast, xiiParticleBehaviorFactory);

public:
  xiiParticleBehaviorFactory_Raycast();
  ~xiiParticleBehaviorFactory_Raycast();

  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  xiiEnum<xiiParticleRaycastHitReaction> m_Reaction;
  xiiUInt8                               m_uiCollisionLayer = 0;
  xiiString                              m_sOnCollideEvent;
  float                                  m_fBounceFactor = 0.6f;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_Raycast final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_Raycast, xiiParticleBehavior);

public:
  xiiParticleBehavior_Raycast();

  virtual void CreateRequiredStreams() override;

  xiiEnum<xiiParticleRaycastHitReaction> m_Reaction;
  xiiUInt8                               m_uiCollisionLayer = 0;
  xiiTempHashedString                    m_sOnCollideEvent;
  float                                  m_fBounceFactor = 0.6f;

protected:
  friend class xiiParticleBehaviorFactory_Raycast;

  virtual void Process(xiiUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(xiiParticleWorldModule* pParticleModule) override;

  xiiPhysicsWorldModuleInterface* m_pPhysicsModule;

  xiiProcessingStream* m_pStreamPosition     = nullptr;
  xiiProcessingStream* m_pStreamLastPosition = nullptr;
  xiiProcessingStream* m_pStreamVelocity     = nullptr;
};
