#pragma once

#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Meshes/MeshResourceDescriptor.h>

using xiiMaterialResourceHandle = xiiTypedResourceHandle<class xiiMaterialResource>;

class XII_GRAPHICSCORE_DLL xiiMeshResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshResource, xiiMeshResourceDescriptor);

public:
  xiiMeshResource();

  /// \brief Returns the array of sub-meshes in this mesh.
  xiiArrayPtr<const xiiMeshResourceDescriptor::SubMesh> GetSubMeshes() const { return m_SubMeshes; }

  /// \brief Returns the mesh buffer that is used by this resource.
  const xiiMeshBufferResourceHandle& GetMeshBuffer() const { return m_hMeshBuffer; }

  /// \brief Returns the default materials for this mesh.
  xiiArrayPtr<const xiiMaterialResourceHandle> GetMaterials() const { return m_Materials; }

  /// \brief Returns the bounds of this mesh.
  const xiiBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  // TODO: clean up
  xiiSkeletonResourceHandle                                          m_hDefaultSkeleton;
  xiiHashTable<xiiHashedString, xiiMeshResourceDescriptor::BoneData> m_Bones;
  float                                                              m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiDynamicArray<xiiMeshResourceDescriptor::SubMesh> m_SubMeshes;
  xiiMeshBufferResourceHandle                         m_hMeshBuffer;
  xiiDynamicArray<xiiMaterialResourceHandle>          m_Materials;

  xiiBoundingBoxSphere m_Bounds;

  static xiiUInt32 s_uiMeshBufferNameSuffix;
};

using xiiMeshResourceHandle = xiiTypedResourceHandle<class xiiMeshResource>;
