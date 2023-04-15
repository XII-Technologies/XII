#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class xiiPhysicsWorldModuleInterface;

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_Bounds final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_Bounds, xiiParticleBehaviorFactory);

public:
  xiiParticleBehaviorFactory_Bounds();

  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  xiiVec3                             m_vPositionOffset;
  xiiVec3                             m_vBoxExtents;
  xiiEnum<xiiParticleOutOfBoundsMode> m_OutOfBoundsMode;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_Bounds final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_Bounds, xiiParticleBehavior);

public:
  xiiVec3                             m_vPositionOffset;
  xiiVec3                             m_vBoxExtents;
  xiiEnum<xiiParticleOutOfBoundsMode> m_OutOfBoundsMode;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

  xiiProcessingStream* m_pStreamPosition     = nullptr;
  xiiProcessingStream* m_pStreamLastPosition = nullptr;
};
