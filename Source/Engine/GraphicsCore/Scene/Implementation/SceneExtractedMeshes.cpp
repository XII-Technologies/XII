/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Scene/Implementation/SceneDatabaseManagerState.h>

namespace
{
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

  xiiMaterialResourceHandle SelectMaterial(const xiiMeshRenderData& renderData, xiiUInt32& out_uiMaterialIndex)
  {
    out_uiMaterialIndex = 0U;
    if (renderData.m_uiSectionIndex != xiiInvalidIndex && renderData.m_hMesh.IsValid())
    {
      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh->GetLoadingState() == xiiResourceState::Loaded)
      {
        const auto                              lods     = mesh->GetLODs();
        const xiiArrayPtr<const xiiMeshSection> sections = lods.IsEmpty() ? mesh->GetSections() : lods[xiiMath::Min(renderData.m_uiLODIndex, lods.GetCount() - 1)].m_Sections;
        if (renderData.m_uiSectionIndex < sections.GetCount())
          out_uiMaterialIndex = sections[renderData.m_uiSectionIndex].m_uiMaterialIndex;
      }
    }

    return out_uiMaterialIndex < renderData.m_hMaterials.GetCount() ? renderData.m_hMaterials[out_uiMaterialIndex] : xiiMaterialResourceHandle{};
  }
} // namespace

xiiResult xiiSceneDatabaseManager::SynchronizeExtractedMeshes(xiiSceneDatabaseContextHandle handle, const xiiExtractedRenderData& extractedData, xiiUInt64 uiFrameIndex)
{
  return SynchronizeExtractedMeshes(handle, extractedData.GetAllRenderData(), uiFrameIndex);
}

xiiResult xiiSceneDatabaseManager::SynchronizeExtractedMeshes(xiiSceneDatabaseContextHandle handle, xiiArrayPtr<xiiRenderData* const> packets, xiiUInt64 uiFrameIndex)
{
  xiiSceneDatabaseManagerState::Slot* pSlot = nullptr;
  if (!s_pState || !s_pState->m_Contexts.TryGetValue(handle.GetInternalID(), pSlot) || !s_pState->m_bEngineStarted || !xiiGeometryResidencyManager::IsInitialized())
    return XII_FAILURE;

  // Reject invalid selection controls before changing membership or acquiring resources.
  for (const auto* data : packets)
  {
    const auto* mesh = xiiDynamicCast<const xiiMeshRenderData*>(data);
    if (mesh && mesh->m_hMesh.IsValid() &&
        (!xiiMath::IsFinite(mesh->m_fLodBias) || (mesh->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) && mesh->m_uiLODIndex >= 8U)))
      return XII_FAILURE;
  }

  xiiSceneDatabase& scene = *pSlot->m_pDatabase;
  // Several filtered snapshots can be synchronized during one CPU frame. Membership belongs
  // to this call, while resource retirement still uses the GPU frame timeline.
  const auto                                            synchronization = ++pSlot->m_uiSynchronization;
  const xiiRenderDataBatch::Iterator<xiiMeshRenderData> meshes(packets, 0U, xiiMath::MaxValue<xiiUInt32>());
  for (auto it = meshes; it.IsValid(); ++it)
  {
    const xiiMeshRenderData& renderData = *it;
    if (!renderData.m_hMesh.IsValid())
      continue;

    const xiiUInt32                             forcedLod     = renderData.m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) ? renderData.m_uiLODIndex : xiiInvalidIndex;
    xiiUInt32                                   materialIndex = 0;
    const xiiMaterialResourceHandle             hMaterial     = SelectMaterial(renderData, materialIndex);
    const xiiSceneDatabaseManagerState::MeshKey key{renderData.m_hOwnerObject, renderData.m_hOwnerComponent, renderData.m_uiUniqueID, renderData.m_uiSectionIndex};

    xiiSceneDatabaseManagerState::ExtractedMeshEntry* pEntry = nullptr;
    pSlot->m_ExtractedMeshes.TryGetValue(key, pEntry);
    // A consumer may clear its database between frames. Retire the extraction resources as well
    // before recreating entries; stale scene handles must never update a newly reused slot.
    if (pEntry != nullptr && !scene.IsAlive(pEntry->m_hSceneObject))
    {
      xiiGeometryResidencyManager::ReleaseMeshGeometry(pEntry->m_hGeometry, uiFrameIndex);
      xiiMaterialManager::ReleaseMaterialResource(pEntry->m_hMaterial);
      pSlot->m_MeshSources.Remove(pEntry->m_hSceneObject.GetInternalID().m_Data + 1U);
      pSlot->m_ExtractedMeshes.Remove(key);
      pEntry = nullptr;
    }
    if (pEntry == nullptr)
    {
      const xiiGeometryHandle hGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(renderData.m_hMesh);
      if (hGeometry.IsInvalidated())
        continue;
      const xiiMaterialGpuHandle hGpuMaterial = xiiMaterialManager::AcquireMaterialResource(hMaterial);

      xiiSceneObjectDesc desc;
      desc.m_LocalTransform   = renderData.m_GlobalTransform.GetAsMat4();
      desc.m_Flags            = GetSceneFlags(renderData, hMaterial);
      desc.m_uiGeometryIndex  = hGeometry.GetInternalID().m_InstanceIndex;
      desc.m_uiMaterialIndex  = !hGpuMaterial.IsInvalidated() ? hGpuMaterial.GetInternalID().m_InstanceIndex : xiiInvalidIndex;
      desc.m_uiUserData       = renderData.m_uiUniqueID;
      desc.m_uiFirstPrimitive = renderData.m_uiFirstPrimitive;
      desc.m_uiPrimitiveCount = renderData.m_uiPrimitiveCount;
      desc.m_uiForcedLod      = forcedLod;
      desc.m_fLodBias         = renderData.m_fLodBias;

      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh.GetAcquireResult() == xiiResourceAcquireResult::Final)
        desc.m_LocalBounds = mesh->GetBounds();

      xiiSceneDatabaseManagerState::ExtractedMeshEntry entry;
      entry.m_hSceneObject      = scene.CreateObject(desc);
      entry.m_hMesh             = renderData.m_hMesh;
      entry.m_hGeometry         = hGeometry;
      entry.m_hMaterial         = hMaterial;
      entry.m_hGpuMaterial      = hGpuMaterial;
      entry.m_uiSynchronization = synchronization;
      if (entry.m_hSceneObject.IsInvalidated())
      {
        xiiGeometryResidencyManager::ReleaseMeshGeometry(hGeometry, uiFrameIndex);
        xiiMaterialManager::ReleaseMaterialResource(hMaterial);
        continue;
      }

      pSlot->m_ExtractedMeshes.Insert(key, entry);
      pSlot->m_ExtractedMeshes.TryGetValue(key, pEntry);
      xiiSceneMeshSource source;
      source.m_hSceneObject    = entry.m_hSceneObject;
      source.m_hOwnerObject    = key.m_hObject;
      source.m_hOwnerComponent = key.m_hComponent;
      source.m_uiSectionIndex  = key.m_uiSectionIndex;
      source.m_uiMaterialIndex = materialIndex;
      source.m_uiRenderDataId  = key.m_uiRenderDataId;
      pSlot->m_MeshSources.Insert(entry.m_hSceneObject.GetInternalID().m_Data + 1U, source);
    }
    else
    {
      // Resource handles survive content reloads. Acquire the current content revision before
      // releasing the previous one, including when two views migrate during different frames.
      const xiiGeometryHandle hGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(renderData.m_hMesh);
      if (hGeometry.IsInvalidated())
        continue;

      xiiGeometryResidencyManager::ReleaseMeshGeometry(pEntry->m_hGeometry, uiFrameIndex);
      const bool changed  = pEntry->m_hGeometry != hGeometry;
      pEntry->m_hMesh     = renderData.m_hMesh;
      pEntry->m_hGeometry = hGeometry;
      if (changed)
      {
        scene.SetGeometry(pEntry->m_hSceneObject, hGeometry.GetInternalID().m_InstanceIndex);
        xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
        if (mesh->GetLoadingState() == xiiResourceState::Loaded)
          scene.SetLocalBounds(pEntry->m_hSceneObject, mesh->GetBounds());
      }
    }

    if (pEntry != nullptr && pEntry->m_hMaterial != hMaterial)
    {
      const xiiMaterialGpuHandle hGpuMaterial = xiiMaterialManager::AcquireMaterialResource(hMaterial);
      xiiMaterialManager::ReleaseMaterialResource(pEntry->m_hMaterial);
      pEntry->m_hMaterial    = hMaterial;
      pEntry->m_hGpuMaterial = hGpuMaterial;
      scene.SetMaterial(pEntry->m_hSceneObject, !hGpuMaterial.IsInvalidated() ? hGpuMaterial.GetInternalID().m_InstanceIndex : xiiInvalidIndex);
    }

    if (pEntry == nullptr)
      continue;

    auto& source                = pSlot->m_MeshSources[pEntry->m_hSceneObject.GetInternalID().m_Data + 1U];
    source.m_uiMaterialIndex    = materialIndex;
    pEntry->m_uiSynchronization = synchronization;
    scene.SetLocalTransform(pEntry->m_hSceneObject, renderData.m_GlobalTransform.GetAsMat4());
    scene.SetFlags(pEntry->m_hSceneObject, GetSceneFlags(renderData, hMaterial));
    scene.SetPrimitiveRange(pEntry->m_hSceneObject, renderData.m_uiFirstPrimitive, renderData.m_uiPrimitiveCount);
    XII_SUCCEED_OR_RETURN(scene.SetLodSelection(pEntry->m_hSceneObject, forcedLod, renderData.m_fLodBias));
    xiiGeometryResidencyManager::RequestResidency(pEntry->m_hGeometry, renderData.m_uiLODIndex, uiFrameIndex);
    xiiGeometryResidencyManager::Touch(pEntry->m_hGeometry, uiFrameIndex);
  }

  for (auto it = pSlot->m_ExtractedMeshes.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiSynchronization == synchronization)
    {
      ++it;
      continue;
    }

    pSlot->m_MeshSources.Remove(it.Value().m_hSceneObject.GetInternalID().m_Data + 1U);
    scene.DestroyObject(it.Value().m_hSceneObject);
    xiiGeometryResidencyManager::ReleaseMeshGeometry(it.Value().m_hGeometry, uiFrameIndex);
    xiiMaterialManager::ReleaseMaterialResource(it.Value().m_hMaterial);
    it = pSlot->m_ExtractedMeshes.Remove(it);
  }

  scene.CommitFrame(uiFrameIndex);
  pSlot->m_uiLastSynchronizedFrame = uiFrameIndex;
  return XII_SUCCESS;
}
