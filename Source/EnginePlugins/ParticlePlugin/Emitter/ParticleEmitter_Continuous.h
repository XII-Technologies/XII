#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

using xiiCurve1DResourceHandle = xiiTypedResourceHandle<class xiiCurve1DResource>;

class XII_PARTICLEPLUGIN_DLL xiiParticleEmitterFactory_Continuous final : public xiiParticleEmitterFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitterFactory_Continuous, xiiParticleEmitterFactory);

public:
  xiiParticleEmitterFactory_Continuous();

  virtual const xiiRTTI* GetEmitterType() const override;
  virtual void           CopyEmitterProperties(xiiParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void           QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

public:
  xiiTime m_StartDelay;

  xiiUInt32 m_uiSpawnCountPerSec;
  xiiUInt32 m_uiSpawnCountPerSecRange;
  xiiString m_sSpawnCountScaleParameter;

  xiiCurve1DResourceHandle m_hCountCurve;
  xiiTime                  m_CurveDuration;

  void        SetCountCurveFile(const char* szFile);
  const char* GetCountCurveFile() const;
};


class XII_PARTICLEPLUGIN_DLL xiiParticleEmitter_Continuous final : public xiiParticleEmitter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitter_Continuous, xiiParticleEmitter);

public:
  xiiTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  xiiUInt32           m_uiSpawnCountPerSec;
  xiiUInt32           m_uiSpawnCountPerSecRange;
  xiiTempHashedString m_sSpawnCountScaleParameter;

  xiiCurve1DResourceHandle m_hCountCurve;
  xiiTime                  m_CurveDuration;


  virtual void CreateRequiredStreams() override {}

protected:
  virtual bool IsContinuous() const override { return true; }

  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}
  virtual void OnFinalize() override;

  virtual xiiParticleEmitterState IsFinished() override;
  virtual xiiUInt32               ComputeSpawnCount(const xiiTime& tDiff) override;

  xiiTime m_CountCurveTime;
  xiiTime m_TimeSinceRandom;
  float   m_fCurSpawnPerSec;
  float   m_fCurSpawnCounter;
};
