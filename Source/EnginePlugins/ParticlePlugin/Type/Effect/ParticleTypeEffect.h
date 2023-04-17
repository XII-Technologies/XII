#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using xiiParticleEffectResourceHandle = xiiTypedResourceHandle<class xiiParticleEffectResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeEffectFactory final : public xiiParticleTypeFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeEffectFactory, xiiParticleTypeFactory);

public:
  xiiParticleTypeEffectFactory();
  ~xiiParticleTypeEffectFactory();

  virtual const xiiRTTI* GetTypeType() const override;
  virtual void           CopyTypeProperties(xiiParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  xiiString m_sEffect;
  xiiString m_sSharedInstanceName; // to be removed
};

class XII_PARTICLEPLUGIN_DLL xiiParticleTypeEffect final : public xiiParticleType
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleTypeEffect, xiiParticleType);

public:
  xiiParticleTypeEffect();
  ~xiiParticleTypeEffect();

  xiiParticleEffectResourceHandle m_hEffect;
  // xiiString m_sSharedInstanceName;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return m_fMaxEffectRadius; }

protected:
  friend class xiiParticleTypeEffectFactory;

  virtual void OnReset() override;
  virtual void Process(xiiUInt64 uiNumElements) override;
  void         OnParticleDeath(const xiiStreamGroupElementRemovedEvent& e);
  void         ClearEffects(bool bInterruptImmediately);

  float                m_fMaxEffectRadius = 1.0f;
  xiiProcessingStream* m_pStreamPosition  = nullptr;
  xiiProcessingStream* m_pStreamEffectID  = nullptr;
};
