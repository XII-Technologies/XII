#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleEmitterFactory_Burst final : public xiiParticleEmitterFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitterFactory_Burst, xiiParticleEmitterFactory);

public:
  xiiParticleEmitterFactory_Burst();

  virtual const xiiRTTI* GetEmitterType() const override;
  virtual void           CopyEmitterProperties(xiiParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void           QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

public:
  xiiTime m_Duration;
  xiiTime m_StartDelay;

  xiiUInt32 m_uiSpawnCountMin;
  xiiUInt32 m_uiSpawnCountRange;
  xiiString m_sSpawnCountScaleParameter;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleEmitter_Burst final : public xiiParticleEmitter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitter_Burst, xiiParticleEmitter);

public:
  xiiTime m_Duration;   // overall duration in which the emitter is considered active, 0 for single frame
  xiiTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  xiiUInt32           m_uiSpawnCountMin;
  xiiUInt32           m_uiSpawnCountRange;
  xiiTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void OnFinalize() override;
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}

  virtual xiiParticleEmitterState IsFinished() override;
  virtual xiiUInt32               ComputeSpawnCount(const xiiTime& tDiff) override;

  xiiUInt32 m_uiSpawnCountLeft = 0;
  float     m_fSpawnPerSecond  = 0;
  float     m_fSpawnAccu       = 0;
};
