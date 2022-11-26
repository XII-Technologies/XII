#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiParticleEffectInstance::xiiParticleEffectInstance()
{
  m_pTask = XII_DEFAULT_NEW(xiiParticleEffectUpdateTask, this);
  m_pTask->ConfigureTask("Particle Effect Update", xiiTaskNesting::Maybe);

  m_pOwnerModule = nullptr;

  Destruct();
}

xiiParticleEffectInstance::~xiiParticleEffectInstance()
{
  Destruct();
}

void xiiParticleEffectInstance::Construct(xiiParticleEffectHandle hEffectHandle, const xiiParticleEffectResourceHandle& hResource, xiiWorld* pWorld, xiiParticleWorldModule* pOwnerModule, xiiUInt64 uiRandomSeed, bool bIsShared, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams)
{
  m_hEffectHandle   = hEffectHandle;
  m_pWorld          = pWorld;
  m_pOwnerModule    = pOwnerModule;
  m_hResource       = hResource;
  m_bIsSharedEffect = bIsShared;
  m_bEmitterEnabled = true;
  m_bIsFinishing    = false;
  m_BoundingVolume.SetInvalid();
  m_ElapsedTimeSinceUpdate.SetZero();
  m_EffectIsVisible.SetZero();
  m_iMinSimStepsToDo = 4;
  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_TotalEffectLifeTime.SetZero();
  m_pVisibleIf   = nullptr;
  m_uiRandomSeed = uiRandomSeed;

  if (uiRandomSeed == 0)
    m_Random.InitializeFromCurrentTime();
  else
    m_Random.Initialize(uiRandomSeed);

  Reconfigure(true, floatParams, colorParams);
}

void xiiParticleEffectInstance::Destruct()
{
  Interrupt();

  m_SharedInstances.Clear();
  m_hEffectHandle.Invalidate();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_bIsSharedEffect = false;
  m_pWorld          = nullptr;
  m_hResource.Invalidate();
  m_hEffectHandle.Invalidate();
  m_uiReviveTimeout = 5;
}

void xiiParticleEffectInstance::Interrupt()
{
  ClearParticleSystems();
  ClearEventReactions();
  m_bEmitterEnabled = false;
}

void xiiParticleEffectInstance::SetEmitterEnabled(bool enable)
{
  m_bEmitterEnabled = enable;

  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    }
  }
}


bool xiiParticleEffectInstance::HasActiveParticles() const
{
  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->HasActiveParticles())
        return true;
    }
  }

  return false;
}


void xiiParticleEffectInstance::ClearParticleSystem(xiiUInt32 index)
{
  if (m_ParticleSystems[index])
  {
    m_pOwnerModule->DestroySystemInstance(m_ParticleSystems[index]);
    m_ParticleSystems[index] = nullptr;
  }
}

void xiiParticleEffectInstance::ClearParticleSystems()
{
  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    ClearParticleSystem(i);
  }

  m_ParticleSystems.Clear();
}


void xiiParticleEffectInstance::ClearEventReactions()
{
  for (xiiUInt32 i = 0; i < m_EventReactions.GetCount(); ++i)
  {
    if (m_EventReactions[i])
    {
      m_EventReactions[i]->GetDynamicRTTI()->GetAllocator()->Deallocate(m_EventReactions[i]);
    }
  }

  m_EventReactions.Clear();
}

bool xiiParticleEffectInstance::IsContinuous() const
{
  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->IsContinuous())
        return true;
    }
  }

  return false;
}

void xiiParticleEffectInstance::PreSimulate()
{
  if (m_PreSimulateDuration.GetSeconds() == 0.0)
    return;

  PassTransformToSystems();

  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  // simulate in large steps to get close
  {
    const xiiTime tDiff = xiiTime::Seconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 10.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // finer steps
  {
    const xiiTime tDiff = xiiTime::Seconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // even finer
  {
    const xiiTime tDiff = xiiTime::Seconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // final step if necessary
  if (m_PreSimulateDuration.GetSeconds() > 0.0)
  {
    StepSimulation(m_PreSimulateDuration);
    m_PreSimulateDuration = xiiTime::Seconds(0);
  }

  if (!IsContinuous())
  {
    // Can't check this at the beginning, because the particle systems are only set up during StepSimulation.
    xiiLog::Warning("Particle pre-simulation is enabled on an effect that is not continuous.");
  }
}

void xiiParticleEffectInstance::SetIsVisible() const
{
  // if it is visible this frame, also render it the next few frames
  // this has multiple purposes:
  // 1) it fixes the transition when handing off an effect from a
  //    xiiParticleComponent to a xiiParticleFinisherComponent
  //    though this would only need one frame overlap
  // 2) The bounding volume for culling is only computed every couple of frames
  //    so it may be too small and culling could be imprecise
  //    by just rendering it the next 100ms, no matter what, the bounding volume
  //    does not need to be updated so frequently
  m_EffectIsVisible = xiiClock::GetGlobalClock()->GetAccumulatedTime() + xiiTime::Seconds(0.1);
}


void xiiParticleEffectInstance::SetVisibleIf(xiiParticleEffectInstance* pOtherVisible)
{
  XII_ASSERT_DEV(pOtherVisible != this, "Invalid effect");
  m_pVisibleIf = pOtherVisible;
}

bool xiiParticleEffectInstance::IsVisible() const
{
  if (m_pVisibleIf != nullptr)
  {
    return m_pVisibleIf->IsVisible();
  }

  return m_EffectIsVisible >= xiiClock::GetGlobalClock()->GetAccumulatedTime();
}

void xiiParticleEffectInstance::Reconfigure(bool bFirstTime, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams)
{
  if (!m_hResource.IsValid())
  {
    xiiLog::Error("Effect Reconfigure: Effect Resource is invalid");
    return;
  }

  xiiResourceLock<xiiParticleEffectResource> pResource(m_hResource, xiiResourceAcquireMode::BlockTillLoaded);

  const auto& desc    = pResource->GetDescriptor().m_Effect;
  const auto& systems = desc.GetParticleSystems();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_fApplyInstanceVelocity = desc.m_fApplyInstanceVelocity;
  m_bSimulateInLocalSpace  = desc.m_bSimulateInLocalSpace;
  m_InvisibleUpdateRate    = desc.m_InvisibleUpdateRate;

  // parameters
  {
    m_FloatParameters.Clear();
    m_ColorParameters.Clear();

    for (auto it = desc.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(xiiTempHashedString(it.Key().GetData()), it.Value());
    }

    for (auto it = desc.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(xiiTempHashedString(it.Key().GetData()), it.Value());
    }

    // shared effects do not support per-instance parameters
    if (m_bIsSharedEffect)
    {
      if (!floatParams.IsEmpty() || !colorParams.IsEmpty())
      {
        xiiLog::Warning("Shared particle effects do not support effect parameters");
      }
    }
    else
    {
      for (xiiUInt32 p = 0; p < floatParams.GetCount(); ++p)
      {
        SetParameter(floatParams[p].m_sName, floatParams[p].m_Value);
      }

      for (xiiUInt32 p = 0; p < colorParams.GetCount(); ++p)
      {
        SetParameter(colorParams[p].m_sName, colorParams[p].m_Value);
      }
    }
  }

  if (bFirstTime)
  {
    m_PreSimulateDuration = desc.m_PreSimulateDuration;
  }

  // TODO Check max number of particles etc. to reset

  if (m_ParticleSystems.GetCount() != systems.GetCount())
  {
    // reset everything
    ClearParticleSystems();
  }

  m_ParticleSystems.SetCount(systems.GetCount());

  struct MulCount
  {
    XII_DECLARE_POD_TYPE();

    float     m_fMultiplier = 1.0f;
    xiiUInt32 m_uiCount     = 0;
  };

  xiiHybridArray<MulCount, 8> systemMaxParticles;
  {
    systemMaxParticles.SetCountUninitialized(systems.GetCount());
    for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      xiiUInt32 uiMaxParticlesAbs = 0, uiMaxParticlesPerSec = 0;
      for (const xiiParticleEmitterFactory* pEmitter : systems[i]->GetEmitterFactories())
      {
        xiiUInt32 uiMaxParticlesAbs0 = 0, uiMaxParticlesPerSec0 = 0;
        pEmitter->QueryMaxParticleCount(uiMaxParticlesAbs0, uiMaxParticlesPerSec0);

        uiMaxParticlesAbs += uiMaxParticlesAbs0;
        uiMaxParticlesPerSec += uiMaxParticlesPerSec0;
      }

      const xiiTime tLifetime = systems[i]->GetAvgLifetime();

      const xiiUInt32 uiMaxParticles = xiiMath::Max(32u, xiiMath::Max(uiMaxParticlesAbs, (xiiUInt32)(uiMaxParticlesPerSec * tLifetime.GetSeconds())));

      float fMultiplier = 1.0f;

      for (const xiiParticleInitializerFactory* pInitializer : systems[i]->GetInitializerFactories())
      {
        fMultiplier *= pInitializer->GetSpawnCountMultiplier(this);
      }

      systemMaxParticles[i].m_fMultiplier = xiiMath::Max(0.0f, fMultiplier);
      systemMaxParticles[i].m_uiCount     = (xiiUInt32)(uiMaxParticles * systemMaxParticles[i].m_fMultiplier);
    }
  }
  // delete all that have important changes
  {
    for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        if (m_ParticleSystems[i]->GetMaxParticles() != systemMaxParticles[i].m_uiCount)
          ClearParticleSystem(i);
      }
    }
  }

  // recreate where necessary
  {
    for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] == nullptr)
      {
        m_ParticleSystems[i] = m_pOwnerModule->CreateSystemInstance(systemMaxParticles[i].m_uiCount, m_pWorld, this, systemMaxParticles[i].m_fMultiplier);
      }
    }
  }

  const xiiVec3 vStartVelocity = m_vVelocity * m_fApplyInstanceVelocity;

  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform, vStartVelocity);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    m_ParticleSystems[i]->Finalize();
  }

  // recreate event reactions
  {
    ClearEventReactions();

    m_EventReactions.SetCount(desc.GetEventReactions().GetCount());

    const auto& er = desc.GetEventReactions();
    for (xiiUInt32 i = 0; i < er.GetCount(); ++i)
    {
      if (m_EventReactions[i] == nullptr)
      {
        m_EventReactions[i] = er[i]->CreateEventReaction(this);
      }
    }
  }
}

bool xiiParticleEffectInstance::Update(const xiiTime& tDiff)
{
  XII_PROFILE_SCOPE("PFX: Effect Update");

  xiiTime tMinStep = xiiTime::Seconds(0);

  if (!IsVisible() && m_iMinSimStepsToDo == 0)
  {
    // shared effects always get paused when they are invisible
    if (IsSharedEffect())
      return true;

    switch (m_InvisibleUpdateRate)
    {
      case xiiEffectInvisibleUpdateRate::FullUpdate:
        tMinStep = xiiTime::Seconds(1.0 / 60.0);
        break;

      case xiiEffectInvisibleUpdateRate::Max20fps:
        tMinStep = xiiTime::Milliseconds(50);
        break;

      case xiiEffectInvisibleUpdateRate::Max10fps:
        tMinStep = xiiTime::Milliseconds(100);
        break;

      case xiiEffectInvisibleUpdateRate::Max5fps:
        tMinStep = xiiTime::Milliseconds(200);
        break;

      case xiiEffectInvisibleUpdateRate::Pause:
      {
        if (m_bEmitterEnabled)
        {
          // during regular operation, pause
          return m_uiReviveTimeout > 0;
        }

        // otherwise do infrequent updates to shut the effect down
        tMinStep = xiiTime::Milliseconds(200);
        break;
      }

      case xiiEffectInvisibleUpdateRate::Discard:
        Interrupt();
        return false;
    }
  }

  m_ElapsedTimeSinceUpdate += tDiff;
  PassTransformToSystems();

  // if the time step is too big, iterate multiple times
  {
    const xiiTime tMaxTimeStep = xiiTime::Milliseconds(200); // in sync with Max5fps
    while (m_ElapsedTimeSinceUpdate > tMaxTimeStep)
    {
      m_ElapsedTimeSinceUpdate -= tMaxTimeStep;

      if (!StepSimulation(tMaxTimeStep))
        return false;
    }
  }

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return m_uiReviveTimeout > 0;

  // do the remainder
  const xiiTime tUpdateDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate.SetZero();

  return StepSimulation(tUpdateDiff);
}

bool xiiParticleEffectInstance::StepSimulation(const xiiTime& tDiff)
{
  m_TotalEffectLifeTime += tDiff;

  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i] != nullptr)
    {
      auto state = m_ParticleSystems[i]->Update(tDiff);

      if (state == xiiParticleSystemState::Inactive)
      {
        ClearParticleSystem(i);
      }
      else if (state != xiiParticleSystemState::OnlyReacting)
      {
        // this is used to delay particle effect death by a couple of frames
        // that way, if an event is in the pipeline that might trigger a reacting emitter,
        // or particles are in the spawn queue, but not yet created, we don't kill the effect too early
        m_uiReviveTimeout = 3;
      }
    }
  }

  CombineSystemBoundingVolumes();

  m_iMinSimStepsToDo = xiiMath::Max<xiiInt8>(m_iMinSimStepsToDo - 1, 0);

  --m_uiReviveTimeout;
  return m_uiReviveTimeout > 0;
}


void xiiParticleEffectInstance::AddParticleEvent(const xiiParticleEvent& pe)
{
  // drop events when the capacity is full
  if (m_EventQueue.GetCount() == m_EventQueue.GetCapacity())
    return;

  m_EventQueue.PushBack(pe);
}

void xiiParticleEffectInstance::UpdateWindSamples()
{
  const xiiUInt32 uiDataIdx = xiiRenderWorld::GetDataIndexForExtraction();

  m_vSampleWindResults[uiDataIdx].Clear();

  if (m_vSampleWindLocations[uiDataIdx].IsEmpty())
    return;

  m_vSampleWindResults[uiDataIdx].SetCount(m_vSampleWindLocations[uiDataIdx].GetCount(), xiiVec3::ZeroVector());

  if (auto pWind = GetWorld()->GetModuleReadOnly<xiiWindWorldModuleInterface>())
  {
    for (xiiUInt32 i = 0; i < m_vSampleWindLocations[uiDataIdx].GetCount(); ++i)
    {
      m_vSampleWindResults[uiDataIdx][i] = pWind->GetWindAt(m_vSampleWindLocations[uiDataIdx][i]);
    }
  }

  m_vSampleWindLocations[uiDataIdx].Clear();
}

xiiUInt64 xiiParticleEffectInstance::GetNumActiveParticles() const
{
  xiiUInt64 num = 0;

  for (auto pSystem : m_ParticleSystems)
  {
    if (pSystem)
    {
      num += pSystem->GetNumActiveParticles();
    }
  }

  return num;
}

void xiiParticleEffectInstance::SetTransform(const xiiTransform& transform, const xiiVec3& vParticleStartVelocity)
{
  m_Transform             = transform;
  m_TransformForNextFrame = transform;

  m_vVelocity             = vParticleStartVelocity;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

void xiiParticleEffectInstance::SetTransformForNextFrame(const xiiTransform& transform, const xiiVec3& vParticleStartVelocity)
{
  m_TransformForNextFrame = transform;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

xiiInt32 xiiParticleEffectInstance::AddWindSampleLocation(const xiiVec3& pos)
{
  const xiiUInt32 uiDataIdx = xiiRenderWorld::GetDataIndexForRendering();

  if (m_vSampleWindLocations[uiDataIdx].GetCount() < m_vSampleWindLocations[uiDataIdx].GetCapacity())
  {
    m_vSampleWindLocations[uiDataIdx].PushBack(pos);
    return m_vSampleWindLocations[uiDataIdx].GetCount() - 1;
  }

  return -1;
}

xiiVec3 xiiParticleEffectInstance::GetWindSampleResult(xiiInt32 idx) const
{
  const xiiUInt32 uiDataIdx = xiiRenderWorld::GetDataIndexForRendering();

  if (idx >= 0 && m_vSampleWindResults[uiDataIdx].GetCount() > (xiiUInt32)idx)
  {
    return m_vSampleWindResults[uiDataIdx][idx];
  }

  return xiiVec3::ZeroVector();
}

void xiiParticleEffectInstance::PassTransformToSystems()
{
  if (!m_bSimulateInLocalSpace)
  {
    const xiiVec3 vStartVel = m_vVelocity * m_fApplyInstanceVelocity;

    for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        m_ParticleSystems[i]->SetTransform(m_Transform, vStartVel);
      }
    }
  }
}

void xiiParticleEffectInstance::AddSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Insert(pSharedInstanceOwner);
}

void xiiParticleEffectInstance::RemoveSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Remove(pSharedInstanceOwner);
}

bool xiiParticleEffectInstance::ShouldBeUpdated() const
{
  if (m_hEffectHandle.IsInvalidated())
    return false;

  // do not update shared instances when there is no one watching
  if (m_bIsSharedEffect && m_SharedInstances.GetCount() == 0)
    return false;

  return true;
}

void xiiParticleEffectInstance::GetBoundingVolume(xiiBoundingBoxSphere& volume) const
{
  if (!m_BoundingVolume.IsValid())
  {
    volume = xiiBoundingSphere(xiiVec3::ZeroVector(), 0.25f);
    return;
  }

  volume = m_BoundingVolume;

  if (!m_bSimulateInLocalSpace)
  {
    // transform the bounding volume to local space, unless it was already created there
    const xiiMat4 invTrans = GetTransform().GetAsMat4().GetInverse();
    volume.Transform(invTrans);
  }
}

void xiiParticleEffectInstance::CombineSystemBoundingVolumes()
{
  xiiBoundingBoxSphere effectVolume;
  effectVolume.SetInvalid();

  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      const xiiBoundingBoxSphere& systemVolume = m_ParticleSystems[i]->GetBoundingVolume();
      if (systemVolume.IsValid())
      {
        effectVolume.ExpandToInclude(systemVolume);
      }
    }
  }

  m_BoundingVolume = effectVolume;
}

void xiiParticleEffectInstance::ProcessEventQueues()
{
  m_Transform = m_TransformForNextFrame;
  m_vVelocity = m_vVelocityForNextFrame;

  if (m_EventQueue.IsEmpty())
    return;

  XII_PROFILE_SCOPE("PFX: Effect Event Queue");
  for (xiiUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->ProcessEventQueue(m_EventQueue);
    }
  }

  for (const xiiParticleEvent& e : m_EventQueue)
  {
    xiiUInt32 rnd = m_Random.UIntInRange(100);

    for (xiiParticleEventReaction* pReaction : m_EventReactions)
    {
      if (pReaction->m_sEventName != e.m_EventType)
        continue;

      if (pReaction->m_uiProbability > rnd)
      {
        pReaction->ProcessEvent(e);
        break;
      }

      rnd -= pReaction->m_uiProbability;
    }
  }

  m_EventQueue.Clear();
}

xiiParticleEffectUpdateTask::xiiParticleEffectUpdateTask(xiiParticleEffectInstance* pEffect)
{
  m_pEffect = pEffect;
  m_UpdateDiff.SetZero();
}

void xiiParticleEffectUpdateTask::Execute()
{
  if (HasBeenCanceled())
    return;

  if (m_UpdateDiff.GetSeconds() != 0.0)
  {
    m_pEffect->PreSimulate();

    if (!m_pEffect->Update(m_UpdateDiff))
    {
      const xiiParticleEffectHandle hEffect = m_pEffect->GetHandle();
      XII_ASSERT_DEBUG(!hEffect.IsInvalidated(), "Invalid particle effect handle");

      m_pEffect->GetOwnerWorldModule()->DestroyEffectInstance(hEffect, true, nullptr);
    }
  }
}

void xiiParticleEffectInstance::SetParameter(const xiiTempHashedString& name, float value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (xiiUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == name.GetHash())
    {
      m_FloatParameters[i].m_fValue = value;
      return;
    }
  }

  auto& ref        = m_FloatParameters.ExpandAndGetRef();
  ref.m_uiNameHash = name.GetHash();
  ref.m_fValue     = value;
}

void xiiParticleEffectInstance::SetParameter(const xiiTempHashedString& name, const xiiColor& value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (xiiUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == name.GetHash())
    {
      m_ColorParameters[i].m_Value = value;
      return;
    }
  }

  auto& ref        = m_ColorParameters.ExpandAndGetRef();
  ref.m_uiNameHash = name.GetHash();
  ref.m_Value      = value;
}

xiiInt32 xiiParticleEffectInstance::FindFloatParameter(const xiiTempHashedString& name) const
{
  for (xiiUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == name.GetHash())
      return i;
  }

  return -1;
}

float xiiParticleEffectInstance::GetFloatParameter(const xiiTempHashedString& name, float defaultValue) const
{
  if (name.IsEmpty())
    return defaultValue;

  for (xiiUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == name.GetHash())
      return m_FloatParameters[i].m_fValue;
  }

  return defaultValue;
}

xiiInt32 xiiParticleEffectInstance::FindColorParameter(const xiiTempHashedString& name) const
{
  for (xiiUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == name.GetHash())
      return i;
  }

  return -1;
}

const xiiColor& xiiParticleEffectInstance::GetColorParameter(const xiiTempHashedString& name, const xiiColor& defaultValue) const
{
  if (name.IsEmpty())
    return defaultValue;

  for (xiiUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == name.GetHash())
      return m_ColorParameters[i].m_Value;
  }

  return defaultValue;
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectInstance);
