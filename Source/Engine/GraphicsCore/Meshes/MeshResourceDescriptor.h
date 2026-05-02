#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

/// Descriptor for creating mesh resources.
///
/// Defines sub-meshes with materials, references or creates mesh buffer data,
/// and stores bounding information. Used both for procedural mesh generation and
/// loading from files.
class XII_GRAPHICSCORE_DLL xiiMeshResourceDescriptor
{
public:
  /// Describes a sub-mesh within the mesh.
  ///
  /// Each sub-mesh references a range of primitives and a material slot.
  struct SubMesh
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32 m_uiPrimitiveCount = 0; ///< Number of primitives in this sub-mesh.
    xiiUInt32 m_uiFirstPrimitive = 0; ///< Index of the first primitive.
    xiiUInt32 m_uiMaterialIndex  = 0; ///< Index into the material array.

    xiiBoundingBoxSphere m_Bounds; ///< Bounding volume of this sub-mesh.
  };

  /// Material slot information.
  struct Material
  {
    xiiString m_sPath; ///< Path or GUID of the material resource.
  };

  xiiMeshResourceDescriptor();

  void Clear();

  /// Returns the mesh buffer descriptor for creating a new mesh buffer.
  ///
  /// Use this when building mesh data procedurally. Mutually exclusive with UseExistingMeshBuffer.
  xiiMeshBufferResourceDescriptor& MeshBufferDesc();

  const xiiMeshBufferResourceDescriptor& MeshBufferDesc() const;

  /// Uses an existing mesh buffer instead of creating a new one.
  ///
  /// Mutually exclusive with modifying MeshBufferDesc.
  void UseExistingMeshBuffer(const xiiMeshBufferResourceHandle& hBuffer);

  /// Adds a sub-mesh to the descriptor.
  void AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex);

  /// Sets the material path for a material slot.
  void SetMaterial(xiiUInt32 uiMaterialIndex, xiiStringView sPathToMaterial);

  void      Save(xiiStreamWriter& inout_stream) const;
  xiiResult Save(const char* szFile) const;

  xiiResult Load(xiiStreamReader& inout_stream);
  xiiResult Load(const char* szFile);

  const xiiMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  xiiArrayPtr<const Material> GetMaterials() const;

  xiiArrayPtr<const SubMesh> GetSubMeshes() const;

  /// Merges all submeshes into just one.
  void CollapseSubMeshes();

  void                        ComputeBounds();
  const xiiBoundingBoxSphere& GetBounds() const;
  void                        SetBounds(const xiiBoundingBoxSphere& bounds) { m_Bounds = bounds; }

  /// Data for a bone used in skinned meshes.
  struct BoneData
  {
    xiiMat4   m_GlobalInverseRestPoseMatrix;        ///< Transform from mesh space to bone space.
    xiiUInt16 m_uiBoneIndex = xiiInvalidJointIndex; ///< Index into the skeleton.

    xiiResult Serialize(xiiStreamWriter& inout_stream) const;
    xiiResult Deserialize(xiiStreamReader& inout_stream);
  };

  xiiSkeletonResourceHandle               m_hDefaultSkeleton; ///< Default skeleton for skinned meshes.
  xiiHashTable<xiiHashedString, BoneData> m_Bones;            ///< Bone data indexed by bone name.

  /// Maximum distance between any vertex and its influencing bones.
  ///
  /// Can be used for adjusting the bounding box of an animated pose.
  float m_fMaxBoneVertexOffset = 0.0f;

  //
  // Meshlet support
  //
  struct Meshlet
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt32         m_uiFirstPrimitive = 0;          // index into global primitive list (triangle index)
    xiiUInt16         m_uiPrimitiveCount = 0;          // number of triangles
    xiiUInt16         m_uiVertexCount    = 0;          // unique vertices in this meshlet
    xiiBoundingSphere m_Bounds;                        // tight bounding sphere for culling
    xiiVec3           m_ConeAxis;                      // dominant normal direction for cone culling
    float             m_fConeAngleCos          = 1.0f; // cosine of cone opening angle
    xiiUInt32         m_uiMaterialIndex        = 0;    // material slot for this meshlet
    xiiUInt32         m_uiVertexRemapOffset    = 0;    // offset into m_MeshletVertexRemap
    xiiUInt32         m_uiPrimitiveIndexOffset = 0;    // offset into m_MeshletPrimitiveIndices
  };

  /// Meshlet arrays (descriptor-level, used for saving/loading and building resources)
  xiiHybridArray<Meshlet, 64> m_Meshlets;
  xiiDynamicArray<xiiUInt32>  m_MeshletVertexRemap;      // flattened remap tables (per-meshlet)
  xiiDynamicArray<xiiUInt32>  m_MeshletPrimitiveIndices; // flattened triangle indices (3 * triangleCount per meshlet)

private:
  xiiHybridArray<Material, 8>     m_Materials;
  xiiHybridArray<SubMesh, 8>      m_SubMeshes;
  xiiMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  xiiMeshBufferResourceHandle     m_hMeshBuffer;
  xiiBoundingBoxSphere            m_Bounds;
};
