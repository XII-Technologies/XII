#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

bool xiiParticleSystemInstance::HasActiveParticles() const
{
  return m_StreamGroup.GetNumActiveElements() > 0;
}

bool xiiParticleSystemInstance::IsEmitterConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetEmitterFactories();

  if (factories.GetCount() != m_Emitters.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetEmitterType() != m_Emitters[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool xiiParticleSystemInstance::IsInitializerConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetInitializerFactories();

  if (factories.GetCount() != m_Initializers.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetInitializerType() != m_Initializers[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool xiiParticleSystemInstance::IsBehaviorConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetBehaviorFactories();

  if (factories.GetCount() != m_Behaviors.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetBehaviorType() != m_Behaviors[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool xiiParticleSystemInstance::IsTypeConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetTypeFactories();

  if (factories.GetCount() != m_Types.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetTypeType() != m_Types[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

bool xiiParticleSystemInstance::IsFinalizerConfigEqual(const xiiParticleSystemDescriptor* pTemplate) const
{
  const auto& factories = pTemplate->GetFinalizerFactories();

  if (factories.GetCount() != m_Finalizers.GetCount())
    return false;

  for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
  {
    if (factories[i]->GetFinalizerType() != m_Types[i]->GetDynamicRTTI())
      return false;
  }

  return true;
}

void xiiParticleSystemInstance::ConfigureFromTemplate(const xiiParticleSystemDescriptor* pTemplate)
{
  m_bVisible = pTemplate->m_bVisible;

  for (auto& info : m_StreamInfo)
  {
    info.m_bGetsInitialized = false;
    info.m_bInUse           = false;
  }

  const bool allProcessorsEqual = IsEmitterConfigEqual(pTemplate) && IsInitializerConfigEqual(pTemplate) && IsBehaviorConfigEqual(pTemplate) && IsTypeConfigEqual(pTemplate) && IsFinalizerConfigEqual(pTemplate);

  if (!allProcessorsEqual)
  {
    // recreate emitters, initializers and behaviors
    CreateStreamProcessors(pTemplate);
  }
  else
  {
    // just re-initialize the emitters with the new properties
    ReinitializeStreamProcessors(pTemplate);
  }

  SetupOptionalStreams();

  // setup stream initializers for all streams that have none yet
  CreateStreamZeroInitializers();
}


void xiiParticleSystemInstance::Finalize()
{
  for (auto& pEmitter : m_Emitters)
  {
    pEmitter->OnFinalize();
  }

  for (auto& pInitializer : m_Initializers)
  {
    pInitializer->OnFinalize();
  }

  for (auto& pBehavior : m_Behaviors)
  {
    pBehavior->OnFinalize();
  }

  for (auto& pFinalizer : m_Finalizers)
  {
    pFinalizer->OnFinalize();
  }

  for (auto& pType : m_Types)
  {
    pType->OnFinalize();
  }
}

void xiiParticleSystemInstance::CreateStreamProcessors(const xiiParticleSystemDescriptor* pTemplate)
{
  // all spawners get cleared, so clear this as well
  // this has to be done before any streams get created
  // for (auto& info : m_StreamInfo)
  //{
  //  info.m_pZeroInitializer = nullptr;
  //}


  const xiiUInt64 uiMaxParticles = m_StreamGroup.GetNumElements();
  m_StreamGroup.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
  m_StreamInfo.Clear();

  // emitters
  {
    m_Emitters.Clear();

    for (const auto pFactory : pTemplate->GetEmitterFactories())
    {
      xiiParticleEmitter* pEmitter = pFactory->CreateEmitter(this);
      m_StreamGroup.AddProcessor(pEmitter);
      m_Emitters.PushBack(pEmitter);
    }
  }

  // initializers
  {
    m_Initializers.Clear();

    for (const auto pFactory : pTemplate->GetInitializerFactories())
    {
      xiiParticleInitializer* pInitializer = pFactory->CreateInitializer(this);
      m_StreamGroup.AddProcessor(pInitializer);
      m_Initializers.PushBack(pInitializer);
    }
  }

  // behaviors
  {
    m_Behaviors.Clear();

    for (const auto pFactory : pTemplate->GetBehaviorFactories())
    {
      xiiParticleBehavior* pBehavior = pFactory->CreateBehavior(this);
      m_StreamGroup.AddProcessor(pBehavior);
      m_Behaviors.PushBack(pBehavior);
    }
  }

  // finalizers
  {
    m_Finalizers.Clear();

    for (const auto pFactory : pTemplate->GetFinalizerFactories())
    {
      xiiParticleFinalizer* pFinalizer = pFactory->CreateFinalizer(this);
      m_StreamGroup.AddProcessor(pFinalizer);
      m_Finalizers.PushBack(pFinalizer);
    }
  }

  // types
  {
    m_Types.Clear();

    for (const auto pFactory : pTemplate->GetTypeFactories())
    {
      xiiParticleType* pType = pFactory->CreateType(this);
      m_StreamGroup.AddProcessor(pType);
      m_Types.PushBack(pType);
    }
  }
}


void xiiParticleSystemInstance::SetupOptionalStreams()
{
  for (auto& pEmitter : m_Emitters)
  {
    pEmitter->QueryOptionalStreams();
  }

  for (auto& pInitializer : m_Initializers)
  {
    pInitializer->QueryOptionalStreams();
  }

  for (auto& pBehavior : m_Behaviors)
  {
    pBehavior->QueryOptionalStreams();
  }

  for (auto& pFinalizer : m_Finalizers)
  {
    pFinalizer->QueryOptionalStreams();
  }

  for (auto& pType : m_Types)
  {
    pType->QueryOptionalStreams();
  }
}

void xiiParticleSystemInstance::SetTransform(const xiiTransform& transform, const xiiVec3& vParticleStartVelocity)
{
  m_Transform              = transform;
  m_vParticleStartVelocity = vParticleStartVelocity;
}

void xiiParticleSystemInstance::ReinitializeStreamProcessors(const xiiParticleSystemDescriptor* pTemplate)
{
  // emitters
  {
    const auto& factories = pTemplate->GetEmitterFactories();

    for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Emitters[i]->Reset(this);
      factories[i]->CopyEmitterProperties(m_Emitters[i], false);
      m_Emitters[i]->CreateRequiredStreams();
    }
  }

  // initializers
  {
    const auto& factories = pTemplate->GetInitializerFactories();

    for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Initializers[i]->Reset(this);
      factories[i]->CopyInitializerProperties(m_Initializers[i], false);
      m_Initializers[i]->CreateRequiredStreams();
    }
  }

  // behaviors
  {
    const auto& factories = pTemplate->GetBehaviorFactories();

    for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Behaviors[i]->Reset(this);
      factories[i]->CopyBehaviorProperties(m_Behaviors[i], false);
      m_Behaviors[i]->CreateRequiredStreams();
    }
  }

  // finalizers
  {
    const auto& factories = pTemplate->GetFinalizerFactories();

    for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Finalizers[i]->Reset(this);
      factories[i]->CopyFinalizerProperties(m_Finalizers[i], false);
      m_Finalizers[i]->CreateRequiredStreams();
    }
  }

  // types
  {
    const auto& factories = pTemplate->GetTypeFactories();

    for (xiiUInt32 i = 0; i < factories.GetCount(); ++i)
    {
      m_Types[i]->Reset(this);
      factories[i]->CopyTypeProperties(m_Types[i], false);
      m_Types[i]->CreateRequiredStreams();
    }
  }
}

xiiParticleSystemInstance::xiiParticleSystemInstance()
{
  m_BoundingVolume.SetInvalid();
}

void xiiParticleSystemInstance::Construct(xiiUInt32 uiMaxParticles, xiiWorld* pWorld, xiiParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier)
{
  m_Transform.SetIdentity();
  m_pOwnerEffect          = pOwnerEffect;
  m_bEmitterEnabled       = true;
  m_bVisible              = true;
  m_pWorld                = pWorld;
  m_fSpawnCountMultiplier = fSpawnCountMultiplier;

  m_StreamInfo.Clear();
  m_StreamGroup.SetSize(uiMaxParticles);
}

void xiiParticleSystemInstance::Destruct()
{
  m_StreamGroup.Clear();
  m_Emitters.Clear();
  m_Initializers.Clear();
  m_Behaviors.Clear();
  m_Finalizers.Clear();
  m_Types.Clear();

  m_StreamInfo.Clear();
}

xiiParticleSystemState::Enum xiiParticleSystemInstance::Update(const xiiTime& diff)
{
  XII_PROFILE_SCOPE("PFX: System Update");

  xiiUInt32 uiSpawnedParticles = 0;

  if (m_bEmitterEnabled)
  {
    // if all emitters are finished, we deactivate this on the whole system
    bool bAllEmittersInactive = true;

    for (auto pEmitter : m_Emitters)
    {
      if (pEmitter->IsFinished() == xiiParticleEmitterState::Active)
      {
        bAllEmittersInactive    = false;
        const xiiUInt32 uiSpawn = pEmitter->ComputeSpawnCount(diff);

        if (uiSpawn > 0)
        {
          XII_PROFILE_SCOPE("PFX: System Emit");
          m_StreamGroup.InitializeElements(uiSpawn);
          uiSpawnedParticles += uiSpawn;
        }
      }
    }

    if (bAllEmittersInactive)
    {
      // there is a race condition writing this variable when an effect is used by a xiiParticleTypeEffect and should be disabled
      // therefore we must never set this variable to 'true' here, but we can set it to 'false' once we are done
      m_bEmitterEnabled = false;
    }
  }

  bool bHasReactingEmitters = false;

  // always check reactive emitters, as long as there are particles alive, they might produce more
  {
    for (auto pEmitter : m_Emitters)
    {
      if (pEmitter->IsFinished() == xiiParticleEmitterState::OnlyReacting)
      {
        bHasReactingEmitters = true;

        const xiiUInt32 uiSpawn = pEmitter->ComputeSpawnCount(diff);

        if (uiSpawn > 0)
        {
          XII_PROFILE_SCOPE("PFX: System Emit (React)");
          m_StreamGroup.InitializeElements(uiSpawn);
          uiSpawnedParticles += uiSpawn;
        }
      }
    }
  }

  {
    XII_PROFILE_SCOPE("PFX: System Step Behaviors");
    for (auto pBehavior : m_Behaviors)
    {
      pBehavior->StepParticleSystem(diff, uiSpawnedParticles);
    }
  }

  {
    XII_PROFILE_SCOPE("PFX: System Step Finalizers");
    for (auto pFinalizer : m_Finalizers)
    {
      pFinalizer->StepParticleSystem(diff, uiSpawnedParticles);
    }
  }

  {
    XII_PROFILE_SCOPE("PFX: System Step Types");
    for (auto pType : m_Types)
    {
      pType->StepParticleSystem(diff, uiSpawnedParticles);
    }
  }

  {
    XII_PROFILE_SCOPE("PFX: System Process");
    m_StreamGroup.Process();
  }

  if (m_bEmitterEnabled)
    return xiiParticleSystemState::Active;

  // all emitters are done, but some particles are still alive
  if (HasActiveParticles())
    return xiiParticleSystemState::EmittersFinished;

  return bHasReactingEmitters ? xiiParticleSystemState::OnlyReacting : xiiParticleSystemState::Inactive;
}

xiiProcessingStream* xiiParticleSystemInstance::QueryStream(const char* szName, xiiProcessingStream::DataType type) const
{
  xiiStringBuilder fullName;
  xiiParticleStreamFactory::GetFullStreamName(szName, type, fullName);

  return m_StreamGroup.GetStreamByName(fullName);
}

void xiiParticleSystemInstance::CreateStream(const char* szName, xiiProcessingStream::DataType type, xiiProcessingStream** pStream, xiiParticleStreamBinding& inout_binding, bool bWillInitializeElements)
{
  XII_ASSERT_DEV(pStream != nullptr, "The pointer to the stream pointer must not be null");

  xiiStringBuilder fullName;
  xiiParticleStreamFactory::GetFullStreamName(szName, type, fullName);

  StreamInfo* pInfo = nullptr;

  xiiProcessingStream* pSubStream = m_StreamGroup.GetStreamByName(fullName);
  if (pSubStream == nullptr)
  {
    pSubStream = m_StreamGroup.AddStream(fullName, type);

    pInfo          = &m_StreamInfo.ExpandAndGetRef();
    pInfo->m_sName = fullName;
  }
  else
  {
    for (auto& info : m_StreamInfo)
    {
      if (info.m_sName == fullName)
      {
        pInfo = &info;
        break;
      }
    }

    XII_ASSERT_DEV(pInfo != nullptr, "Could not find info for stream");
  }

  pInfo->m_bInUse = true;
  if (bWillInitializeElements)
    pInfo->m_bGetsInitialized = true;

  XII_ASSERT_DEV(pSubStream != nullptr, "Stream creation failed ('{0}' -> '{1}')", szName, fullName);
  *pStream = pSubStream;

  {
    auto& bind      = inout_binding.m_Bindings.ExpandAndGetRef();
    bind.m_ppStream = pStream;
    bind.m_sName    = fullName;
  }
}

void xiiParticleSystemInstance::CreateStreamZeroInitializers()
{
  for (xiiUInt32 i = 0; i < m_StreamInfo.GetCount();)
  {
    auto& info = m_StreamInfo[i];

    if ((!info.m_bInUse || info.m_bGetsInitialized) && info.m_pDefaultInitializer)
    {
      m_StreamGroup.RemoveProcessor(info.m_pDefaultInitializer);
      info.m_pDefaultInitializer = nullptr;
    }

    if (!info.m_bInUse)
    {
      m_StreamGroup.RemoveStreamByName(info.m_sName.GetData());
      m_StreamInfo.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (auto& info : m_StreamInfo)
  {
    if (info.m_bGetsInitialized)
      continue;

    XII_ASSERT_DEV(info.m_bInUse, "Invalid state");

    if (info.m_pDefaultInitializer == nullptr)
    {
      xiiParticleStream* pStream = GetOwnerWorldModule()->CreateStreamDefaultInitializer(this, info.m_sName);

      if (pStream == nullptr)
      {
        xiiLog::Warning("Particle stream '{0}' is zero-initialized.", info.m_sName);

        xiiProcessingStreamSpawnerZeroInitialized* pZeroInit = XII_DEFAULT_NEW(xiiProcessingStreamSpawnerZeroInitialized);
        pZeroInit->SetStreamName(info.m_sName);

        info.m_pDefaultInitializer = pZeroInit;
      }
      else
      {
        // xiiLog::Debug("Particle stream '{0}' is default-initialized.", info.m_sName);
        info.m_pDefaultInitializer = pStream;
      }

      m_StreamGroup.AddProcessor(info.m_pDefaultInitializer);
    }
  }
}

void xiiParticleStreamBinding::UpdateBindings(const xiiProcessingStreamGroup* pGroup) const
{
  for (const auto& bind : m_Bindings)
  {
    xiiProcessingStream* pStream = pGroup->GetStreamByName(bind.m_sName);
    XII_ASSERT_DEV(pStream != nullptr, "Stream binding '{0}' is invalid now", bind.m_sName);

    *bind.m_ppStream = pStream;
  }
}

void xiiParticleSystemInstance::ProcessEventQueue(xiiParticleEventQueue queue)
{
  for (auto pEmitter : m_Emitters)
  {
    pEmitter->ProcessEventQueue(queue);
  }
}

xiiParticleWorldModule* xiiParticleSystemInstance::GetOwnerWorldModule() const
{
  return m_pOwnerEffect->GetOwnerWorldModule();
}

void xiiParticleSystemInstance::ExtractSystemRenderData(xiiMsgExtractRenderData& ref_msg, const xiiTransform& instanceTransform) const
{
  for (auto pType : m_Types)
  {
    pType->ExtractTypeRenderData(ref_msg, instanceTransform);
  }
}

void xiiParticleSystemInstance::AddParticleDeathEventHandler(ParticleDeathHandler handler)
{
  m_StreamGroup.m_ElementRemovedEvent.AddEventHandler(handler);
}

void xiiParticleSystemInstance::RemoveParticleDeathEventHandler(ParticleDeathHandler handler)
{
  m_StreamGroup.m_ElementRemovedEvent.RemoveEventHandler(handler);
}

void xiiParticleSystemInstance::SetBoundingVolume(const xiiBoundingBoxSphere& volume, float fMaxParticleSize)
{
  m_BoundingVolume = volume;

  float fExpand = 0;
  for (const auto pType : m_Types)
  {
    fExpand = xiiMath::Max(fExpand, pType->GetMaxParticleRadius(fMaxParticleSize));
  }

  m_BoundingVolume.m_vBoxHalfExtends += xiiVec3(fExpand);
  m_BoundingVolume.m_fSphereRadius += fExpand;
}

bool xiiParticleSystemInstance::IsContinuous() const
{
  for (const xiiParticleEmitter* pEmitter : m_Emitters)
  {
    if (pEmitter->IsContinuous())
      return true;
  }

  return false;
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemInstance);
