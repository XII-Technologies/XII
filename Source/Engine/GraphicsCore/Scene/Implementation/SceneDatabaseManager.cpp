/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>

class xiiSceneDatabaseManagerState
{
public:
  struct ExtractedMeshEntry
  {
    xiiSceneObjectHandle      m_hSceneObject;
    xiiMeshResourceHandle     m_hMesh;
    xiiGeometryHandle         m_hGeometry;
    xiiMaterialResourceHandle m_hMaterial;
    xiiMaterialGpuHandle      m_hGpuMaterial;
    xiiUInt64                 m_uiLastSeenFrame = 0U;
  };

  struct Slot
  {
    xiiUniquePtr<xiiSceneDatabase>              m_pDatabase;
    xiiHashTable<xiiUInt32, ExtractedMeshEntry> m_ExtractedMeshes;
    xiiUInt64                                   m_uiLastSynchronizedFrame = 0U;
    xiiUInt32                                   m_uiGeneration            = 1U;
  };

  xiiDynamicArray<Slot>      m_Slots;
  xiiDynamicArray<xiiUInt32> m_FreeSlots;
  bool                       m_bEngineStarted = false;
};

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

  void ReleaseExtractedMeshes(xiiSceneDatabaseManagerState::Slot& slot)
  {
    for (auto it = slot.m_ExtractedMeshes.GetIterator(); it.IsValid(); ++it)
    {
      xiiGeometryResidencyManager::ReleaseMeshGeometry(it.Value().m_hMesh, slot.m_uiLastSynchronizedFrame);
      xiiMaterialManager::ReleaseMaterialResource(it.Value().m_hMaterial);
    }

    slot.m_ExtractedMeshes.Clear();
    slot.m_uiLastSynchronizedFrame = 0U;
  }

  xiiBitflags<xiiSceneObjectFlags> GetSceneFlags(const xiiMeshRenderData& renderData, const xiiMaterialResourceHandle& hMaterial)
  {
    xiiBitflags<xiiSceneObjectFlags> flags = xiiSceneObjectFlags::Default;
    flags.AddOrRemove(xiiSceneObjectFlags::Static, renderData.m_Flags.IsSet(xiiMeshRenderDataFlags::StaticObject));
    flags.Add(xiiSceneObjectFlags::SensorVisible);

    if (hMaterial.IsValid())
    {
      xiiResourceLock<xiiMaterialResource> material(hMaterial, xiiResourceAcquireMode::PointerOnly);
      if (material.GetAcquireResult() == xiiResourceAcquireResult::Final)
      {
        const xiiMaterialRuntimeState& runtimeState = material->GetRuntimeState();
        const bool                     bTranslucent = runtimeState.IsTranslucent();
        flags.AddOrRemove(xiiSceneObjectFlags::Transparent, bTranslucent);
        flags.AddOrRemove(xiiSceneObjectFlags::Occluder, !bTranslucent);

        // Assets authored before explicit routing flags existed used an empty feature mask. Keep
        // their legacy shadow behavior while new materials can opt individual paths out.
        const xiiBitflags<xiiMaterialFeatureFlags> routingFlags = xiiMaterialFeatureFlags::ReceivesLighting |
                                                                    xiiMaterialFeatureFlags::CastsShadows |
                                                                    xiiMaterialFeatureFlags::WritesVelocity;
        if (runtimeState.m_FeatureFlags.IsAnySet(routingFlags))
        {
          flags.AddOrRemove(xiiSceneObjectFlags::CastShadows, runtimeState.m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::CastsShadows));
          flags.AddOrRemove(xiiSceneObjectFlags::ReceiveShadows, runtimeState.m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::ReceivesLighting));
        }
      }
    }
    return flags;
  }

  xiiMaterialResourceHandle SelectMaterial(const xiiMeshRenderData& renderData)
  {
    if (renderData.m_hMaterials.IsEmpty())
      return {};

    xiiUInt32 uiMaterialIndex = 0U;
    if (renderData.m_uiSectionIndex != xiiInvalidIndex && renderData.m_hMesh.IsValid())
    {
      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
      {
        const xiiArrayPtr<const xiiMeshSection> sections = mesh->GetSections();
        if (renderData.m_uiSectionIndex < sections.GetCount())
          uiMaterialIndex = sections[renderData.m_uiSectionIndex].m_uiMaterialIndex;
      }
    }

    return uiMaterialIndex < renderData.m_hMaterials.GetCount() ? renderData.m_hMaterials[uiMaterialIndex] : xiiMaterialResourceHandle{};
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

  ReleaseExtractedMeshes(*pSlot);
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

xiiResult xiiSceneDatabaseManager::SynchronizeExtractedMeshes(xiiSceneDatabaseContextHandle handle, const xiiExtractedRenderData& extractedData, xiiUInt64 uiFrameIndex)
{
  xiiSceneDatabaseManagerState::Slot* pSlot = GetSlot(s_pState.Borrow(), handle);
  if (pSlot == nullptr || !s_pState->m_bEngineStarted || !xiiGeometryResidencyManager::IsInitialized())
    return XII_FAILURE;

  xiiSceneDatabase&                                     scene = *pSlot->m_pDatabase;
  const xiiRenderDataBatch::Iterator<xiiMeshRenderData> meshes(extractedData.GetAllRenderData(), 0U, xiiMath::MaxValue<xiiUInt32>());
  for (auto it = meshes; it.IsValid(); ++it)
  {
    const xiiMeshRenderData& renderData = *it;
    if (!renderData.m_hMesh.IsValid())
      continue;

    const xiiMaterialResourceHandle hMaterial = SelectMaterial(renderData);

    xiiSceneDatabaseManagerState::ExtractedMeshEntry* pEntry = nullptr;
    if (!pSlot->m_ExtractedMeshes.TryGetValue(renderData.m_uiUniqueID, pEntry))
    {
      const xiiGeometryHandle hGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(renderData.m_hMesh);
      if (!hGeometry.IsValid())
        continue;
      const xiiMaterialGpuHandle hGpuMaterial = xiiMaterialManager::AcquireMaterialResource(hMaterial);

      xiiSceneObjectDesc desc;
      desc.m_LocalTransform  = renderData.m_GlobalTransform.GetAsMat4();
      desc.m_Flags           = GetSceneFlags(renderData, hMaterial);
      desc.m_uiGeometryIndex = hGeometry.m_uiIndex;
      desc.m_uiMaterialIndex = hGpuMaterial.IsValid() ? hGpuMaterial.m_uiSlot : xiiInvalidIndex;
      desc.m_uiUserData      = renderData.m_uiUniqueID;

      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
        desc.m_LocalBounds = mesh->GetBounds();

      xiiSceneDatabaseManagerState::ExtractedMeshEntry entry;
      entry.m_hSceneObject    = scene.CreateObject(desc);
      entry.m_hMesh           = renderData.m_hMesh;
      entry.m_hGeometry       = hGeometry;
      entry.m_hMaterial       = hMaterial;
      entry.m_hGpuMaterial    = hGpuMaterial;
      entry.m_uiLastSeenFrame = uiFrameIndex;
      if (!entry.m_hSceneObject.IsValid())
      {
        xiiGeometryResidencyManager::ReleaseMeshGeometry(renderData.m_hMesh, uiFrameIndex);
        xiiMaterialManager::ReleaseMaterialResource(hMaterial);
        continue;
      }

      pSlot->m_ExtractedMeshes.Insert(renderData.m_uiUniqueID, entry);
      pSlot->m_ExtractedMeshes.TryGetValue(renderData.m_uiUniqueID, pEntry);
    }
    else if (pEntry->m_hMesh != renderData.m_hMesh)
    {
      const xiiGeometryHandle hGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(renderData.m_hMesh);
      if (!hGeometry.IsValid())
        continue;

      xiiGeometryResidencyManager::ReleaseMeshGeometry(pEntry->m_hMesh, uiFrameIndex);
      pEntry->m_hMesh     = renderData.m_hMesh;
      pEntry->m_hGeometry = hGeometry;
      scene.SetGeometry(pEntry->m_hSceneObject, hGeometry.m_uiIndex);

      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
        scene.SetLocalBounds(pEntry->m_hSceneObject, mesh->GetBounds());
    }

    if (pEntry != nullptr && pEntry->m_hMaterial != hMaterial)
    {
      const xiiMaterialGpuHandle hGpuMaterial = xiiMaterialManager::AcquireMaterialResource(hMaterial);
      xiiMaterialManager::ReleaseMaterialResource(pEntry->m_hMaterial);
      pEntry->m_hMaterial    = hMaterial;
      pEntry->m_hGpuMaterial = hGpuMaterial;
      scene.SetMaterial(pEntry->m_hSceneObject, hGpuMaterial.IsValid() ? hGpuMaterial.m_uiSlot : xiiInvalidIndex);
    }

    if (pEntry == nullptr)
      continue;

    pEntry->m_uiLastSeenFrame = uiFrameIndex;
    scene.SetLocalTransform(pEntry->m_hSceneObject, renderData.m_GlobalTransform.GetAsMat4());
    scene.SetFlags(pEntry->m_hSceneObject, GetSceneFlags(renderData, hMaterial));
    xiiGeometryResidencyManager::RequestResidency(pEntry->m_hGeometry, renderData.m_uiLODIndex, uiFrameIndex);
    xiiGeometryResidencyManager::Touch(pEntry->m_hGeometry, uiFrameIndex);
  }

  for (auto it = pSlot->m_ExtractedMeshes.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenFrame == uiFrameIndex)
    {
      ++it;
      continue;
    }

    scene.DestroyObject(it.Value().m_hSceneObject);
    xiiGeometryResidencyManager::ReleaseMeshGeometry(it.Value().m_hMesh, uiFrameIndex);
    xiiMaterialManager::ReleaseMaterialResource(it.Value().m_hMaterial);
    it = pSlot->m_ExtractedMeshes.Remove(it);
  }

  scene.CommitFrame(uiFrameIndex);
  pSlot->m_uiLastSynchronizedFrame = uiFrameIndex;
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

  for (xiiSceneDatabaseManagerState::Slot& slot : s_pState->m_Slots)
    ReleaseExtractedMeshes(slot);
  s_pState->m_bEngineStarted = false;
}

void xiiSceneDatabaseManager::Shutdown()
{
  if (s_pState == nullptr)
    return;

  EngineShutdown();
  for (xiiSceneDatabaseManagerState::Slot& slot : s_pState->m_Slots)
  {
    slot.m_pDatabase.Clear();
  }

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

xiiResult xiiSceneDatabaseContext::SynchronizeExtractedMeshes(const xiiExtractedRenderData& extractedData, xiiUInt64 uiFrameIndex)
{
  return xiiSceneDatabaseManager::SynchronizeExtractedMeshes(m_Handle, extractedData, uiFrameIndex);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneDatabaseManager);
