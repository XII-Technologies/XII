/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Meshes/MeshSkinning.h>
#include <GraphicsCore/Scene/SceneDatabase.h>

xiiResult xiiSceneDatabase::SetSkinningPalette(xiiSceneObjectHandle hObject, xiiArrayPtr<const xiiMat4> palette,
                                               xiiUInt32 uiVertexStride, xiiUInt32 uiBoneIndicesOffset, xiiUInt32 uiBoneWeightsOffset, bool bResetHistory)
{
  if (!IsAlive(hObject) || (!palette.IsEmpty() && (uiVertexStride < 8U || (uiVertexStride & 3U) != 0U || uiBoneIndicesOffset > uiVertexStride - 8U || uiBoneWeightsOffset > uiVertexStride - 4U || (uiBoneIndicesOffset & 1U) != 0U)))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(xiiMeshSkinning::ValidatePalette(palette));
  SkinningPose* pose = nullptr;
  if (!m_SkinningPoses.TryGetValue(hObject.GetInternalID().m_Data, pose))
  {
    if (palette.IsEmpty())
      return XII_SUCCESS;
    pose          = &m_SkinningPoses[hObject.GetInternalID().m_Data];
    bResetHistory = true;
  }
  pose->m_Current = palette;
  if (bResetHistory)
    pose->m_Previous = palette;
  if (!palette.IsEmpty())
  {
    pose->m_uiVertexStride      = uiVertexStride;
    pose->m_uiBoneIndicesOffset = uiBoneIndicesOffset;
    pose->m_uiBoneWeightsOffset = uiBoneWeightsOffset;
  }
  m_GpuDirty[hObject.GetInternalID().m_InstanceIndex] = 1;
  ++m_uiRevision;
  return XII_SUCCESS;
}

void xiiSceneDatabase::BuildSkinningSnapshot(xiiUInt32 uiObject, xiiGpuSceneInstance& instance)
{
  const auto    representative = m_LodGroups[uiObject];
  const auto    handle         = IsAlive(representative) && m_GeometryIndices[representative.GetInternalID().m_InstanceIndex] == m_GeometryIndices[uiObject] ? representative : m_ObjectHandles[uiObject];
  SkinningPose* pose           = nullptr;
  // Newly allocated POD records have no history to inspect and are already dirty.
  if (m_GpuDirty[uiObject] == 0 && (instance.m_uiSkinningCount != 0 || instance.m_uiPreviousSkinningCount != 0))
    m_GpuDirty[uiObject] = 1;
  // Reset the entire tail, including instances that were previously posed or reused.
  instance.m_uiSkinningOffset = instance.m_uiSkinningCount = 0;
  instance.m_uiPreviousSkinningOffset = instance.m_uiPreviousSkinningCount = 0;
  instance.m_uiBoneIndicesOffset = instance.m_uiBoneWeightsOffset = 0;
  instance.m_uiSkinningVertexStride = instance.m_uiSkinningPadding = 0;
  if (!m_SkinningPoses.TryGetValue(handle.GetInternalID().m_Data, pose))
    return;
  instance.m_uiSkinningOffset         = pose->m_SnapshotRange.x;
  instance.m_uiSkinningCount          = pose->m_SnapshotRange.y;
  instance.m_uiPreviousSkinningOffset = pose->m_SnapshotRange.z;
  instance.m_uiPreviousSkinningCount  = pose->m_SnapshotRange.w;
  instance.m_uiBoneIndicesOffset      = pose->m_uiBoneIndicesOffset;
  instance.m_uiBoneWeightsOffset      = pose->m_uiBoneWeightsOffset;
  instance.m_uiSkinningVertexStride   = pose->m_uiVertexStride;
  // Offsets can move when an unrelated posed instance is removed. The existing upload mirror
  // still coalesces byte-identical records; these flags also keep public dirty ranges correct.
  m_GpuDirty[uiObject] = 1;
}

void xiiSceneDatabase::PrepareSkinningSnapshots()
{
  m_GpuSkinningMatrices.Clear();
  for (auto& entry : m_SkinningPoses)
  {
    auto& pose  = entry.Value();
    auto& range = pose.m_SnapshotRange;
    range.x     = m_GpuSkinningMatrices.GetCount();
    range.y     = pose.m_Current.GetCount();
    m_GpuSkinningMatrices.PushBackRange(pose.m_Current);
    const bool identical = pose.m_Current.GetArrayPtr() == pose.m_Previous.GetArrayPtr();
    range.z              = identical ? range.x : m_GpuSkinningMatrices.GetCount();
    range.w              = pose.m_Previous.GetCount();
    if (!identical)
      m_GpuSkinningMatrices.PushBackRange(pose.m_Previous);
  }
}

void xiiSceneDatabase::AdvanceSkinningPoses()
{
  for (auto it = m_SkinningPoses.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_Current.IsEmpty())
      it = m_SkinningPoses.Remove(it);
    else
    {
      it.Value().m_Previous = it.Value().m_Current;
      ++it;
    }
  }
}
