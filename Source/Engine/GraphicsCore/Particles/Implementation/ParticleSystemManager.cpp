/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <GraphicsCore/Particles/ParticleSystemManager.h>

class xiiParticleSystemManagerState
{
public:
  xiiIdTable<xiiParticleSystemRuntimeId, xiiUniquePtr<xiiParticleSystemRuntime>> m_Runtimes;
  bool                                                                           m_bEngineStarted = false;
};

xiiUniquePtr<xiiParticleSystemManagerState> xiiParticleSystemManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, ParticleSystemManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiParticleSystemManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiParticleSystemManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiParticleSystemManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiParticleSystemManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiParticleSystemRuntimeHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiParticleSystemRuntimeHandle>)
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  xiiParticleSystemRuntime* ResolveRuntime(xiiParticleSystemManagerState* pState, xiiParticleSystemRuntimeHandle handle)
  {
    if (pState == nullptr)
      return nullptr;

    xiiUniquePtr<xiiParticleSystemRuntime>* pRuntime = nullptr;
    return pState->m_Runtimes.TryGetValue(handle.GetInternalID(), pRuntime) ? pRuntime->Borrow() : nullptr;
  }
} // namespace

bool xiiParticleSystemManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiParticleSystemManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted;
}

xiiParticleSystemRuntimeHandle xiiParticleSystemManager::CreateRuntime(xiiSharedPtr<xiiGALDevice> pDevice, const xiiParticleSystemDescriptor& descriptor)
{
  XII_ASSERT_DEV(IsInitialized(), "The particle system manager is not initialized.");
  if (!IsInitialized() || pDevice == nullptr)
    return {};

  xiiUniquePtr<xiiParticleSystemRuntime> pRuntime = XII_DEFAULT_NEW(xiiParticleSystemRuntime);
  if (pRuntime->Initialize(std::move(pDevice), descriptor).Failed())
    return {};

  return xiiParticleSystemRuntimeHandle(s_pState->m_Runtimes.Insert(std::move(pRuntime)));
}

void xiiParticleSystemManager::DestroyRuntime(xiiParticleSystemRuntimeHandle handle)
{
  xiiParticleSystemRuntime* pRuntime = ResolveRuntime(s_pState.Borrow(), handle);
  if (pRuntime == nullptr)
    return;

  pRuntime->Shutdown();
  XII_VERIFY(s_pState->m_Runtimes.Remove(handle.GetInternalID()), "The particle runtime was validated before removal.");
}

bool xiiParticleSystemManager::IsValid(xiiParticleSystemRuntimeHandle handle)
{
  return ResolveRuntime(s_pState.Borrow(), handle) != nullptr;
}

xiiParticleSystemRuntime* xiiParticleSystemManager::GetRuntime(xiiParticleSystemRuntimeHandle handle)
{
  return ResolveRuntime(s_pState.Borrow(), handle);
}

void xiiParticleSystemManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Particle system manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiParticleSystemManagerState);
}

void xiiParticleSystemManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede particle system manager engine startup.");
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiParticleSystemManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  for (auto it = s_pState->m_Runtimes.GetIterator(); it.IsValid(); ++it)
    it.Value()->Shutdown();
  s_pState->m_Runtimes.Clear();
  s_pState->m_bEngineStarted = false;
}

void xiiParticleSystemManager::Shutdown()
{
  if (s_pState == nullptr)
    return;

  EngineShutdown();
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Particles_Implementation_ParticleSystemManager);
