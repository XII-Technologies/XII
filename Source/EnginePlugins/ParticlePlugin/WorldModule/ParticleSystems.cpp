#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

xiiParticleSystemInstance* xiiParticleWorldModule::CreateSystemInstance(
  xiiUInt32                  uiMaxParticles,
  xiiWorld*                  pWorld,
  xiiParticleEffectInstance* pOwnerEffect,
  float                      fSpawnMultiplier)
{
  XII_LOCK(m_Mutex);

  xiiParticleSystemInstance* pResult = nullptr;

  if (!m_ParticleSystemFreeList.IsEmpty())
  {
    pResult = m_ParticleSystemFreeList.PeekBack();
    m_ParticleSystemFreeList.PopBack();
  }

  if (pResult == nullptr)
  {
    pResult = &m_ParticleSystems.ExpandAndGetRef();
  }

  pResult->Construct(uiMaxParticles, pWorld, pOwnerEffect, fSpawnMultiplier);

  return pResult;
}

void xiiParticleWorldModule::DestroySystemInstance(xiiParticleSystemInstance* pInstance)
{
  XII_LOCK(m_Mutex);

  XII_ASSERT_DEBUG(pInstance != nullptr, "Invalid particle system");
  pInstance->Destruct();
  m_ParticleSystemFreeList.PushBack(pInstance);
}


XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleSystems);
