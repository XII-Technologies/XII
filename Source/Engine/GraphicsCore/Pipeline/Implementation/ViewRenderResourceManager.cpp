/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Pipeline/ViewRenderResourceManager.h>
#include <GraphicsFoundation/Device/Device.h>

class xiiViewRenderResourceManagerState
{
public:
  struct Slot
  {
    xiiUniquePtr<xiiView::ViewPassResources> m_pResources;
    xiiUInt32                                m_uiGeneration = 1U;
  };

  xiiDynamicArray<Slot>      m_Slots;
  xiiDynamicArray<xiiUInt32> m_FreeSlots;
  bool                       m_bEngineStarted = false;
};

xiiUniquePtr<xiiViewRenderResourceManagerState> xiiViewRenderResourceManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, ViewRenderResourceManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "LightingManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiViewRenderResourceManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiViewRenderResourceManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiViewRenderResourceManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiViewRenderResourceManager::Shutdown();
  }

  // Keep this terminator on a unique source line because subsystem symbols use __LINE__ in unity builds.
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiViewRenderResourceContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiViewRenderResourceContextHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

bool xiiViewRenderResourceManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

xiiViewRenderResourceContextHandle xiiViewRenderResourceManager::CreateContext()
{
  XII_ASSERT_DEV(IsInitialized(), "The view render-resource manager is not initialized.");
  if (!IsInitialized())
    return {};

  xiiUInt32 uiIndex = xiiInvalidIndex;
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

  xiiViewRenderResourceManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
  slot.m_pResources                            = XII_DEFAULT_NEW(xiiView::ViewPassResources);

  xiiViewRenderResourceContextHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiViewRenderResourceManager::DestroyContext(xiiViewRenderResourceContextHandle handle)
{
  if (!IsValid(handle))
    return;

  xiiViewRenderResourceManagerState::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  slot.m_pResources.Clear();
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  s_pState->m_FreeSlots.PushBack(handle.m_uiIndex);
}

bool xiiViewRenderResourceManager::IsValid(xiiViewRenderResourceContextHandle handle)
{
  return GetContext(handle) != nullptr;
}

void* xiiViewRenderResourceManager::GetContext(xiiViewRenderResourceContextHandle handle)
{
  if (s_pState == nullptr || !handle.IsValid() || handle.m_uiIndex >= s_pState->m_Slots.GetCount())
    return nullptr;

  xiiViewRenderResourceManagerState::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  return slot.m_uiGeneration == handle.m_uiGeneration ? slot.m_pResources.Borrow() : nullptr;
}

const void* xiiViewRenderResourceManager::GetContextConst(xiiViewRenderResourceContextHandle handle)
{
  return GetContext(handle);
}

void xiiViewRenderResourceManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "View render-resource manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiViewRenderResourceManagerState);
}

void xiiViewRenderResourceManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede view render-resource manager engine startup.");
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiViewRenderResourceManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  for (xiiUInt32 uiIndex = 0U; uiIndex < s_pState->m_Slots.GetCount(); ++uiIndex)
  {
    xiiViewRenderResourceManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
    if (slot.m_pResources == nullptr)
      continue;

    slot.m_pResources.Clear();
    ++slot.m_uiGeneration;
    if (slot.m_uiGeneration == 0U)
      slot.m_uiGeneration = 1U;
    s_pState->m_FreeSlots.PushBack(uiIndex);
  }

  s_pState->m_bEngineStarted = false;
}

void xiiViewRenderResourceManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_ViewRenderResourceManager);
