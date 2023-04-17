#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_SpherePosition final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_SpherePosition, xiiParticleInitializerFactory);

public:
  xiiParticleInitializerFactory_SpherePosition();

  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float          GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const override;

public:
  xiiVec3              m_vPositionOffset;
  float                m_fRadius;
  bool                 m_bSpawnOnSurface;
  bool                 m_bSetVelocity;
  xiiVarianceTypeFloat m_Speed;
  xiiString            m_sScaleRadiusParameter;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_SpherePosition final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_SpherePosition, xiiParticleInitializer);

public:
  xiiVec3              m_vPositionOffset;
  float                m_fRadius;
  bool                 m_bSpawnOnSurface;
  bool                 m_bSetVelocity;
  xiiVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamPosition;
  xiiProcessingStream* m_pStreamVelocity;
};
