#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

class XII_RENDERERCORE_DLL xiiMeshResourceDescriptor
{
public:
  struct SubMesh
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiPrimitiveCount;
    xiiUInt32 m_uiFirstPrimitive;
    xiiUInt32 m_uiMaterialIndex;

    xiiBoundingBoxSphere m_Bounds;
  };

  struct Material
  {
    xiiString m_sPath;
  };

  xiiMeshResourceDescriptor();

  void Clear();

  xiiMeshBufferResourceDescriptor& MeshBufferDesc();

  const xiiMeshBufferResourceDescriptor& MeshBufferDesc() const;

  void UseExistingMeshBuffer(const xiiMeshBufferResourceHandle& hBuffer);

  void AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex);

  void SetMaterial(xiiUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void      Save(xiiStreamWriter& stream);
  xiiResult Save(const char* szFile);

  xiiResult Load(xiiStreamReader& stream);
  xiiResult Load(const char* szFile);

  const xiiMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  xiiArrayPtr<const Material> GetMaterials() const;

  xiiArrayPtr<const SubMesh> GetSubMeshes() const;

  /// \brief Merges all submeshes into just one.
  void CollapseSubMeshes();

  void                        ComputeBounds();
  const xiiBoundingBoxSphere& GetBounds() const;
  void                        SetBounds(const xiiBoundingBoxSphere& bounds) { m_Bounds = bounds; }

  struct BoneData
  {
    xiiMat4   m_GlobalInverseBindPoseMatrix;
    xiiUInt16 m_uiBoneIndex = xiiInvalidJointIndex;

    xiiResult Serialize(xiiStreamWriter& stream) const;
    xiiResult Deserialize(xiiStreamReader& stream);
  };

  xiiSkeletonResourceHandle               m_hDefaultSkeleton;
  xiiHashTable<xiiHashedString, BoneData> m_Bones;
  float                                   m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  xiiHybridArray<Material, 8>     m_Materials;
  xiiHybridArray<SubMesh, 8>      m_SubMeshes;
  xiiMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  xiiMeshBufferResourceHandle     m_hMeshBuffer;
  xiiBoundingBoxSphere            m_Bounds;
};
