#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory_FadeOut final : public xiiParticleBehaviorFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory_FadeOut, xiiParticleBehaviorFactory);

public:
  virtual const xiiRTTI* GetBehaviorType() const override;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  float m_fStartAlpha = 1.0f;
  float m_fExponent   = 1.0f;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior_FadeOut final : public xiiParticleBehavior
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior_FadeOut, xiiParticleBehavior);

public:
  float m_fStartAlpha = 1.0f;
  float m_fExponent   = 1.0f;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamLifeTime         = nullptr;
  xiiProcessingStream* m_pStreamColor            = nullptr;
  xiiUInt8             m_uiFirstToUpdate         = 0;
  xiiUInt8             m_uiCurrentUpdateInterval = 2;
};
