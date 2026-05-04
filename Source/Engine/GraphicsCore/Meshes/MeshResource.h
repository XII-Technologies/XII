/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

/// \brief Mesh resource usage flags, used to specify intended usage patterns and GPU feature support for a mesh resource.
struct XII_GRAPHICSCORE_DLL xiiMeshResourceUsageFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None            = 0U,         ///< No special usage, default for most meshes.
    StaticGeometry  = XII_BIT(0), ///< Mesh geometry is static and will not change at runtime.
    DynamicGeometry = XII_BIT(1), ///< Mesh geometry can change at runtime.
    Skinned         = XII_BIT(2), ///< Mesh has skinning information for animation.
    MorphTargets    = XII_BIT(3), ///< Mesh has morph targets for shape animation.
    Instancing      = XII_BIT(4), ///< Mesh can be instanced for efficient rendering.
    MeshShaderReady = XII_BIT(5), ///< Mesh is optimized for mesh shaders.
    RayTracingReady = XII_BIT(6), ///< Mesh is ready for ray tracing.
    Streaming       = XII_BIT(7), ///< Mesh can be streamed for efficient memory management.
    CpuReadable     = XII_BIT(8), ///< Mesh data can be read from CPU.

    Default = StaticGeometry | MeshShaderReady
  };

  struct Bits
  {
    StorageType StaticGeometry : 1;
    StorageType DynamicGeometry : 1;
    StorageType Skinned : 1;
    StorageType MorphTargets : 1;
    StorageType Instancing : 1;
    StorageType MeshShaderReady : 1;
    StorageType RayTracingReady : 1;
    StorageType Streaming : 1;
    StorageType CpuReadable : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiMeshResourceUsageFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshResourceUsageFlags);

/// \brief Selection mode for LODs of a mesh resource, used to determine how LODs are chosen at runtime.
struct XII_GRAPHICSCORE_DLL xiiMeshLodSelectionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Distance = 0U, ///< LOD selection based on distance from the camera.
    ScreenSize,    ///< LOD selection based on projected screen size of the mesh.
    Explicit,      ///< LOD selection is explicitly controlled by the application (e.g., via a property on the mesh component).

    ENUM_COUNT,

    Default = ScreenSize
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshLodSelectionMode);

/// \brief Mesh section describing a contiguous range of primitives with the same material and other properties, used for rendering and culling.
struct XII_GRAPHICSCORE_DLL xiiMeshSection
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32            m_uiFirstPrimitive = 0U;                                  ///< Index of the first primitive in this section.
  xiiUInt32            m_uiPrimitiveCount = 0U;                                  ///< Number of primitives in this section.
  xiiUInt32            m_uiFirstMeshlet   = 0U;                                  ///< Index of the first meshlet in this section.
  xiiUInt32            m_uiMeshletCount   = 0U;                                  ///< Number of meshlets in this section.
  xiiUInt16            m_uiMaterialIndex  = 0U;                                  ///< Index into the mesh's material array for the material used by this section.
  xiiUInt16            m_uiFlags          = 0U;                                  ///< Custom flags for this section, can be used for various purposes (e.g., marking sections as double-sided).
  xiiBoundingBoxSphere m_Bounds           = xiiBoundingBoxSphere::MakeInvalid(); ///< Bounding volume for this section, used for culling.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief Mesh LOD (Level of Detail) containing multiple sections, used to define different levels of detail for a mesh resource.
struct XII_GRAPHICSCORE_DLL xiiMeshLOD
{
  float                              m_fScreenSize    = 1.0f;                        ///< Screen size threshold for this LOD, used for LOD selection when m_LodMode is xiiMeshLodSelectionMode::ScreenSize.
  float                              m_fMaxDistance   = 0.0f;                        ///< Maximum distance for this LOD, used for LOD selection when m_LodMode is xiiMeshLodSelectionMode::Distance.
  xiiUInt32                          m_uiFirstMeshlet = 0U;                          ///< Index of the first meshlet in this LOD, used for rendering and culling.
  xiiUInt32                          m_uiMeshletCount = 0U;                          ///< Number of meshlets in this LOD.
  xiiHybridArray<xiiMeshSection, 8U> m_Sections;                                     ///< Sections contained in this LOD, used for rendering and culling.
  xiiBoundingBoxSphere               m_Bounds = xiiBoundingBoxSphere::MakeInvalid(); ///< Bounding volume for this LOD, used for culling.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief Mesh morph target containing vertex offsets and bounding volume, used for shape animation of a mesh resource.
struct XII_GRAPHICSCORE_DLL xiiMeshMorphTarget
{
  xiiHashedString      m_Name;                                                 ///< Name of the morph target, used for identification and animation control.
  xiiUInt32            m_uiVertexOffset = 0U;                                  ///< Offset into the mesh's vertex buffer where the morph target's vertex data starts, used for rendering and animation.
  xiiUInt32            m_uiVertexCount  = 0U;                                  ///< Number of vertices affected by this morph target, used for rendering and animation.
  xiiBoundingBoxSphere m_Bounds         = xiiBoundingBoxSphere::MakeInvalid(); ///< Bounding volume for this morph target, used for culling and animation control.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief Mesh bone data containing the global inverse rest pose matrix and bone index, used for skeletal animation of a mesh resource.
struct XII_GRAPHICSCORE_DLL xiiMeshBoneData
{
  xiiMat4   m_GlobalInverseRestPoseMatrix = xiiMat4::MakeIdentity();        ///< Global inverse rest pose matrix for this bone, used for skinning calculations in skeletal animation.
  xiiUInt16 m_uiBoneIndex                 = xiiMath::MaxValue<xiiUInt16>(); ///< Index of this bone in the skeleton, used for animation control.

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

/// \brief Authoring and streaming descriptor for xiiMeshResource.
///
/// The descriptor is intentionally GPU-first: sections and LODs are defined in primitive/meshlet
/// ranges, material slots are handles/paths, and the mesh buffer descriptor contains the packed
/// vertex/index/meshlet payload that is uploaded to GAL buffers by xiiMeshBufferResource.
class XII_GRAPHICSCORE_DLL xiiMeshResourceDescriptor
{
public:
  xiiMeshResourceDescriptor();

  void Clear();

  xiiMeshBufferResourceDescriptor&       MeshBufferDescriptor();
  const xiiMeshBufferResourceDescriptor& MeshBufferDescriptor() const;
  void                                   UseExistingMeshBuffer(const xiiMeshBufferResourceHandle& hBuffer);
  const xiiMeshBufferResourceHandle&     GetExistingMeshBuffer() const;

  xiiUInt32                    AddMaterialSlot(xiiStringView sPathToMaterial);
  void                         SetMaterial(xiiUInt32 uiMaterialIndex, xiiStringView sPathToMaterial);
  xiiArrayPtr<const xiiString> GetMaterials() const;

  xiiMeshSection&                   AddSection(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex, xiiUInt32 uiLodIndex = 0U);
  void                              AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex);
  xiiArrayPtr<const xiiMeshSection> GetSubMeshes() const;

  xiiMeshLOD&                   AddLOD(float fScreenSize, float fMaxDistance = 0.0f);
  xiiArrayPtr<const xiiMeshLOD> GetLODs() const;

  void CollapseSubMeshes();
  void ComputeBounds();
  void BuildMeshlets(xiiUInt32 uiMaxVertices = 64U, xiiUInt32 uiMaxPrimitives = 124U);

  const xiiBoundingBoxSphere& GetBounds() const;
  void                        SetBounds(const xiiBoundingBoxSphere& bounds);

  void      Save(xiiStreamWriter& inout_stream) const;
  xiiResult Save(const char* szFile) const;
  xiiResult Load(xiiStreamReader& inout_stream);
  xiiResult Load(const char* szFile);

  xiiBitflags<xiiMeshResourceUsageFlags> m_UsageFlags       = xiiMeshResourceUsageFlags::Default;
  xiiEnum<xiiMeshLodSelectionMode>       m_LodMode          = xiiMeshLodSelectionMode::ScreenSize;
  xiiUInt32                              m_uiStreamingGroup = 0U;
  xiiUInt32                              m_uiMaxResidentLod = 0U;
  xiiUInt32                              m_uiRuntimeHash    = 0U;

  xiiSkeletonResourceHandle                      m_hDefaultSkeleton;
  xiiHashTable<xiiHashedString, xiiMeshBoneData> m_Bones;
  xiiHybridArray<xiiMeshMorphTarget, 4>          m_MorphTargets;
  float                                          m_fMaxBoneVertexOffset = 0.0f;

private:
  xiiHybridArray<xiiString, 8>      m_Materials;
  xiiHybridArray<xiiMeshSection, 8> m_Sections;
  xiiHybridArray<xiiMeshLOD, 4>     m_LODs;
  xiiMeshBufferResourceDescriptor   m_MeshBufferDescriptor;
  xiiMeshBufferResourceHandle       m_hMeshBuffer;
  xiiBoundingBoxSphere              m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
};

class XII_GRAPHICSCORE_DLL xiiMeshResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshResource, xiiMeshResourceDescriptor);

public:
  xiiMeshResource();
  ~xiiMeshResource();

  const xiiMeshBufferResourceHandle&           GetMeshBuffer() const;
  const xiiMaterialResourceHandle&             GetMaterial(xiiUInt32 uiMaterialIndex) const;
  xiiArrayPtr<const xiiMaterialResourceHandle> GetMaterials() const;

  xiiArrayPtr<const xiiMeshLOD>     GetLODs() const;
  xiiArrayPtr<const xiiMeshSection> GetSections() const;

  const xiiBoundingBoxSphere&            GetBounds() const;
  xiiUInt32                              GetLODCount() const;
  xiiUInt32                              GetMeshletCount() const;
  xiiBitflags<xiiMeshResourceUsageFlags> GetUsageFlags() const;
  xiiEnum<xiiMeshLodSelectionMode>       GetLodMode() const;

  const xiiMeshResourceDescriptor& GetDescriptor() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateMeshBufferFromDescriptor(xiiMeshResourceDescriptor& inout_descriptor);
  void LoadMaterialSlots(const xiiMeshResourceDescriptor& descriptor);

  xiiMeshResourceDescriptor                    m_Descriptor;
  xiiMeshBufferResourceHandle                  m_hMeshBuffer;
  xiiHybridArray<xiiMaterialResourceHandle, 8> m_hMaterials;
};
