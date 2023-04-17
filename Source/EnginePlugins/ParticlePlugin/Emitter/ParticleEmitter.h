#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiParticleSystemInstance;
class xiiProcessingStream;
class xiiParticleEmitter;

/// \brief Base class for all particle emitters
class XII_PARTICLEPLUGIN_DLL xiiParticleEmitterFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitterFactory, xiiReflectedClass);

public:
  virtual const xiiRTTI* GetEmitterType() const                                                     = 0;
  virtual void           CopyEmitterProperties(xiiParticleEmitter* pEmitter, bool bFirstTime) const = 0;

  xiiParticleEmitter* CreateEmitter(xiiParticleSystemInstance* pOwner) const;
  virtual void        QueryMaxParticleCount(xiiUInt32& out_uiMaxParticlesAbs, xiiUInt32& out_uiMaxParticlesPerSecond) const = 0;

  virtual void Save(xiiStreamWriter& inout_stream) const = 0;
  virtual void Load(xiiStreamReader& inout_stream)       = 0;
};

enum class xiiParticleEmitterState
{
  Active,
  Finished,
  OnlyReacting, //< Doesn't do anything, unless there are events that trigger it. That means it is considered finished, when all other emitters are
                //finished.
};

/// \brief Base class for stream spawners that are used by xiiParticleEmitter's
class XII_PARTICLEPLUGIN_DLL xiiParticleEmitter : public xiiParticleModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleEmitter, xiiParticleModule);

  friend class xiiParticleSystemInstance;
  friend class xiiParticleEmitterFactory;

protected:
  virtual bool IsContinuous() const;
  virtual void Process(xiiUInt64 uiNumElements) final override;

  /// \brief Called once per update. Must return how many new particles are to be spawned.
  virtual xiiUInt32 ComputeSpawnCount(const xiiTime& tDiff) = 0;

  /// \brief Called before ComputeSpawnCount(). Should return true, if the emitter will never spawn any more particles.
  virtual xiiParticleEmitterState IsFinished() = 0;

  virtual void ProcessEventQueue(xiiParticleEventQueue queue);
};
