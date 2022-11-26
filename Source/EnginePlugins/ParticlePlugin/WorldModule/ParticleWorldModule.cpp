#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiParticleWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleWorldModule::xiiParticleWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

xiiParticleWorldModule::~xiiParticleWorldModule()
{
  ClearParticleStreamFactories();
}

void xiiParticleWorldModule::Initialize()
{
  ConfigureParticleStreamFactories();

  {
    auto updateDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiParticleWorldModule::UpdateEffects, this);
    updateDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;
    updateDesc.m_fPriority                 = 1000.0f; // kick off particle tasks as early as possible

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto finishDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiParticleWorldModule::EnsureUpdatesFinished, this);
    finishDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    finishDesc.m_bOnlyUpdateWhenSimulating = true;
    finishDesc.m_fPriority                 = -1000.0f; // sync with particle tasks as late as possible

    RegisterUpdateFunction(finishDesc);
  }

  xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&xiiParticleWorldModule::ResourceEventHandler, this));

  {
    xiiHybridArray<const xiiRTTI*, 32> types;
    xiiRTTI::GetAllTypesDerivedFrom(xiiGetStaticRTTI<xiiParticleModule>(), types, false);

    for (const xiiRTTI* pRtti : types)
    {
      if (pRtti->GetAllocator()->CanAllocate())
      {
        xiiUniquePtr<xiiParticleModule> pModule = pRtti->GetAllocator()->Allocate<xiiParticleModule>();
        pModule->RequestRequiredWorldModulesForCache(this);
      }
    }
  }
}


void xiiParticleWorldModule::Deinitialize()
{
  XII_LOCK(m_Mutex);

  xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&xiiParticleWorldModule::ResourceEventHandler, this));

  WorldClear();
}

void xiiParticleWorldModule::EnsureUpdatesFinished(const xiiWorldModule::UpdateContext& context)
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  xiiTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  {
    XII_LOCK(m_Mutex);

    // The simulation tasks are done and the game objects have their global transform updated at this point, so we can push the transform
    // to the particle effects for the next simulation step and also ensure that the bounding volumes are correct for culling and rendering.
    if (xiiParticleComponentManager* pManager = GetWorld()->GetComponentManager<xiiParticleComponentManager>())
    {
      pManager->UpdatePfxTransformsAndBounds();
    }

    if (xiiParticleFinisherComponentManager* pManager = GetWorld()->GetComponentManager<xiiParticleFinisherComponentManager>())
    {
      pManager->UpdateBounds();
    }

    for (xiiUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
    {
      CreateFinisherComponent(m_NeedFinisherComponent[i]);
    }

    m_NeedFinisherComponent.Clear();
  }
}

void xiiParticleWorldModule::ExtractEffectRenderData(const xiiParticleEffectInstance* pEffect, xiiMsgExtractRenderData& msg, const xiiTransform& systemTransform) const
{
  XII_ASSERT_DEBUG(xiiTaskSystem::IsTaskGroupFinished(m_EffectUpdateTaskGroup), "Particle Effect Update Task is not finished!");

  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
  {
    const xiiParticleSystemInstance* pSystem = pEffect->GetParticleSystems()[i];

    if (pSystem == nullptr)
      continue;

    if (!pSystem->HasActiveParticles() || !pSystem->IsVisible())
      continue;

    pSystem->ExtractSystemRenderData(msg, systemTransform);
  }
}

void xiiParticleWorldModule::ResourceEventHandler(const xiiResourceEvent& e)
{
  if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiParticleEffectResource>())
  {
    XII_LOCK(m_Mutex);

    xiiParticleEffectResourceHandle hResource((xiiParticleEffectResource*)(e.m_pResource));

    const xiiUInt32 numEffects = m_ParticleEffects.GetCount();
    for (xiiUInt32 i = 0; i < numEffects; ++i)
    {
      if (m_ParticleEffects[i].GetResource() == hResource)
      {
        m_EffectsToReconfigure.PushBack(&m_ParticleEffects[i]);
      }
    }
  }
}

void xiiParticleWorldModule::ConfigureParticleStreamFactories()
{
  ClearParticleStreamFactories();

  xiiStringBuilder fullName;

  for (xiiRTTI* pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<xiiParticleStreamFactory>() || !pRtti->GetAllocator()->CanAllocate())
      continue;

    xiiParticleStreamFactory* pFactory = pRtti->GetAllocator()->Allocate<xiiParticleStreamFactory>();

    xiiParticleStreamFactory::GetFullStreamName(pFactory->GetStreamName(), pFactory->GetStreamDataType(), fullName);

    m_StreamFactories[fullName] = pFactory;
  }
}

void xiiParticleWorldModule::ClearParticleStreamFactories()
{
  for (auto it : m_StreamFactories)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_StreamFactories.Clear();
}

xiiParticleStream* xiiParticleWorldModule::CreateStreamDefaultInitializer(xiiParticleSystemInstance* pOwner, const char* szFullStreamName) const
{
  auto it = m_StreamFactories.Find(szFullStreamName);
  if (!it.IsValid())
    return nullptr;

  return it.Value()->CreateParticleStream(pOwner);
}

xiiWorldModule* xiiParticleWorldModule::GetCachedWorldModule(const xiiRTTI* pRtti) const
{
  xiiWorldModule* pModule = nullptr;
  m_WorldModuleCache.TryGetValue(pRtti, pModule);
  return pModule;
}

void xiiParticleWorldModule::CacheWorldModule(const xiiRTTI* pRtti)
{
  m_WorldModuleCache[pRtti] = GetWorld()->GetOrCreateModule(pRtti);
}

void xiiParticleWorldModule::CreateFinisherComponent(xiiParticleEffectInstance* pEffect)
{
  if (pEffect && !pEffect->IsSharedEffect())
  {
    pEffect->SetVisibleIf(nullptr);

    xiiWorld* pWorld = GetWorld();

    const xiiTransform transform = pEffect->GetTransform();

    xiiGameObjectDesc go;
    go.m_LocalPosition = transform.m_vPosition;
    go.m_LocalRotation = transform.m_qRotation;
    go.m_LocalScaling  = transform.m_vScale;
    // go.m_Tags = GetOwner()->GetTags(); // TODO: pass along tags -> needed for rendering filters

    xiiGameObject* pFinisher;
    pWorld->CreateObject(go, pFinisher);

    xiiParticleFinisherComponent* pFinisherComp;
    xiiParticleFinisherComponent::CreateComponent(pFinisher, pFinisherComp);

    pFinisherComp->m_EffectController = xiiParticleEffectController(this, pEffect->GetHandle());
    pFinisherComp->m_EffectController.SetTransform(transform, xiiVec3::ZeroVector()); // clear the velocity
  }
}

void xiiParticleWorldModule::WorldClear()
{
  // make sure no particle update task is still in the pipeline
  xiiTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  XII_LOCK(m_Mutex);

  m_FinishingEffects.Clear();
  m_NeedFinisherComponent.Clear();

  m_ActiveEffects.Clear();
  m_ParticleEffects.Clear();
  m_ParticleSystems.Clear();
  m_ParticleEffectsFreeList.Clear();
  m_ParticleSystemFreeList.Clear();
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleWorldModule);
