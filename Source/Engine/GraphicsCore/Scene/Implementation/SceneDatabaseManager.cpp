/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>

class xiiSceneDatabaseManagerState
{
public:
  struct Slot
  {
    xiiUniquePtr<xiiSceneDatabase> m_pDatabase;
    xiiUInt32                      m_uiGeneration = 1U;
  };

  xiiDynamicArray<Slot>      m_Slots;
  xiiDynamicArray<xiiUInt32> m_FreeSlots;
};

xiiUniquePtr<xiiSceneDatabaseManagerState> xiiSceneDatabaseManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, SceneDatabaseManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiSceneDatabaseManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiSceneDatabaseManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneDatabaseContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneDatabaseContextHandle>)
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
  xiiSceneDatabaseManagerState::Slot* GetSlot(xiiSceneDatabaseManagerState* pState, xiiSceneDatabaseContextHandle handle)
  {
    if (pState == nullptr || !handle.IsValid() || handle.m_uiIndex >= pState->m_Slots.GetCount())
      return nullptr;

    xiiSceneDatabaseManagerState::Slot& slot = pState->m_Slots[handle.m_uiIndex];
    return slot.m_uiGeneration == handle.m_uiGeneration && slot.m_pDatabase != nullptr ? &slot : nullptr;
  }
} // namespace

bool xiiSceneDatabaseManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiSceneDatabaseManager::IsInitialized()
{
  return s_pState != nullptr;
}

xiiSceneDatabaseContextHandle xiiSceneDatabaseManager::CreateContext(xiiUInt32 uiInitialCapacity)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The scene database manager is not initialized.");
  if (s_pState == nullptr)
    return {};

  xiiUniquePtr<xiiSceneDatabase> pDatabase = XII_DEFAULT_NEW(xiiSceneDatabase);
  if (uiInitialCapacity > 0U)
    pDatabase->Reserve(uiInitialCapacity);

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

  xiiSceneDatabaseManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
  slot.m_pDatabase                         = std::move(pDatabase);

  xiiSceneDatabaseContextHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiSceneDatabaseManager::DestroyContext(xiiSceneDatabaseContextHandle handle)
{
  xiiSceneDatabaseManagerState::Slot* pSlot = GetSlot(s_pState.Borrow(), handle);
  if (pSlot == nullptr)
    return;

  pSlot->m_pDatabase.Clear();
  ++pSlot->m_uiGeneration;
  if (pSlot->m_uiGeneration == 0U)
    pSlot->m_uiGeneration = 1U;
  s_pState->m_FreeSlots.PushBack(handle.m_uiIndex);
}

bool xiiSceneDatabaseManager::IsValid(xiiSceneDatabaseContextHandle handle)
{
  return GetSlot(s_pState.Borrow(), handle) != nullptr;
}

xiiSceneDatabase* xiiSceneDatabaseManager::GetDatabase(xiiSceneDatabaseContextHandle handle)
{
  xiiSceneDatabaseManagerState::Slot* pSlot = GetSlot(s_pState.Borrow(), handle);
  return pSlot != nullptr ? pSlot->m_pDatabase.Borrow() : nullptr;
}

void xiiSceneDatabaseManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Scene database manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiSceneDatabaseManagerState);
}

void xiiSceneDatabaseManager::Shutdown()
{
  if (s_pState == nullptr)
    return;

  for (xiiSceneDatabaseManagerState::Slot& slot : s_pState->m_Slots)
    slot.m_pDatabase.Clear();

  s_pState.Clear();
}

xiiSceneDatabaseContext::~xiiSceneDatabaseContext()
{
  Shutdown();
}

xiiResult xiiSceneDatabaseContext::Initialize(xiiUInt32 uiInitialCapacity)
{
  Shutdown();
  m_Handle = xiiSceneDatabaseManager::CreateContext(uiInitialCapacity);
  return m_Handle.IsValid() ? XII_SUCCESS : XII_FAILURE;
}

void xiiSceneDatabaseContext::Shutdown()
{
  if (m_Handle.IsValid())
    xiiSceneDatabaseManager::DestroyContext(m_Handle);
  m_Handle = {};
}

bool xiiSceneDatabaseContext::IsInitialized() const
{
  return xiiSceneDatabaseManager::IsValid(m_Handle);
}

xiiSceneDatabase& xiiSceneDatabaseContext::GetDatabase()
{
  xiiSceneDatabase* pDatabase = xiiSceneDatabaseManager::GetDatabase(m_Handle);
  XII_ASSERT_DEV(pDatabase != nullptr, "Scene database context is not initialized.");
  return *pDatabase;
}

const xiiSceneDatabase& xiiSceneDatabaseContext::GetDatabase() const
{
  const xiiSceneDatabase* pDatabase = xiiSceneDatabaseManager::GetDatabase(m_Handle);
  XII_ASSERT_DEV(pDatabase != nullptr, "Scene database context is not initialized.");
  return *pDatabase;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneDatabaseManager);
