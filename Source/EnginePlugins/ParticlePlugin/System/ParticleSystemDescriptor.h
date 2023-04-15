#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class xiiParticleEmitterFactory;
class xiiParticleBehaviorFactory;
class xiiParticleInitializerFactory;
class xiiParticleTypeFactory;

class XII_PARTICLEPLUGIN_DLL xiiParticleSystemDescriptor final : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleSystemDescriptor, xiiReflectedClass);

public:
  xiiParticleSystemDescriptor();
  ~xiiParticleSystemDescriptor();

  //////////////////////////////////////////////////////////////////////////
  /// Properties

  const xiiHybridArray<xiiParticleEmitterFactory*, 1>& GetEmitterFactories() const { return m_EmitterFactories; }

  void                                                     AddInitializerFactory(xiiParticleInitializerFactory* pFactory) { m_InitializerFactories.PushBack(pFactory); }
  void                                                     RemoveInitializerFactory(xiiParticleInitializerFactory* pFactory) { m_InitializerFactories.RemoveAndCopy(pFactory); }
  const xiiHybridArray<xiiParticleInitializerFactory*, 4>& GetInitializerFactories() const { return m_InitializerFactories; }

  void                                                  AddBehaviorFactory(xiiParticleBehaviorFactory* pFactory) { m_BehaviorFactories.PushBack(pFactory); }
  void                                                  RemoveBehaviorFactory(xiiParticleBehaviorFactory* pFactory) { m_BehaviorFactories.RemoveAndCopy(pFactory); }
  const xiiHybridArray<xiiParticleBehaviorFactory*, 4>& GetBehaviorFactories() const { return m_BehaviorFactories; }

  void                                              AddTypeFactory(xiiParticleTypeFactory* pFactory) { m_TypeFactories.PushBack(pFactory); }
  void                                              RemoveTypeFactory(xiiParticleTypeFactory* pFactory) { m_TypeFactories.RemoveAndCopy(pFactory); }
  const xiiHybridArray<xiiParticleTypeFactory*, 2>& GetTypeFactories() const { return m_TypeFactories; }

  const xiiHybridArray<xiiParticleFinalizerFactory*, 2>& GetFinalizerFactories() const { return m_FinalizerFactories; }

  xiiTime GetAvgLifetime() const;

  bool m_bVisible;

  xiiVarianceTypeTime m_LifeTime;
  xiiString           m_sOnDeathEvent;
  xiiString           m_sLifeScaleParameter;

  //////////////////////////////////////////////////////////////////////////

  void Save(xiiStreamWriter& inout_stream) const;
  void Load(xiiStreamReader& inout_stream);

private:
  void ClearEmitters();
  void ClearInitializers();
  void ClearBehaviors();
  void ClearTypes();
  void ClearFinalizers();
  void SetupDefaultProcessors();

  xiiString                                         m_sName;
  xiiHybridArray<xiiParticleEmitterFactory*, 1>     m_EmitterFactories;
  xiiHybridArray<xiiParticleInitializerFactory*, 4> m_InitializerFactories;
  xiiHybridArray<xiiParticleBehaviorFactory*, 4>    m_BehaviorFactories;
  xiiHybridArray<xiiParticleFinalizerFactory*, 2>   m_FinalizerFactories;
  xiiHybridArray<xiiParticleTypeFactory*, 2>        m_TypeFactories;
};
