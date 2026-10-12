/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Scene/SceneDatabaseManager.h>

class xiiSceneDatabaseManagerState
{
public:
  struct MeshKey
  {
    xiiGameObjectHandle m_hObject;
    xiiComponentHandle  m_hComponent;
    xiiUInt32           m_uiRenderDataId = 0U;
    xiiUInt32           m_uiSectionIndex = xiiInvalidIndex;
    xiiUInt32           m_uiLodIndex     = xiiInvalidIndex;
  };

  struct MeshKeyHash
  {
    static xiiUInt32 Hash(const MeshKey& key)
    {
      auto hash = xiiHashingUtils::CombineHashValues32(xiiHashHelper<xiiGameObjectHandle>::Hash(key.m_hObject), xiiHashHelper<xiiComponentHandle>::Hash(key.m_hComponent));
      hash      = xiiHashingUtils::CombineHashValues32(hash, key.m_uiRenderDataId);
      hash      = xiiHashingUtils::CombineHashValues32(hash, key.m_uiSectionIndex);
      return xiiHashingUtils::CombineHashValues32(hash, key.m_uiLodIndex);
    }

    static bool Equal(const MeshKey& a, const MeshKey& b)
    {
      return a.m_hObject == b.m_hObject && a.m_hComponent == b.m_hComponent && a.m_uiRenderDataId == b.m_uiRenderDataId && a.m_uiSectionIndex == b.m_uiSectionIndex && a.m_uiLodIndex == b.m_uiLodIndex;
    }
  };

  struct ExtractedMeshEntry
  {
    xiiSceneObjectHandle      m_hSceneObject;
    xiiMeshResourceHandle     m_hMesh;
    xiiGeometryHandle         m_hGeometry;
    xiiMaterialResourceHandle m_hMaterial;
    xiiMaterialGpuHandle      m_hGpuMaterial;
    xiiUInt64                 m_uiSynchronization = 0U;
  };

  struct Slot
  {
    xiiUniquePtr<xiiSceneDatabase>                         m_pDatabase;
    xiiHashTable<MeshKey, ExtractedMeshEntry, MeshKeyHash> m_ExtractedMeshes;
    xiiHashTable<xiiUInt32, xiiSceneMeshSource>            m_MeshSources;
    xiiHashSet<MeshKey, MeshKeyHash>                       m_SynchronizedMeshGroups; ///< Reusable scratch, cleared for every synchronization.
    xiiUInt64                                              m_uiLastSynchronizedFrame = 0U;
    xiiUInt64                                              m_uiSynchronization       = 0U;
  };

  xiiIdTable<xiiSceneDatabaseContextId, Slot> m_Contexts;
  bool                                        m_bEngineStarted = false;
};
