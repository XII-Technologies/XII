/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/GpuInstanceData.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/ExtractedRenderData.h>
#include <GraphicsCore/Scene/Implementation/SceneExtractedSkinning.h>

namespace
{
  // Tokens never alias an earlier draw, including across world destruction/recreation. The
  // existing atomic facility supplies allocation; there is no public rendering object handle.
  xiiAtomicInteger64 s_NextGpuIdentity;
  struct PacketKey
  {
    xiiGameObjectHandle m_Object;
    xiiComponentHandle  m_Component;
    xiiUInt32           m_RenderId = 0;
    xiiUInt32           m_Section  = xiiInvalidIndex;
    xiiUInt32           m_Lod      = xiiInvalidIndex;
  };
  struct PacketHash
  {
    static xiiUInt32 Hash(const PacketKey& key)
    {
      auto hash = xiiHashingUtils::CombineHashValues32(xiiHashHelper<xiiGameObjectHandle>::Hash(key.m_Object), xiiHashHelper<xiiComponentHandle>::Hash(key.m_Component));
      hash      = xiiHashingUtils::CombineHashValues32(hash, key.m_RenderId);
      hash      = xiiHashingUtils::CombineHashValues32(hash, key.m_Section);
      return xiiHashingUtils::CombineHashValues32(hash, key.m_Lod);
    }
    static bool Equal(const PacketKey& a, const PacketKey& b)
    {
      return a.m_Object == b.m_Object && a.m_Component == b.m_Component && a.m_RenderId == b.m_RenderId && a.m_Section == b.m_Section && a.m_Lod == b.m_Lod;
    }
  };
  xiiBitflags<xiiGpuInstanceFlags> GetFlags(const xiiMeshRenderData& packet, const xiiMaterialResourceHandle& handle)
  {
    xiiBitflags<xiiGpuInstanceFlags> flags = xiiGpuInstanceFlags::Default;
    flags.Add(xiiGpuInstanceFlags::SensorVisible);
    flags.AddOrRemove(xiiGpuInstanceFlags::Static, packet.m_Flags.IsSet(xiiMeshRenderDataFlags::StaticObject));
    if (handle.IsValid())
    {
      xiiResourceLock<xiiMaterialResource> material(handle, xiiResourceAcquireMode::PointerOnly);
      if (material.GetAcquireResult() == xiiResourceAcquireResult::Final)
      {
        const auto& state = material->GetRuntimeState();
        flags.AddOrRemove(xiiGpuInstanceFlags::Transparent, state.IsTranslucent());
        flags.AddOrRemove(xiiGpuInstanceFlags::Occluder, !state.IsTranslucent());
        if (state.m_FeatureFlags.IsAnySet(xiiMaterialFeatureFlags::ReceivesLighting | xiiMaterialFeatureFlags::CastsShadows | xiiMaterialFeatureFlags::WritesVelocity))
        {
          flags.AddOrRemove(xiiGpuInstanceFlags::CastShadows, state.m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::CastsShadows));
          flags.AddOrRemove(xiiGpuInstanceFlags::ReceiveShadows, state.m_FeatureFlags.IsSet(xiiMaterialFeatureFlags::ReceivesLighting));
        }
      }
    }
    return flags;
  }
} // namespace

class xiiGpuInstanceDataState
{
public:
  explicit xiiGpuInstanceDataState(const xiiWorld& world) : m_World(world) {}
  struct Entry
  {
    PacketKey                 m_Key;
    xiiGpuInstanceSource      m_Source;
    xiiGeometryHandle         m_Geometry;
    xiiMaterialResourceHandle m_Material;
    xiiMaterialGpuHandle      m_GpuMaterial;
    xiiDynamicArray<xiiMat4>  m_CurrentPose, m_PreviousPose;
    xiiUInt64                 m_Seen            = 0;
    xiiUInt32                 m_Representative  = 0;
    xiiUInt32                 m_MaximumMeshlets = 0;
    bool                      m_Changed         = true;
  };
  const xiiWorld&                                m_World;
  xiiDynamicArray<Entry>                         m_Entries;
  xiiHashTable<PacketKey, xiiUInt32, PacketHash> m_Packets;
  xiiHashTable<xiiUInt32, xiiUInt32>             m_Identities;
  xiiUInt64                                      m_Extraction = 0, m_Frame = 0;

  bool IsLive(const xiiGpuInstanceSource& source) const
  {
    const xiiGameObject* object = nullptr;
    if (!m_World.TryGetObject(source.m_hOwnerObject, object) || !object->IsActive())
      return false;
    if (source.m_hOwnerComponent.IsInvalidated())
      return true;
    const xiiComponent* component = nullptr;
    return m_World.TryGetComponent(source.m_hOwnerComponent, component) && component->IsActiveAndInitialized() && component->GetOwner() == object;
  }
  void Release(Entry& entry) const
  {
    xiiGeometryResidencyManager::ReleaseMeshGeometry(entry.m_Geometry, m_Frame);
    xiiMaterialManager::ReleaseMaterialResource(entry.m_Material);
  }
};

xiiGpuInstanceData::xiiGpuInstanceData(const xiiWorld& world) : m_pState(XII_DEFAULT_NEW(xiiGpuInstanceDataState, world)) {}
xiiGpuInstanceData::~xiiGpuInstanceData()
{
  for (auto& entry : m_pState->m_Entries)
    m_pState->Release(entry);
}

xiiResult xiiGpuInstanceData::UpdateFromExtraction(const xiiExtractedRenderData& packets, xiiUInt64 frame)
{
  return UpdateFromExtraction(packets.GetAllRenderData(), frame);
}

xiiResult xiiGpuInstanceData::UpdateFromExtraction(xiiArrayPtr<xiiRenderData* const> packets, xiiUInt64 frame)
{
  if (!xiiGeometryResidencyManager::IsInitialized())
    return XII_FAILURE;
  auto& state = *m_pState;
  // Validate before changing membership or leases. Stale authoritative owners are omitted.
  for (const auto* data : packets)
    if (const auto* mesh = xiiDynamicCast<const xiiMeshRenderData*>(data); mesh && mesh->m_hMesh.IsValid())
    {
      if (!xiiMath::IsFinite(mesh->m_fLodBias) || (mesh->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) && mesh->m_uiLODIndex >= 8) || mesh->m_RangeMode > xiiMeshRenderData::RangeMode::WholeMesh)
        return XII_FAILURE;
      xiiResourceLock<xiiMeshResource> resource(mesh->m_hMesh, xiiResourceAcquireMode::PointerOnly);
      if (resource.GetAcquireResult() == xiiResourceAcquireResult::Final)
      {
        xiiExtractedMeshSkinning skinning;
        XII_SUCCEED_OR_RETURN(xiiPrepareExtractedMeshSkinning(*resource.GetPointer(), *mesh, skinning));
      }
    }
  ++state.m_Extraction;
  const bool nextFrame = frame != state.m_Frame;
  state.m_Frame        = frame;
  for (auto& entry : state.m_Entries)
  {
    entry.m_Changed = false;
    if (nextFrame)
      entry.m_PreviousPose = entry.m_CurrentPose;
  }
  xiiHashSet<PacketKey, PacketHash> groups;
  auto                              pack = [&](const xiiMeshRenderData& packet, const xiiMeshResource& mesh, const xiiExtractedMeshSkinning& skinning, const PacketKey& key, xiiUInt32 materialIndex, xiiUInt32 first, xiiUInt32 count, xiiUInt32& representative) -> xiiResult {
    const auto geometry = xiiGeometryResidencyManager::AcquireMeshGeometry(packet.m_hMesh);
    if (geometry.IsInvalidated())
      return XII_SUCCESS;
    const auto material = materialIndex < packet.m_hMaterials.GetCount() ? packet.m_hMaterials[materialIndex] : xiiMaterialResourceHandle{};
    xiiUInt32  index    = 0;
    const bool fresh    = !state.m_Packets.TryGetValue(key, index);
    if (fresh)
    {
      const auto identity = s_NextGpuIdentity.Increment();
      if (identity <= 0 || identity > xiiMath::MaxValue<xiiUInt32>())
      {
        xiiGeometryResidencyManager::ReleaseMeshGeometry(geometry, frame);
        return XII_FAILURE; // Exhaustion is fail-closed; an old GPU identity is never recycled.
      }
      index               = state.m_Entries.GetCount();
      auto& entry         = state.m_Entries.ExpandAndGetRef();
      entry.m_Key         = key;
      entry.m_Source      = {key.m_Object, key.m_Component, key.m_Lod, key.m_Section, materialIndex, key.m_RenderId};
      entry.m_Changed     = true;
      auto& record        = m_Instances.ExpandAndGetRef();
      record.m_uiObjectId = static_cast<xiiUInt32>(identity);
      state.m_Packets.Insert(key, index);
      state.m_Identities.Insert(record.m_uiObjectId, index);
    }
    auto&      entry     = state.m_Entries[index];
    auto&      record    = m_Instances[index];
    const auto before    = record;
    const bool resetPose = fresh || entry.m_Geometry != geometry;
    if (!fresh)
      xiiGeometryResidencyManager::ReleaseMeshGeometry(entry.m_Geometry, frame);
    entry.m_Geometry = geometry;
    if (fresh || entry.m_Material != material)
    {
      const auto gpuMaterial = xiiMaterialManager::AcquireMaterialResource(material);
      if (!fresh)
        xiiMaterialManager::ReleaseMaterialResource(entry.m_Material);
      entry.m_Material    = material;
      entry.m_GpuMaterial = gpuMaterial;
    }
    entry.m_Source.m_uiMaterialIndex = materialIndex;
    entry.m_Seen                     = state.m_Extraction;
    if (representative == 0)
      representative = record.m_uiObjectId;
    entry.m_Representative           = representative;
    record.m_uiGeometryIndex         = geometry.GetInternalID().m_InstanceIndex;
    record.m_uiMaterialIndex         = entry.m_GpuMaterial.IsInvalidated() ? xiiInvalidIndex : entry.m_GpuMaterial.GetInternalID().m_InstanceIndex;
    record.m_PreviousGlobalTransform = fresh ? packet.m_GlobalTransform.GetAsMat4() : nextFrame ? record.m_GlobalTransform :
                                                                                                                               record.m_PreviousGlobalTransform;
    record.m_GlobalTransform         = packet.m_GlobalTransform.GetAsMat4();
    auto normal                      = record.m_GlobalTransform.GetRotationalPart();
    if (normal.Invert().Succeeded())
      normal.Transpose();
    else
      normal.SetIdentity();
    const auto row0 = normal.GetRow(0), row1 = normal.GetRow(1), row2 = normal.GetRow(2);
    record.m_NormalTransformRow0 = xiiVec4(row0.x, row0.y, row0.z, 0);
    record.m_NormalTransformRow1 = xiiVec4(row1.x, row1.y, row1.z, 0);
    record.m_NormalTransformRow2 = xiiVec4(row2.x, row2.y, row2.z, 0);
    auto bounds                  = skinning.m_Bounds;
    bounds.Transform(record.m_GlobalTransform);
    record.m_BoundsCenterRadius = xiiVec4(bounds.m_vCenter.x, bounds.m_vCenter.y, bounds.m_vCenter.z, bounds.m_fSphereRadius);
    record.m_BoundsExtents      = xiiVec4(bounds.m_vBoxHalfExtents.x, bounds.m_vBoxHalfExtents.y, bounds.m_vBoxHalfExtents.z, 0);
    record.m_uiFlags            = GetFlags(packet, material).GetValue();
    record.m_uiUserData         = packet.m_uiUniqueID;
    record.m_uiFirstPrimitive   = first;
    record.m_uiPrimitiveCount   = count;
    record.m_uiForcedLod        = packet.m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) ? packet.m_uiLODIndex : xiiInvalidIndex;
    record.m_uiDrawLod          = key.m_Lod;
    record.m_fLodBias           = packet.m_fLodBias;
    if (record.m_uiObjectId == representative)
    {
      entry.m_CurrentPose = packet.m_SkinningMatrices;
      if (resetPose)
        entry.m_PreviousPose = entry.m_CurrentPose;
      record.m_uiBoneIndicesOffset    = skinning.m_uiBoneIndicesOffset;
      record.m_uiBoneWeightsOffset    = skinning.m_uiBoneWeightsOffset;
      record.m_uiSkinningVertexStride = skinning.m_uiVertexStride;
    }
    xiiUInt32 counts[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    XII_SUCCEED_OR_RETURN(xiiGeometryResidencyManager::GetPrimitiveRangeMeshletCounts(geometry, first, count, xiiMakeArrayPtr(counts)));
    entry.m_MaximumMeshlets = 0;
    for (xiiUInt32 lod = 0; lod < xiiGpuGeometryRecord::s_uiMaxLods; ++lod)
      if (key.m_Lod == xiiInvalidIndex || key.m_Lod == lod)
        entry.m_MaximumMeshlets = xiiMath::Max(entry.m_MaximumMeshlets, counts[lod]);
    xiiGeometryResidencyManager::RequestResidency(geometry, packet.m_uiLODIndex, frame);
    xiiGeometryResidencyManager::Touch(geometry, frame);
    entry.m_Changed |= xiiMemoryUtils::Compare(&before, &record, 1) != 0;
    return XII_SUCCESS;
  };
  for (const auto* data : packets)
  {
    const auto* packet = xiiDynamicCast<const xiiMeshRenderData*>(data);
    if (!packet || !packet->m_hMesh.IsValid())
      continue;
    if (!state.IsLive({packet->m_hOwnerObject, packet->m_hOwnerComponent}))
      continue;
    PacketKey key{packet->m_hOwnerObject, packet->m_hOwnerComponent, packet->m_uiUniqueID, packet->m_uiSectionIndex};
    if (packet->m_RangeMode == xiiMeshRenderData::RangeMode::WholeMesh)
      key.m_Section = xiiInvalidIndex;
    if (groups.Insert(key))
      continue;
    xiiResourceLock<xiiMeshResource> mesh(packet->m_hMesh, xiiResourceAcquireMode::PointerOnly);
    if (mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
      continue;
    xiiExtractedMeshSkinning skinning;
    XII_SUCCEED_OR_RETURN(xiiPrepareExtractedMeshSkinning(*mesh.GetPointer(), *packet, skinning));
    xiiUInt32 representative = 0;
    if (packet->m_RangeMode == xiiMeshRenderData::RangeMode::PrimitiveRange)
    {
      xiiUInt32  material = 0;
      const auto sections = mesh->GetLODs().IsEmpty() ? mesh->GetSections() : mesh->GetLODs()[xiiMath::Min(packet->m_uiLODIndex, mesh->GetLODs().GetCount() - 1)].m_Sections.GetArrayPtr();
      if (key.m_Section < sections.GetCount())
        material = sections[key.m_Section].m_uiMaterialIndex;
      XII_SUCCEED_OR_RETURN(pack(*packet, *mesh.GetPointer(), skinning, key, material, packet->m_uiFirstPrimitive, packet->m_uiPrimitiveCount, representative));
    }
    else
      for (xiiUInt32 lod = 0; lod < mesh->GetLODs().GetCount(); ++lod)
      {
        if (packet->m_Flags.IsSet(xiiMeshRenderDataFlags::ForceLOD) && lod != packet->m_uiLODIndex)
          continue;
        const auto sections = mesh->GetLODs()[lod].m_Sections.GetArrayPtr();
        for (xiiUInt32 section = 0; section < sections.GetCount(); ++section)
        {
          if (packet->m_RangeMode == xiiMeshRenderData::RangeMode::MeshSection && section != packet->m_uiSectionIndex)
            continue;
          if (sections[section].m_uiPrimitiveCount == 0)
            continue;
          key.m_Section = section;
          key.m_Lod     = lod;
          XII_SUCCEED_OR_RETURN(pack(*packet, *mesh.GetPointer(), skinning, key, sections[section].m_uiMaterialIndex, sections[section].m_uiFirstPrimitive, sections[section].m_uiPrimitiveCount, representative));
        }
      }
  }
  // Dense draw storage is compacted without creating/destroying an independently owned object.
  for (xiiUInt32 i = 0; i < state.m_Entries.GetCount();)
  {
    if (state.m_Entries[i].m_Seen == state.m_Extraction)
    {
      ++i;
      continue;
    }
    state.m_Packets.Remove(state.m_Entries[i].m_Key);
    state.m_Identities.Remove(m_Instances[i].m_uiObjectId);
    state.Release(state.m_Entries[i]);
    state.m_Entries.RemoveAtAndSwap(i);
    m_Instances.RemoveAtAndSwap(i);
    if (i < state.m_Entries.GetCount())
    {
      state.m_Packets[state.m_Entries[i].m_Key]       = i;
      state.m_Identities[m_Instances[i].m_uiObjectId] = i;
      state.m_Entries[i].m_Changed                    = true;
    }
  }
  m_SkinningMatrices.Clear();
  for (xiiUInt32 i = 0; i < state.m_Entries.GetCount(); ++i)
  {
    const auto& entry = state.m_Entries[i];
    if (entry.m_Representative != m_Instances[i].m_uiObjectId)
      continue;
    auto&      record         = m_Instances[i];
    const auto before         = record;
    record.m_uiSkinningOffset = m_SkinningMatrices.GetCount();
    record.m_uiSkinningCount  = entry.m_CurrentPose.GetCount();
    m_SkinningMatrices.PushBackRange(entry.m_CurrentPose);
    record.m_uiPreviousSkinningOffset = entry.m_CurrentPose == entry.m_PreviousPose ? record.m_uiSkinningOffset : m_SkinningMatrices.GetCount();
    record.m_uiPreviousSkinningCount  = entry.m_PreviousPose.GetCount();
    if (entry.m_CurrentPose != entry.m_PreviousPose)
      m_SkinningMatrices.PushBackRange(entry.m_PreviousPose);
    state.m_Entries[i].m_Changed |= xiiMemoryUtils::Compare(&before, &record, 1) != 0;
  }
  for (xiiUInt32 i = 0; i < state.m_Entries.GetCount(); ++i)
  {
    auto&      record         = m_Instances[i];
    const auto before         = record;
    xiiUInt32  representative = i;
    state.m_Identities.TryGetValue(state.m_Entries[i].m_Representative, representative);
    record.m_uiLodSelectionIndex      = representative == i ? xiiInvalidIndex : representative;
    const auto& pose                  = m_Instances[representative];
    record.m_uiSkinningOffset         = pose.m_uiSkinningOffset;
    record.m_uiSkinningCount          = pose.m_uiSkinningCount;
    record.m_uiPreviousSkinningOffset = pose.m_uiPreviousSkinningOffset;
    record.m_uiPreviousSkinningCount  = pose.m_uiPreviousSkinningCount;
    record.m_uiBoneIndicesOffset      = pose.m_uiBoneIndicesOffset;
    record.m_uiBoneWeightsOffset      = pose.m_uiBoneWeightsOffset;
    record.m_uiSkinningVertexStride   = pose.m_uiSkinningVertexStride;
    state.m_Entries[i].m_Changed |= xiiMemoryUtils::Compare(&before, &record, 1) != 0;
  }
  m_UploadRanges.Clear();
  for (xiiUInt32 i = 0; i < state.m_Entries.GetCount();)
  {
    if (!state.m_Entries[i].m_Changed)
    {
      ++i;
      continue;
    }
    const auto first = i++;
    while (i < state.m_Entries.GetCount() && state.m_Entries[i].m_Changed)
      ++i;
    m_UploadRanges.PushBack({first, i - first});
  }
  return XII_SUCCESS;
}

xiiUInt64 xiiGpuInstanceData::GetMaximumMeshletCount() const
{
  xiiUInt64 count = 0;
  for (const auto& entry : m_pState->m_Entries)
    count += entry.m_MaximumMeshlets;
  return count;
}

xiiResult xiiGpuInstanceData::Resolve(xiiUInt32 identity, xiiGpuInstanceSource& source) const
{
  source          = {};
  xiiUInt32 index = 0;
  if (!m_pState->m_Identities.TryGetValue(identity, index) || !m_pState->IsLive(m_pState->m_Entries[index].m_Source))
    return XII_FAILURE;
  source = m_pState->m_Entries[index].m_Source;
  return XII_SUCCESS;
}

void xiiGpuInstanceData::SelectInstances(xiiArrayPtr<xiiRenderData* const> packets, xiiDynamicArray<xiiUInt32>& indices) const
{
  indices.Clear();
  xiiHashSet<PacketKey, PacketHash> selected;
  for (const auto* data : packets)
    if (const auto* packet = xiiDynamicCast<const xiiMeshRenderData*>(data))
      selected.Insert(PacketKey{packet->m_hOwnerObject, packet->m_hOwnerComponent, packet->m_uiUniqueID, packet->m_RangeMode == xiiMeshRenderData::RangeMode::WholeMesh ? xiiInvalidIndex : packet->m_uiSectionIndex});
  for (xiiUInt32 i = 0; i < m_pState->m_Entries.GetCount(); ++i)
  {
    auto key         = m_pState->m_Entries[i].m_Key;
    key.m_Lod        = xiiInvalidIndex;
    const bool found = selected.Contains(key);
    key.m_Section    = xiiInvalidIndex;
    if (found || selected.Contains(key))
      indices.PushBack(i);
  }
}
