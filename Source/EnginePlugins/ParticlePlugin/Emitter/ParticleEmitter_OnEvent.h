#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

class XII_PARTICLEPLUGIN_DLL xiiParticleEmitterFactory_OnEvent final : public xiiParticleEmitterFactory
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitterFactory_OnEvent, xiiParticleEmitterFactory);

public:
  xiiParticleEmitterFactory_OnEvent();
  ~xiiParticleEmitterFactory_OnEvent();

  virtual const xiiRTTI* GetEmitterType() const override;
  virtual void           CopyEmitterProperties(xiiParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void           QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(xiiStreamWriter& inout_stream) const override;
  virtual void Load(xiiStreamReader& inout_stream) override;

  xiiString m_sEventName;
  xiiUInt32 m_uiSpawnCountMin   = 1;
  xiiUInt32 m_uiSpawnCountRange = 0;
  xiiString m_sSpawnCountScaleParameter;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleEmitter_OnEvent final : public xiiParticleEmitter
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitter_OnEvent, xiiParticleEmitter);

public:
  xiiTempHashedString m_sEventName;
  xiiUInt32           m_uiSpawnCountMin   = 1;
  xiiUInt32           m_uiSpawnCountRange = 0;
  xiiTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}

  virtual xiiParticleEmitterState IsFinished() override;
  virtual xiiUInt32               ComputeSpawnCount(const xiiTime& tDiff) override;

  virtual void ProcessEventQueue(xiiParticleEventQueue queue) override;

  bool m_bSpawn = false;
};
