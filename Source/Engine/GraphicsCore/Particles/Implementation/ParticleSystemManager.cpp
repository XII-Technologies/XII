/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Particles/ParticleSystemManager.h>

class xiiParticleSystemManagerState
{
public:
  struct Slot
  {
    xiiUniquePtr<xiiParticleSystemRuntime> m_pRuntime;
    xiiUInt32                              m_uiGeneration = 1U;
  };

  xiiDynamicArray<Slot>      m_Slots;
  xiiDynamicArray<xiiUInt32> m_FreeSlots;
  bool                       m_bEngineStarted = false;
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
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  xiiParticleSystemManagerState::Slot* GetSlot(xiiParticleSystemManagerState* pState, xiiParticleSystemRuntimeHandle handle)
  {
    if (pState == nullptr || !handle.IsValid() || handle.m_uiIndex >= pState->m_Slots.GetCount())
      return nullptr;

    xiiParticleSystemManagerState::Slot& slot = pState->m_Slots[handle.m_uiIndex];
    return slot.m_uiGeneration == handle.m_uiGeneration && slot.m_pRuntime != nullptr ? &slot : nullptr;
  }

  void InvalidateSlot(xiiParticleSystemManagerState& state, xiiUInt32 uiIndex)
  {
    xiiParticleSystemManagerState::Slot& slot = state.m_Slots[uiIndex];
    if (slot.m_pRuntime != nullptr)
    {
      slot.m_pRuntime->Shutdown();
      slot.m_pRuntime.Clear();
    }

    ++slot.m_uiGeneration;
    if (slot.m_uiGeneration == 0U)
      slot.m_uiGeneration = 1U;
    state.m_FreeSlots.PushBack(uiIndex);
  }
} // namespace

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

  xiiUInt32 uiIndex;
  if (!s_pState->m_FreeSlots.IsEmpty())
  {
    uiIndex = s_pState->m_FreeSlots.PeekBack();
    s_pState->m_FreeSlots.PopBack();
  }
  else
  {
    uiIndex = s_pState->m_Slots.GetCount();
    s_pState->m_Slots.ExpandAndGetRef();
  }

  xiiParticleSystemManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
  slot.m_pRuntime                          = std::move(pRuntime);

  xiiParticleSystemRuntimeHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiParticleSystemManager::DestroyRuntime(xiiParticleSystemRuntimeHandle handle)
{
  if (GetSlot(s_pState.Borrow(), handle) == nullptr)
    return;

  InvalidateSlot(*s_pState, handle.m_uiIndex);
}

bool xiiParticleSystemManager::IsValid(xiiParticleSystemRuntimeHandle handle)
{
  return GetSlot(s_pState.Borrow(), handle) != nullptr;
}

xiiParticleSystemRuntime* xiiParticleSystemManager::GetRuntime(xiiParticleSystemRuntimeHandle handle)
{
  xiiParticleSystemManagerState::Slot* pSlot = GetSlot(s_pState.Borrow(), handle);
  return pSlot != nullptr ? pSlot->m_pRuntime.Borrow() : nullptr;
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

  for (xiiUInt32 uiIndex = 0U; uiIndex < s_pState->m_Slots.GetCount(); ++uiIndex)
  {
    if (s_pState->m_Slots[uiIndex].m_pRuntime != nullptr)
      InvalidateSlot(*s_pState, uiIndex);
  }
  s_pState->m_bEngineStarted = false;
}

void xiiParticleSystemManager::Shutdown()
{
  if (s_pState == nullptr)
    return;

  for (xiiParticleSystemManagerState::Slot& slot : s_pState->m_Slots)
  {
    if (slot.m_pRuntime != nullptr)
      slot.m_pRuntime->Shutdown();
    slot.m_pRuntime.Clear();
  }
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Particles_Implementation_ParticleSystemManager);
