/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Pipeline/ViewRenderResourceManager.h>
#include <GraphicsFoundation/Device/Device.h>

class xiiViewRenderResourceManagerState
{
public:
  xiiIdTable<xiiViewRenderResourceContextId, xiiUniquePtr<xiiView::ViewPassResources>> m_Contexts;
  bool                                                                                 m_bEngineStarted = false;
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
XII_END_STATIC_REFLECTED_TYPE;

bool xiiViewRenderResourceManager::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiViewRenderResourceManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

xiiViewRenderResourceContextHandle xiiViewRenderResourceManager::CreateContext()
{
  XII_ASSERT_DEV(IsInitialized(), "The view render-resource manager is not initialized.");
  if (!IsInitialized())
    return {};

  return xiiViewRenderResourceContextHandle(s_pState->m_Contexts.Insert(XII_DEFAULT_NEW(xiiView::ViewPassResources)));
}

void xiiViewRenderResourceManager::DestroyContext(xiiViewRenderResourceContextHandle handle)
{
  if (!IsValid(handle))
    return;

  XII_VERIFY(s_pState->m_Contexts.Remove(handle.GetInternalID()), "The view render-resource context was validated before removal.");
}

bool xiiViewRenderResourceManager::IsValid(xiiViewRenderResourceContextHandle handle)
{
  return GetContext(handle) != nullptr;
}

void* xiiViewRenderResourceManager::GetContext(xiiViewRenderResourceContextHandle handle)
{
  if (s_pState == nullptr)
    return nullptr;

  xiiUniquePtr<xiiView::ViewPassResources>* pResources = nullptr;
  return s_pState->m_Contexts.TryGetValue(handle.GetInternalID(), pResources) ? pResources->Borrow() : nullptr;
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

  s_pState->m_Contexts.Clear();

  s_pState->m_bEngineStarted = false;
}

void xiiViewRenderResourceManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_ViewRenderResourceManager);
