/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Scene/Implementation/SceneDatabaseManagerState.h>
#include <GraphicsCore/Scene/Implementation/SceneExtractedSkinning.h>

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

  xiiUInt32 SelectMaterialIndex(const xiiMeshRenderData& renderData)
  {
    xiiUInt32 materialIndex = 0U;
    if (renderData.m_uiSectionIndex != xiiInvalidIndex && renderData.m_hMesh.IsValid())
    {
      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh->GetLoadingState() == xiiResourceState::Loaded)
      {
        const auto                              lods     = mesh->GetLODs();
        const xiiArrayPtr<const xiiMeshSection> sections = lods.IsEmpty() ? mesh->GetSections() : lods[xiiMath::Min(renderData.m_uiLODIndex, lods.GetCount() - 1)].m_Sections;
        if (renderData.m_uiSectionIndex < sections.GetCount())
          materialIndex = sections[renderData.m_uiSectionIndex].m_uiMaterialIndex;
      }
    }

    return materialIndex;
  }
  xiiResult SynchronizeMesh(xiiSceneDatabaseManagerState::Slot& slot, const xiiMeshRenderData& renderData, const xiiSceneDatabaseManagerState::MeshKey& key, xiiUInt32 materialIndex, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiPrimitiveCount, const xiiExtractedMeshSkinning& skinning, xiiUInt64 uiFrameIndex, xiiUInt64 synchronization, xiiSceneObjectHandle& inout_hLodRepresentative)
  {
    auto&           scene                = *slot.m_pDatabase;
    bool            resetSkinningHistory = true;
    const xiiUInt32 forcedLod            = renderData.m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) ? renderData.m_uiLODIndex : xiiInvalidIndex;
    const auto      hMaterial            = materialIndex < renderData.m_hMaterials.GetCount() ? renderData.m_hMaterials[materialIndex] : xiiMaterialResourceHandle{};

    xiiSceneDatabaseManagerState::ExtractedMeshEntry* pEntry = nullptr;
    slot.m_ExtractedMeshes.TryGetValue(key, pEntry);
    // A consumer may clear its database between frames. Retire the extraction resources as well
    // before recreating entries; stale scene handles must never update a newly reused slot.
    if (pEntry != nullptr && !scene.IsAlive(pEntry->m_hSceneObject))
    {
      xiiGeometryResidencyManager::ReleaseMeshGeometry(pEntry->m_hGeometry, uiFrameIndex);
      xiiMaterialManager::ReleaseMaterialResource(pEntry->m_hMaterial);
      slot.m_MeshSources.Remove(pEntry->m_hSceneObject.GetInternalID().m_Data + 1U);
      slot.m_ExtractedMeshes.Remove(key);
      pEntry = nullptr;
    }
    if (pEntry == nullptr)
    {
      const xiiGeometryHandle hGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(renderData.m_hMesh);
      if (hGeometry.IsInvalidated())
        return XII_SUCCESS;
      const xiiMaterialGpuHandle hGpuMaterial = xiiMaterialManager::AcquireMaterialResource(hMaterial);

      xiiSceneObjectDesc desc;
      desc.m_LocalTransform   = renderData.m_GlobalTransform.GetAsMat4();
      desc.m_Flags            = GetSceneFlags(renderData, hMaterial);
      desc.m_uiGeometryIndex  = hGeometry.GetInternalID().m_InstanceIndex;
      desc.m_uiMaterialIndex  = !hGpuMaterial.IsInvalidated() ? hGpuMaterial.GetInternalID().m_InstanceIndex : xiiInvalidIndex;
      desc.m_uiUserData       = renderData.m_uiUniqueID;
      desc.m_uiFirstPrimitive = uiFirstPrimitive;
      desc.m_uiPrimitiveCount = uiPrimitiveCount;
      desc.m_uiForcedLod      = forcedLod;
      desc.m_uiDrawLod        = key.m_uiLodIndex;
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
        return XII_SUCCESS;
      }

      slot.m_ExtractedMeshes.Insert(key, entry);
      slot.m_ExtractedMeshes.TryGetValue(key, pEntry);
      xiiSceneMeshSource source;
      source.m_hSceneObject    = entry.m_hSceneObject;
      source.m_hOwnerObject    = key.m_hObject;
      source.m_hOwnerComponent = key.m_hComponent;
      source.m_uiSectionIndex  = key.m_uiSectionIndex;
      source.m_uiLodIndex      = key.m_uiLodIndex;
      source.m_uiMaterialIndex = materialIndex;
      source.m_uiRenderDataId  = key.m_uiRenderDataId;
      slot.m_MeshSources.Insert(entry.m_hSceneObject.GetInternalID().m_Data + 1U, source);
    }
    else
    {
      // Resource handles survive content reloads. Acquire the current content revision before
      // releasing the previous one, including when two views migrate during different frames.
      const xiiGeometryHandle hGeometry = xiiGeometryResidencyManager::AcquireMeshGeometry(renderData.m_hMesh);
      if (hGeometry.IsInvalidated())
        return XII_SUCCESS;

      xiiGeometryResidencyManager::ReleaseMeshGeometry(pEntry->m_hGeometry, uiFrameIndex);
      const bool changed   = pEntry->m_hGeometry != hGeometry;
      resetSkinningHistory = changed;
      pEntry->m_hMesh      = renderData.m_hMesh;
      pEntry->m_hGeometry  = hGeometry;
      if (changed)
      {
        pEntry->m_bMeshletCountsValid = false;
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
      return XII_SUCCESS;

    if (!pEntry->m_bMeshletCountsValid || pEntry->m_uiFirstPrimitive != uiFirstPrimitive || pEntry->m_uiPrimitiveCount != uiPrimitiveCount)
    {
      pEntry->m_bMeshletCountsValid = false;
      XII_SUCCEED_OR_RETURN(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(pEntry->m_hGeometry, uiFirstPrimitive, uiPrimitiveCount, xiiMakeArrayPtr(pEntry->m_uiMeshletCounts)));
      pEntry->m_uiFirstPrimitive    = uiFirstPrimitive;
      pEntry->m_uiPrimitiveCount    = uiPrimitiveCount;
      pEntry->m_bMeshletCountsValid = true;
    }

    auto& source                = slot.m_MeshSources[pEntry->m_hSceneObject.GetInternalID().m_Data + 1U];
    source.m_uiMaterialIndex    = materialIndex;
    pEntry->m_uiSynchronization = synchronization;
    scene.SetLocalTransform(pEntry->m_hSceneObject, renderData.m_GlobalTransform.GetAsMat4());
    scene.SetFlags(pEntry->m_hSceneObject, GetSceneFlags(renderData, hMaterial));
    scene.SetLocalBounds(pEntry->m_hSceneObject, skinning.m_Bounds);
    // Sections/LODs share their representative's pose, just as they share LOD history.
    if (inout_hLodRepresentative.IsInvalidated())
      XII_SUCCEED_OR_RETURN(scene.SetSkinningPalette(pEntry->m_hSceneObject, renderData.m_SkinningMatrices, skinning.m_uiVertexStride, skinning.m_uiBoneIndicesOffset, skinning.m_uiBoneWeightsOffset, resetSkinningHistory));
    else
      // Reload can make a surviving representative a member of a new section group.
      // Its private history is obsolete once it follows the new representative.
      XII_SUCCEED_OR_RETURN(scene.SetSkinningPalette(pEntry->m_hSceneObject, {}, 0, 0, 0, true));
    scene.SetPrimitiveRange(pEntry->m_hSceneObject, uiFirstPrimitive, uiPrimitiveCount);
    XII_SUCCEED_OR_RETURN(scene.SetLodSelection(pEntry->m_hSceneObject, forcedLod, renderData.m_fLodBias, key.m_uiLodIndex));
    XII_SUCCEED_OR_RETURN(scene.SetLodGroup(pEntry->m_hSceneObject, inout_hLodRepresentative));
    if (inout_hLodRepresentative.IsInvalidated())
      inout_hLodRepresentative = pEntry->m_hSceneObject;
    xiiGeometryResidencyManager::RequestResidency(pEntry->m_hGeometry, renderData.m_uiLODIndex, uiFrameIndex);
    xiiGeometryResidencyManager::Touch(pEntry->m_hGeometry, uiFrameIndex);
    return XII_SUCCESS;
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
    if (mesh && mesh->m_hMesh.IsValid() && (!xiiMath::IsFinite(mesh->m_fLodBias) || (mesh->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) && mesh->m_uiLODIndex >= 8U) || mesh->m_RangeMode > xiiMeshRenderData::RangeMode::WholeMesh))
      return XII_FAILURE;
  }

  xiiSceneDatabase& scene = *pSlot->m_pDatabase;
  // Several filtered snapshots can be synchronized during one CPU frame. Membership belongs
  // to this call, while resource retirement still uses the GPU frame timeline.
  const auto synchronization = ++pSlot->m_uiSynchronization;
  pSlot->m_SynchronizedMeshGroups.Clear();
  const xiiRenderDataBatch::Iterator<xiiMeshRenderData> meshes(packets, 0U, xiiMath::MaxValue<xiiUInt32>());
  for (auto it = meshes; it.IsValid(); ++it)
  {
    const xiiMeshRenderData& renderData = *it;
    if (!renderData.m_hMesh.IsValid())
      continue;

    xiiSceneDatabaseManagerState::MeshKey key{renderData.m_hOwnerObject, renderData.m_hOwnerComponent, renderData.m_uiUniqueID, renderData.m_uiSectionIndex};
    if (renderData.m_RangeMode == xiiMeshRenderData::RangeMode::PrimitiveRange)
    {
      xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
        continue;
      xiiExtractedMeshSkinning skinning;
      XII_SUCCEED_OR_RETURN(xiiPrepareExtractedMeshSkinning(*mesh.GetPointer(), renderData, skinning));
      xiiSceneObjectHandle representative;
      XII_SUCCEED_OR_RETURN(SynchronizeMesh(*pSlot, renderData, key, SelectMaterialIndex(renderData), renderData.m_uiFirstPrimitive, renderData.m_uiPrimitiveCount, skinning, uiFrameIndex, synchronization, representative));
      continue;
    }

    // Whole-mesh extraction still emits CPU draw packets per section. Expand each component
    // group once, and keep the scratch capacity across frames. Explicit sections expand only
    // that section ordinal where it exists; no correspondence between material slots is assumed.
    if (renderData.m_RangeMode == xiiMeshRenderData::RangeMode::WholeMesh)
      key.m_uiSectionIndex = xiiInvalidIndex;
    if (pSlot->m_SynchronizedMeshGroups.Insert(key))
      continue;
    xiiResourceLock<xiiMeshResource> mesh(renderData.m_hMesh, xiiResourceAcquireMode::PointerOnly);
    if (mesh->GetLoadingState() != xiiResourceState::Loaded)
      continue;
    // Pose bounds and stream validation are shared by every candidate in this group.
    xiiExtractedMeshSkinning skinning;
    XII_SUCCEED_OR_RETURN(xiiPrepareExtractedMeshSkinning(*mesh.GetPointer(), renderData, skinning));
    const auto           lods = mesh->GetLODs();
    xiiSceneObjectHandle representative;
    for (xiiUInt32 lodIndex = 0; lodIndex < lods.GetCount(); ++lodIndex)
    {
      if (renderData.m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) && lodIndex != renderData.m_uiLODIndex)
        continue;
      const auto sections = lods[lodIndex].m_Sections.GetArrayPtr();
      for (xiiUInt32 sectionIndex = 0; sectionIndex < sections.GetCount(); ++sectionIndex)
      {
        if (renderData.m_RangeMode == xiiMeshRenderData::RangeMode::MeshSection && sectionIndex != renderData.m_uiSectionIndex)
          continue;
        const auto& section = sections[sectionIndex];
        if (section.m_uiPrimitiveCount == 0)
          continue;
        key.m_uiSectionIndex = sectionIndex;
        key.m_uiLodIndex     = lodIndex;
        XII_SUCCEED_OR_RETURN(SynchronizeMesh(*pSlot, renderData, key, section.m_uiMaterialIndex, section.m_uiFirstPrimitive, section.m_uiPrimitiveCount, skinning, uiFrameIndex, synchronization, representative));
      }
    }
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
