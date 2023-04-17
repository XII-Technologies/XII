#pragma once

#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_BoxPosition final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_BoxPosition, xiiParticleInitializerFactory);

public:
  xiiParticleInitializerFactory_BoxPosition();

  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float          GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

public:
  xiiVec3   m_vPositionOffset;
  xiiVec3   m_vSize;
  xiiString m_sScaleXParameter;
  xiiString m_sScaleYParameter;
  xiiString m_sScaleZParameter;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_BoxPosition final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_BoxPosition, xiiParticleInitializer);

public:
  xiiVec3 m_vPositionOffset;
  xiiVec3 m_vSize;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamPosition;
};
