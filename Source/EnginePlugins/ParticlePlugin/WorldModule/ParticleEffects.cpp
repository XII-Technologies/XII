#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiParticleEffectHandle xiiParticleWorldModule::InternalCreateEffectInstance(const xiiParticleEffectResourceHandle& hResource, xiiUInt64 uiRandomSeed, bool bIsShared, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams)
{
  XII_LOCK(m_Mutex);

  xiiParticleEffectInstance* pInstance = nullptr;

  if (!m_ParticleEffectsFreeList.IsEmpty())
  {
    pInstance = m_ParticleEffectsFreeList.PeekBack();
    m_ParticleEffectsFreeList.PopBack();
  }
  else
  {
    pInstance = &m_ParticleEffects.ExpandAndGetRef();
  }

  xiiParticleEffectHandle hEffectHandle(m_ActiveEffects.Insert(pInstance));
  pInstance->Construct(hEffectHandle, hResource, GetWorld(), this, uiRandomSeed, bIsShared, floatParams, colorParams);

  return hEffectHandle;
}

xiiParticleEffectHandle xiiParticleWorldModule::InternalCreateSharedEffectInstance(
  const char*                            szSharedName,
  const xiiParticleEffectResourceHandle& hResource,
  xiiUInt64                              uiRandomSeed,
  const void*                            pSharedInstanceOwner)
{
  XII_LOCK(m_Mutex);

  xiiStringBuilder fullName;
  fullName.Format("{{0}}-{{1}}[{2}]", szSharedName, hResource.GetResourceID(), uiRandomSeed);

  bool                       bExisted = false;
  auto                       it       = m_SharedEffects.FindOrAdd(fullName, &bExisted);
  xiiParticleEffectInstance* pEffect  = nullptr;

  if (bExisted)
  {
    TryGetEffectInstance(it.Value(), pEffect);
  }

  if (!pEffect)
  {
    it.Value() =
      InternalCreateEffectInstance(hResource, uiRandomSeed, true, xiiArrayPtr<xiiParticleEffectFloatParam>(), xiiArrayPtr<xiiParticleEffectColorParam>());
    TryGetEffectInstance(it.Value(), pEffect);
  }

  XII_ASSERT_DEBUG(pEffect != nullptr, "Invalid effect pointer");
  pEffect->AddSharedInstance(pSharedInstanceOwner);

  return it.Value();
}


xiiParticleEffectHandle xiiParticleWorldModule::CreateEffectInstance(const xiiParticleEffectResourceHandle& hResource, xiiUInt64 uiRandomSeed, const char* szSharedName, const void*& inout_pSharedInstanceOwner, xiiArrayPtr<xiiParticleEffectFloatParam> floatParams, xiiArrayPtr<xiiParticleEffectColorParam> colorParams)
{
  XII_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  bool bIsShared = !xiiStringUtils::IsNullOrEmpty(szSharedName) && (inout_pSharedInstanceOwner != nullptr);

  if (!bIsShared)
  {
    xiiResourceLock<xiiParticleEffectResource> pResource(hResource, xiiResourceAcquireMode::BlockTillLoaded);
    bIsShared |= pResource->GetDescriptor().m_Effect.m_bAlwaysShared;
  }

  if (!bIsShared)
  {
    inout_pSharedInstanceOwner = nullptr;
    return InternalCreateEffectInstance(hResource, uiRandomSeed, false, floatParams, colorParams);
  }
  else
  {
    return InternalCreateSharedEffectInstance(szSharedName, hResource, uiRandomSeed, inout_pSharedInstanceOwner);
  }
}

void xiiParticleWorldModule::DestroyEffectInstance(const xiiParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner)
{
  XII_LOCK(m_Mutex);

  xiiParticleEffectInstance* pInstance = nullptr;
  if (TryGetEffectInstance(hEffect, pInstance))
  {
    if (pSharedInstanceOwner != nullptr)
    {
      pInstance->RemoveSharedInstance(pSharedInstanceOwner);
      return; // never delete these
    }

    if (!pInstance->m_bIsFinishing)
    {
      // make sure not to insert it into m_FinishingEffects twice
      pInstance->m_bIsFinishing = true;
      pInstance->SetEmitterEnabled(false);
      m_FinishingEffects.PushBack(pInstance);

      if (!bInterruptImmediately)
      {
        m_NeedFinisherComponent.PushBack(pInstance);
      }
    }

    if (bInterruptImmediately)
    {
      pInstance->Interrupt();
    }
  }
}

bool xiiParticleWorldModule::TryGetEffectInstance(const xiiParticleEffectHandle& hEffect, xiiParticleEffectInstance*& out_pEffect)
{
  return m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), out_pEffect);
}

bool xiiParticleWorldModule::TryGetEffectInstance(const xiiParticleEffectHandle& hEffect, const xiiParticleEffectInstance*& out_pEffect) const
{
  xiiParticleEffectInstance* pEffect = nullptr;
  bool                       bResult = m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), pEffect);
  out_pEffect                        = pEffect;
  return bResult;
}

void xiiParticleWorldModule::UpdateEffects(const xiiWorldModule::UpdateContext& context)
{
  XII_LOCK(m_Mutex);

  DestroyFinishedEffects();
  ReconfigureEffects();

  m_EffectUpdateTaskGroup = xiiTaskSystem::CreateTaskGroup(xiiTaskPriority::LateThisFrame);

  const xiiTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  for (xiiUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    if (!m_ParticleEffects[i].ShouldBeUpdated())
      continue;

    m_ParticleEffects[i].ProcessEventQueues();

    const xiiSharedPtr<xiiTask>& pTask                                      = m_ParticleEffects[i].GetUpdateTask();
    static_cast<xiiParticleEffectUpdateTask*>(pTask.Borrow())->m_UpdateDiff = tDiff;

    xiiTaskSystem::AddTaskToGroup(m_EffectUpdateTaskGroup, pTask);
  }

  xiiTaskSystem::StartTaskGroup(m_EffectUpdateTaskGroup);
}

void xiiParticleWorldModule::DestroyFinishedEffects()
{
  XII_LOCK(m_Mutex);

  for (xiiUInt32 i = 0; i < m_FinishingEffects.GetCount();)
  {
    xiiParticleEffectInstance* pEffect = m_FinishingEffects[i];

    if (!pEffect->HasActiveParticles())
    {
      if (m_ActiveEffects.Remove(pEffect->GetHandle().GetInternalID()))
      {
        pEffect->Destruct();

        m_ParticleEffectsFreeList.PushBack(pEffect);
      }

      m_FinishingEffects.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (xiiUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
  {
    xiiParticleEffectInstance* pEffect = m_NeedFinisherComponent[i];

    CreateFinisherComponent(pEffect);
  }

  m_NeedFinisherComponent.Clear();
}

void xiiParticleWorldModule::ReconfigureEffects()
{
  XII_LOCK(m_Mutex);

  for (auto pEffect : m_EffectsToReconfigure)
  {
    pEffect->Reconfigure(false, xiiArrayPtr<xiiParticleEffectFloatParam>(), xiiArrayPtr<xiiParticleEffectColorParam>());
  }

  m_EffectsToReconfigure.Clear();
}



XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleEffects);
