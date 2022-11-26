#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiParticleSystemInstance;
class xiiProcessingStream;
class xiiParticleInitializer;
class xiiParticleEffectInstance;

/// \brief Base class for all particle emitters
class XII_PARTICLEPLUGIN_DLL xiiParticleInitializerFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializerFactory, xiiReflectedClass);

public:
  virtual const xiiRTTI* GetInitializerType() const                                                             = 0;
  virtual void           CopyInitializerProperties(xiiParticleInitializer* pInitializer, bool bFirstTime) const = 0;
  virtual float          GetSpawnCountMultiplier(const xiiParticleEffectInstance* pEffect) const;

  xiiParticleInitializer* CreateInitializer(xiiParticleSystemInstance* pOwner) const;

  virtual void Save(xiiStreamWriter& stream) const = 0;
  virtual void Load(xiiStreamReader& stream)       = 0;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const {}
};

/// \brief Base class for stream spawners that are used by xiiParticleEmitter's
class XII_PARTICLEPLUGIN_DLL xiiParticleInitializer : public xiiParticleModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleInitializer, xiiParticleModule);

  friend class xiiParticleSystemInstance;
  friend class xiiParticleInitializerFactory;

protected:
  xiiParticleInitializer();

  virtual void Process(xiiUInt64 uiNumElements) final override {}
};
