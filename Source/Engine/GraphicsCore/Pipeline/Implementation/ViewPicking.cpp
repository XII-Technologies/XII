/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/World.h>
#include <GraphicsCore/Pipeline/View.h>

xiiResult xiiView::ResolveRenderedMesh(xiiUInt32 uiGpuObjectId, xiiSceneMeshSource& out_source) const
{
  out_source = {};
  xiiSceneMeshSource source;
  if (!m_pWorld || xiiSceneDatabaseManager::ResolveExtractedMesh(m_GpuSceneContext.GetHandle(), uiGpuObjectId, source).Failed())
    return XII_FAILURE;
  const xiiGameObject* object    = nullptr;
  const xiiComponent*  component = nullptr;
  if (!m_pWorld->TryGetObject(source.m_hOwnerObject, object) || !object->IsActive() ||
      !m_pWorld->TryGetComponent(source.m_hOwnerComponent, component) || !component->IsActive() || component->GetOwner() != object)
    return XII_FAILURE;
  const auto& tags = object->GetTags();
  if (tags.IsAnySet(m_ExcludeTags) || (!m_IncludeTags.IsEmpty() && !tags.IsAnySet(m_IncludeTags)))
    return XII_FAILURE;
  out_source = source;
  return XII_SUCCESS;
}
