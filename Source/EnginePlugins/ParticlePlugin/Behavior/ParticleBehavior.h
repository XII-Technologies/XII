#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiProcessingStream;
class xiiParticleSystemInstance;
class xiiParticleBehavior;

/// \brief Base class for all particle behaviors
class XII_PARTICLEPLUGIN_DLL xiiParticleBehaviorFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehaviorFactory, xiiReflectedClass);

public:
  virtual const xiiRTTI* GetBehaviorType() const                                                     = 0;
  virtual void           CopyBehaviorProperties(xiiParticleBehavior* pObject, bool bFirstTime) const = 0;

  xiiParticleBehavior* CreateBehavior(xiiParticleSystemInstance* pOwner) const;

  virtual void Save(xiiStreamWriter& stream) const = 0;
  virtual void Load(xiiStreamReader& stream)       = 0;

  virtual void QueryFinalizerDependencies(xiiSet<const xiiRTTI*>& inout_FinalizerDeps) const {}
};

class XII_PARTICLEPLUGIN_DLL xiiParticleBehavior : public xiiParticleModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleBehavior, xiiParticleModule);

  friend class xiiParticleSystemInstance;

protected:
  xiiParticleBehavior();
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const xiiTime& tDiff, xiiUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  xiiTime m_TimeDiff;
};
