#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/IdTable.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

using xiiParticleEffectResourceHandle = xiiTypedResourceHandle<class xiiParticleEffectResource>;
class xiiParticleEffectInstance;
struct xiiResourceEvent;
class xiiTaskGroupID;
class xiiParticleStream;
class xiiParticleStreamFactory;

/// \brief This world module stores all particle effect data that is active in a given xiiWorld instance
///
/// It is used to update all effects in one world and also to render them.
/// When an effect is stopped, it only stops emitting new particles, but it lives on until all particles are dead.
/// Therefore particle effects need to be managed outside of components. When a component dies, it only tells the
/// world module to 'destroy' it's effect, the rest is handled behind the scenes.
class XII_PARTICLEPLUGIN_DLL xiiParticleWorldModule final : public xiiWorldModule
{
  XII_DECLARE_WORLD_MODULE();
  XII_ADD_DYNAMIC_REFLECTION(xiiParticleWorldModule, xiiWorldModule);

public:
  xiiParticleWorldModule(xiiWorld* pWorld);
  ~xiiParticleWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  xiiParticleEffectHandle CreateEffectInstance(const xiiParticleEffectResourceHandle& hResource, xiiUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void*& inout_pSharedInstanceOwner, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams);

  /// \brief This does not actually the effect, it first stops it from emitting and destroys it once all particles have actually died of old age.
  void DestroyEffectInstance(const xiiParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner);

  bool TryGetEffectInstance(const xiiParticleEffectHandle& hEffect, xiiParticleEffectInstance*& out_pEffect);
  bool TryGetEffectInstance(const xiiParticleEffectHandle& hEffect, const xiiParticleEffectInstance*& out_pEffect) const;

  /// \brief Extracts render data for the given effect.
  void ExtractEffectRenderData(const xiiParticleEffectInstance* pEffect, xiiMsgExtractRenderData& msg, const xiiTransform& systemTransform) const;

  xiiParticleSystemInstance* CreateSystemInstance(xiiUInt32 uiMaxParticles, xiiWorld* pWorld, xiiParticleEffectInstance* pOwnerEffect, float fSpawnMultiplier);
  void                       DestroySystemInstance(xiiParticleSystemInstance* pInstance);

  xiiParticleStream* CreateStreamDefaultInitializer(xiiParticleSystemInstance* pOwner, const char* szFullStreamName) const;

  /// \brief Can be called at any time (e.g. during xiiParticleBehaviorFactory::CopyBehaviorProperties()) to query a previously cached world module,
  /// even if that happens on a thread which would not be allowed to query this from the xiiWorld at that time.
  xiiWorldModule* GetCachedWorldModule(const xiiRTTI* pRtti) const;

  /// \brief Should be called by xiiParticleModule::RequestRequiredWorldModulesForCache() to cache a pointer to a world module that is needed later.
  template <class T>
  void CacheWorldModule()
  {
    CacheWorldModule(xiiGetStaticRTTI<T>());
  }

  /// \brief Should be called by xiiParticleModule::RequestRequiredWorldModulesForCache() to cache a pointer to a world module that is needed later.
  void CacheWorldModule(const xiiRTTI* pRtti);

private:
  virtual void WorldClear() override;

  void UpdateEffects(const xiiWorldModule::UpdateContext& context);
  void EnsureUpdatesFinished(const xiiWorldModule::UpdateContext& context);

  void                    DestroyFinishedEffects();
  void                    CreateFinisherComponent(xiiParticleEffectInstance* pEffect);
  void                    ResourceEventHandler(const xiiResourceEvent& e);
  void                    ReconfigureEffects();
  xiiParticleEffectHandle InternalCreateSharedEffectInstance(const char* szSharedName, const xiiParticleEffectResourceHandle& hResource, xiiUInt64 uiRandomSeed, const void* pSharedInstanceOwner);
  xiiParticleEffectHandle InternalCreateEffectInstance(const xiiParticleEffectResourceHandle& hResource, xiiUInt64 uiRandomSeed, bool bIsShared, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams);

  void ConfigureParticleStreamFactories();
  void ClearParticleStreamFactories();

  mutable xiiMutex                                            m_Mutex;
  xiiDeque<xiiParticleEffectInstance>                         m_ParticleEffects;
  xiiDynamicArray<xiiParticleEffectInstance*>                 m_FinishingEffects;
  xiiDynamicArray<xiiParticleEffectInstance*>                 m_NeedFinisherComponent;
  xiiDynamicArray<xiiParticleEffectInstance*>                 m_EffectsToReconfigure;
  xiiDynamicArray<xiiParticleEffectInstance*>                 m_ParticleEffectsFreeList;
  xiiMap<xiiString, xiiParticleEffectHandle>                  m_SharedEffects;
  xiiIdTable<xiiParticleEffectId, xiiParticleEffectInstance*> m_ActiveEffects;
  xiiDeque<xiiParticleSystemInstance>                         m_ParticleSystems;
  xiiDynamicArray<xiiParticleSystemInstance*>                 m_ParticleSystemFreeList;
  xiiTaskGroupID                                              m_EffectUpdateTaskGroup;
  xiiMap<xiiString, xiiParticleStreamFactory*>                m_StreamFactories;
  xiiHashTable<const xiiRTTI*, xiiWorldModule*>               m_WorldModuleCache;
};
