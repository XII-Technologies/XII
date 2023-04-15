#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleEmitterFactory_Distance final : public xiiParticleEmitterFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitterFactory_Distance, xiiParticleEmitterFactory);

public:
  xiiParticleEmitterFactory_Distance();

  virtual const xiiRTTI* GetEmitterType() const override;
  virtual void           CopyEmitterProperties(xiiParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void           QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

public:
  float     m_fDistanceThreshold = 0.1f;
  xiiUInt32 m_uiSpawnCountMin    = 1;
  xiiUInt32 m_uiSpawnCountRange  = 0;
  xiiString m_sSpawnCountScaleParameter;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleEmitter_Distance final : public xiiParticleEmitter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitter_Distance, xiiParticleEmitter);

public:
  float               m_fDistanceThresholdSQR;
  xiiUInt32           m_uiSpawnCountMin;
  xiiUInt32           m_uiSpawnCountRange;
  xiiTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual bool IsContinuous() const override;
  virtual void OnFinalize() override;
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override;

  virtual xiiParticleEmitterState IsFinished() override;
  virtual xiiUInt32               ComputeSpawnCount(const xiiTime& tDiff) override;

  bool    m_bFirstUpdate = true;
  xiiVec3 m_vLastSpawnPosition;
};
