#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <JoltPlugin/JoltPluginDLL.h>

using xiiJoltMeshResourceHandle = xiiTypedResourceHandle<class xiiJoltMeshResource>;
using xiiSurfaceResourceHandle  = xiiTypedResourceHandle<class xiiSurfaceResource>;
using xiiCpuMeshResourceHandle  = xiiTypedResourceHandle<class xiiCpuMeshResource>;

struct xiiMsgExtractGeometry;
class xiiJoltMaterial;

namespace JPH
{
  class MeshShape;
  class ConvexHullShape;
  class Shape;
} // namespace JPH

struct XII_JOLTPLUGIN_DLL xiiJoltMeshResourceDescriptor
{
  // empty, these types of resources must be loaded from file
};

class XII_JOLTPLUGIN_DLL xiiJoltMeshResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiJoltMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiJoltMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiJoltMeshResource, xiiJoltMeshResourceDescriptor);

public:
  xiiJoltMeshResource();
  ~xiiJoltMeshResource();

  /// \brief Returns the bounds of the collision mesh
  const xiiBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  /// \brief Returns the array of default surfaces to be used with this mesh.
  ///
  /// Note the array may contain less surfaces than the mesh does. It may also contain invalid surface handles.
  /// Use the default physics material as a fallback.
  const xiiDynamicArray<xiiSurfaceResourceHandle>& GetSurfaces() const { return m_Surfaces; }

  /// \brief Returns whether the mesh resource contains a triangle mesh. Triangle meshes and convex meshes are mutually exclusive.
  bool HasTriangleMesh() const { return m_pTriangleMeshInstance != nullptr || !m_TriangleMeshData.IsEmpty(); }

  /// \brief Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateTriangleMesh(xiiUInt64 uiUserData, const xiiDynamicArray<const xiiJoltMaterial*>& materials) const;

  /// \brief Returns the number of convex meshes. Triangle meshes and convex meshes are mutually exclusive.
  xiiUInt32 GetNumConvexParts() const { return !m_ConvexMeshInstances.IsEmpty() ? m_ConvexMeshInstances.GetCount() : m_ConvexMeshesData.GetCount(); }

  /// \brief Creates a new instance (shape) of the triangle mesh.
  JPH::Shape* InstantiateConvexPart(xiiUInt32 uiPartIdx, xiiUInt64 uiUserData, const xiiJoltMaterial* pMaterial, float fDensity) const;

  /// \brief Converts the geometry of the triangle or convex mesh to a CPU mesh resource
  xiiCpuMeshResourceHandle ConvertToCpuMesh() const;

  xiiUInt32 GetNumTriangles() const { return m_uiNumTriangles; }
  xiiUInt32 GetNumVertices() const { return m_uiNumVertices; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiBoundingBoxSphere                      m_Bounds;
  xiiDynamicArray<xiiSurfaceResourceHandle> m_Surfaces;
  mutable xiiHybridArray<xiiDataBuffer*, 1> m_ConvexMeshesData;
  mutable xiiDataBuffer                     m_TriangleMeshData;
  mutable JPH::Shape*                       m_pTriangleMeshInstance = nullptr;
  mutable xiiHybridArray<JPH::Shape*, 1>    m_ConvexMeshInstances;

  xiiUInt32 m_uiNumVertices  = 0;
  xiiUInt32 m_uiNumTriangles = 0;
};
