/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Visibility/GpuVisibilityManager.h>
#include <GraphicsFoundation/Device/Device.h>

class xiiGpuVisibilityManagerState
{
public:
  struct Slot
  {
    xiiUniquePtr<xiiGpuVisibilitySystem> m_pSystem;
    xiiUInt32                            m_uiGeneration = 1U;
  };

  xiiDynamicArray<Slot>      m_Slots;
  xiiDynamicArray<xiiUInt32> m_FreeSlots;
  bool                       m_bEngineStarted = false;
};

xiiUniquePtr<xiiGpuVisibilityManagerState> xiiGpuVisibilityManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, GpuVisibilityManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "SceneDatabaseManager",
    "GeometryResidencyManager",
    "PipelineCache",
    "ShaderPermutationUtilities"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGpuVisibilityManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGpuVisibilityManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGpuVisibilityManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGpuVisibilityManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuVisibilityContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuVisibilityContextHandle>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Index", m_uiIndex),
      XII_MEMBER_PROPERTY("Generation", m_uiGeneration),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

bool xiiGpuVisibilityManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bEngineStarted && xiiGALDevice::HasDefaultDevice();
}

xiiGpuVisibilityContextHandle xiiGpuVisibilityManager::CreateContext(const xiiGpuVisibilityDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The GPU visibility manager is not initialized.");
  if (!IsInitialized())
    return {};

  const xiiSharedPtr<xiiGALDevice>     pDevice = xiiGALDevice::GetDefaultDevice();
  xiiUniquePtr<xiiGpuVisibilitySystem> pSystem = XII_DEFAULT_NEW(xiiGpuVisibilitySystem);
  if (pSystem->Initialize(pDevice.Borrow(), description).Failed())
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

  xiiGpuVisibilityManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
  slot.m_pSystem                           = std::move(pSystem);

  xiiGpuVisibilityContextHandle handle;
  handle.m_uiIndex      = uiIndex;
  handle.m_uiGeneration = slot.m_uiGeneration;
  return handle;
}

void xiiGpuVisibilityManager::DestroyContext(xiiGpuVisibilityContextHandle handle)
{
  xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  if (pSystem == nullptr)
    return;

  xiiGpuVisibilityManagerState::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  pSystem->Shutdown();
  slot.m_pSystem.Clear();
  ++slot.m_uiGeneration;
  if (slot.m_uiGeneration == 0U)
    slot.m_uiGeneration = 1U;
  s_pState->m_FreeSlots.PushBack(handle.m_uiIndex);
}

bool xiiGpuVisibilityManager::IsValid(xiiGpuVisibilityContextHandle handle)
{
  return GetSystem(handle) != nullptr;
}

xiiGpuVisibilityOutputs xiiGpuVisibilityManager::AddPasses(xiiGpuVisibilityContextHandle handle, xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiSceneDatabaseContextHandle sceneHandle, const xiiGpuVisibilityView& view, const xiiGeometryResidencyManager::UploadHandles& geometry, const xiiGpuVisibilityPassDescription& description, xiiRenderGraphTextureHandle hHiZ)
{
  xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  xiiSceneDatabase*       pScene  = xiiSceneDatabaseManager::GetDatabase(sceneHandle);
  XII_ASSERT_DEV(pSystem != nullptr, "GPU visibility context is invalid or has already been destroyed.");
  XII_ASSERT_DEV(pScene != nullptr, "Scene database context is invalid or has already been destroyed.");
  return pSystem != nullptr && pScene != nullptr ? pSystem->AddPasses(graph, uiFrameIndex, *pScene, view, geometry, description, hHiZ) : xiiGpuVisibilityOutputs{};
}

xiiUInt32 xiiGpuVisibilityManager::GetMeshDispatchGroupCountX(xiiGpuVisibilityContextHandle handle)
{
  const xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->GetMeshDispatchGroupCountX() : 0U;
}

xiiUInt32 xiiGpuVisibilityManager::GetMeshDispatchGroupCountY(xiiGpuVisibilityContextHandle handle)
{
  const xiiGpuVisibilitySystem* pSystem = GetSystem(handle);
  return pSystem != nullptr ? pSystem->GetMeshDispatchGroupCountY() : 0U;
}

void xiiGpuVisibilityManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "GPU visibility manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiGpuVisibilityManagerState);
}

void xiiGpuVisibilityManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede GPU visibility manager engine startup.");
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiGpuVisibilityManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  for (xiiUInt32 uiIndex = 0U; uiIndex < s_pState->m_Slots.GetCount(); ++uiIndex)
  {
    xiiGpuVisibilityManagerState::Slot& slot = s_pState->m_Slots[uiIndex];
    if (slot.m_pSystem == nullptr)
      continue;

    slot.m_pSystem->Shutdown();
    slot.m_pSystem.Clear();
    ++slot.m_uiGeneration;
    if (slot.m_uiGeneration == 0U)
      slot.m_uiGeneration = 1U;
    s_pState->m_FreeSlots.PushBack(uiIndex);
  }
  s_pState->m_bEngineStarted = false;
}

void xiiGpuVisibilityManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiGpuVisibilitySystem* xiiGpuVisibilityManager::GetSystem(xiiGpuVisibilityContextHandle handle)
{
  if (s_pState == nullptr || !handle.IsValid() || handle.m_uiIndex >= s_pState->m_Slots.GetCount())
    return nullptr;

  xiiGpuVisibilityManagerState::Slot& slot = s_pState->m_Slots[handle.m_uiIndex];
  return slot.m_uiGeneration == handle.m_uiGeneration ? slot.m_pSystem.Borrow() : nullptr;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuVisibilityManager);
