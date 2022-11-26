#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory_VelocityCone final : public xiiParticleInitializerFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory_VelocityCone, xiiParticleInitializerFactory);

public:
  xiiParticleInitializerFactory_VelocityCone();

  virtual const xiiRTTI* GetInitializerType() const override;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(xiiStreamWriter& stream) const override;
  virtual void Load(xiiStreamReader& stream) override;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const override;

public:
  xiiAngle             m_Angle;
  xiiVarianceTypeFloat m_Speed;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer_VelocityCone final : public xiiParticleInitializer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer_VelocityCone, xiiParticleInitializer);

public:
  xiiAngle             m_Angle;
  xiiVarianceTypeFloat m_Speed;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  xiiProcessingStream* m_pStreamVelocity;
};
