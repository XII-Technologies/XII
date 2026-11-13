/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Threading/TaskSystem.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <JoltPlugin/Resources/Implementation/JoltCollisionMeshData.h>

/// A worker owns only CPU asset inputs and immutable output. The world joins outstanding jobs
/// before destroying its cache, and never shares object or component pointers with a worker.
class xiiJoltSceneCollisionCook final : public xiiTask
{
public:
  xiiJoltSceneCollisionCook();

  xiiMeshResourceHandle           m_hMesh;
  xiiUInt32                       m_uiSection = xiiInvalidIndex;
  xiiUniquePtr<xiiSmcDescription> m_pGeometry;

  xiiSharedPtr<xiiJoltCollisionMeshData> m_pResult;
  xiiDynamicArray<xiiUInt32>             m_MaterialSlots;
  xiiDynamicArray<xiiString>             m_Materials;
  xiiTaskGroupID                         m_Group;

private:
  void      Execute() override;
  xiiResult CookMesh();
};
