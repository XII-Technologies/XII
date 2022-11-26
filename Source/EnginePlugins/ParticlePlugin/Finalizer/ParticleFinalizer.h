#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiProcessingStream;
class xiiParticleSystemInstance;
class xiiParticleFinalizer;

/// \brief Base class for all particle Finalizers
class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizerFactory : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizerFactory, xiiReflectedClass);

public:
  virtual const xiiRTTI* GetFinalizerType() const                                                      = 0;
  virtual void           CopyFinalizerProperties(xiiParticleFinalizer* pObject, bool bFirstTime) const = 0;

  xiiParticleFinalizer* CreateFinalizer(xiiParticleSystemInstance* pOwner) const;
};

class XII_PARTICLEPLUGIN_DLL xiiParticleFinalizer : public xiiParticleModule
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleFinalizer, xiiParticleModule);

  friend class xiiParticleSystemInstance;

protected:
  xiiParticleFinalizer();
  virtual void InitializeElements(xiiUInt64 uiStartIndex, xiiUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const xiiTime& tDiff, xiiUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  xiiTime m_TimeDiff;
};
