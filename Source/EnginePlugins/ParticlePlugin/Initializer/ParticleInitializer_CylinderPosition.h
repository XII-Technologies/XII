#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_CylinderPosition final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_CylinderPosition, xiiParticleInitializerFactory);

public:
  xiiParticleInitializerFactory_CylinderPosition();

  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;
  virtual float          GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_finalizerDeps) const override;

public:
  xiiVec3              m_vPositionOffset;
  float                m_fRadius;
  float                m_fHeight;
  bool                 m_bSpawnOnSurface;
  bool                 m_bSetVelocity;
  xiiVarianceTypeFloat m_Speed;
  xiiString            m_sScaleRadiusParameter;
  xiiString            m_sScaleHeightParameter;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_CylinderPosition final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_CylinderPosition, xiiParticleInitializer);

public:
  xiiVec3              m_vPositionOffset;
  float                m_fRadius;
  float                m_fHeight;
  bool                 m_bSpawnOnSurface;
  bool                 m_bSetVelocity;
  xiiVarianceTypeFloat m_Speed;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamPosition;
  xiiProcessingStream* m_pStreamVelocity;
};
