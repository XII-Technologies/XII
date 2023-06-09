#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RecastPlugin/RecastPluginDLL.h>

struct rcPolyMesh;
class dtNavMesh;

using xiiRecastNavMeshResourceHandle = xiiTypedResourceHandle<class xiiRecastNavMeshResource>;

struct XII_RECASTPLUGIN_DLL xiiRecastNavMeshResourceDescriptor
{
  xiiRecastNavMeshResourceDescriptor();
  xiiRecastNavMeshResourceDescriptor(const xiiRecastNavMeshResourceDescriptor& rhs) = delete;
  xiiRecastNavMeshResourceDescriptor(xiiRecastNavMeshResourceDescriptor&& rhs);
  ~xiiRecastNavMeshResourceDescriptor();
  void operator=(xiiRecastNavMeshResourceDescriptor&& rhs);
  void operator=(const xiiRecastNavMeshResourceDescriptor& rhs) = delete;

  /// \brief Data that was created by dtCreateNavMeshData() and will be used for dtNavMesh::init()
  xiiDataBuffer m_DetourNavmeshData;

  /// \brief Optional, if available the navmesh can be visualized at runtime
  rcPolyMesh* m_pNavMeshPolygons = nullptr;

  void Clear();

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);
};

class XII_RECASTPLUGIN_DLL xiiRecastNavMeshResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRecastNavMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRecastNavMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiRecastNavMeshResource, xiiRecastNavMeshResourceDescriptor);

public:
  xiiRecastNavMeshResource();
  ~xiiRecastNavMeshResource();

  const dtNavMesh*  GetNavMesh() const { return m_pNavMesh; }
  const rcPolyMesh* GetNavMeshPolygons() const { return m_pNavMeshPolygons; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiDataBuffer m_DetourNavmeshData;
  dtNavMesh*    m_pNavMesh         = nullptr;
  rcPolyMesh*   m_pNavMeshPolygons = nullptr;
};
