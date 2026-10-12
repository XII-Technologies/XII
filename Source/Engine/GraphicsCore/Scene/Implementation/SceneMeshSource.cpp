/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Scene/Implementation/SceneDatabaseManagerState.h>

xiiResult xiiSceneDatabaseManager::ResolveExtractedMesh(xiiSceneDatabaseContextHandle context, xiiUInt32 uiGpuObjectId, xiiSceneMeshSource& out_source)
{
  out_source = {};
  if (!s_pState || !s_pState->m_bEngineStarted)
    return XII_FAILURE;
  xiiSceneDatabaseManagerState::Slot* slot = nullptr;
  if (!s_pState->m_Contexts.TryGetValue(context.GetInternalID(), slot) || slot->m_pDatabase->ResolveGpuObjectId(uiGpuObjectId).IsInvalidated())
    return XII_FAILURE;
  return slot->m_MeshSources.TryGetValue(uiGpuObjectId, out_source) ? XII_SUCCESS : XII_FAILURE;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Scene_Implementation_SceneMeshSource);
