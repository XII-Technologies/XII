/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>

#include <GraphicsCore/Scene/Implementation/SceneDatabaseManagerState.h>

xiiUniquePtr<xiiSceneDatabaseManagerState> xiiSceneDatabaseManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, SceneDatabaseManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "GeometryResidencyManager",
    "MaterialManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiSceneDatabaseManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiSceneDatabaseManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiSceneDatabaseManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiSceneDatabaseManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiSceneDatabaseContextHandle, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiSceneDatabaseContextHandle>)
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  xiiSceneDatabaseManagerState::Slot* GetSlot(xiiSceneDatabaseManagerState* pState, xiiSceneDatabaseContextHandle handle)
  {
    if (pState == nullptr)
      return nullptr;

    xiiSceneDatabaseManagerState::Slot* pSlot = nullptr;
    return pState->m_Contexts.TryGetValue(handle.GetInternalID(), pSlot) ? pSlot : nullptr;
  }

  void ReleaseExtractedMeshes(xiiSceneDatabaseManagerState::Slot& slot)
  {
    for (auto it = slot.m_ExtractedMeshes.GetIterator(); it.IsValid(); ++it)
    {
      xiiGeometryResidencyManager::ReleaseMeshGeometry(it.Value().m_hGeometry, slot.m_uiLastSynchronizedFrame);
      xiiMaterialManager::ReleaseMaterialResource(it.Value().m_hMaterial);
    }

    slot.m_ExtractedMeshes.Clear();
    slot.m_MeshSources.Clear();
    slot.m_uiLastSynchronizedFrame = 0U;
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

  xiiSceneDatabaseManagerState::Slot slot;
  slot.m_pDatabase = std::move(pDatabase);
  return xiiSceneDatabaseContextHandle(s_pState->m_Contexts.Insert(std::move(slot)));
}

void xiiSceneDatabaseManager::DestroyContext(xiiSceneDatabaseContextHandle handle)
{
  xiiSceneDatabaseManagerState::Slot* pSlot = GetSlot(s_pState.Borrow(), handle);
  if (pSlot == nullptr)
    return;

  ReleaseExtractedMeshes(*pSlot);
  XII_VERIFY(s_pState->m_Contexts.Remove(handle.GetInternalID()), "The scene database context was validated before removal.");
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

xiiResult xiiSceneDatabaseManager::GetExtractedMeshletCapacity(xiiSceneDatabaseContextHandle context, xiiUInt64& out_uiMeshlets)
{
  out_uiMeshlets   = 0;
  const auto* slot = GetSlot(s_pState.Borrow(), context);
  if (!slot || !s_pState->m_bEngineStarted)
    return XII_FAILURE;
  xiiUInt64 total = 0;
  for (auto it = slot->m_ExtractedMeshes.GetIterator(); it.IsValid(); ++it)
  {
    const auto* geometry = xiiGeometryResidencyManager::GetGpuRecord(it.Value().m_hGeometry);
    if (!geometry)
      return XII_FAILURE;
    xiiUInt32 maximum = 0;
    for (xiiUInt32 lod = 0; lod < geometry->m_uiLodCount; ++lod)
      if ((geometry->m_uiResidentLodMask & (1U << lod)) != 0 &&
          (it.Key().m_uiLodIndex == xiiInvalidIndex || it.Key().m_uiLodIndex == lod))
        maximum = xiiMath::Max(maximum, geometry->m_Lods[lod].m_uiMeshletCount);
    total += maximum; // At most UInt32 instances * UInt32 meshlets; the sum fits UInt64.
  }
  out_uiMeshlets = total;
  return XII_SUCCESS;
}

void xiiSceneDatabaseManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Scene database manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiSceneDatabaseManagerState);
}

void xiiSceneDatabaseManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede scene database engine startup.");
  if (s_pState != nullptr)
    s_pState->m_bEngineStarted = true;
}

void xiiSceneDatabaseManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  for (auto it = s_pState->m_Contexts.GetIterator(); it.IsValid(); ++it)
    ReleaseExtractedMeshes(it.Value());
  s_pState->m_bEngineStarted = false;
}

void xiiSceneDatabaseManager::Shutdown()
{
  if (s_pState == nullptr)
    return;

  EngineShutdown();
  s_pState->m_Contexts.Clear();
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
  return m_Handle.IsInvalidated() ? XII_FAILURE : XII_SUCCESS;
}

void xiiSceneDatabaseContext::Shutdown()
{
  if (!m_Handle.IsInvalidated())
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

xiiResult xiiSceneDatabaseContext::SynchronizeExtractedMeshes(const xiiExtractedRenderData& extractedData, xiiUInt64 uiFrameIndex)
{
  return xiiSceneDatabaseManager::SynchronizeExtractedMeshes(m_Handle, extractedData, uiFrameIndex);
}

xiiResult xiiSceneDatabaseContext::SynchronizeExtractedMeshes(xiiArrayPtr<xiiRenderData* const> packets, xiiUInt64 uiFrameIndex)
{
  return xiiSceneDatabaseManager::SynchronizeExtractedMeshes(m_Handle, packets, uiFrameIndex);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneDatabaseManager);
