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
/// The descriptor is intentionally GPU-first, that is, sections and LODs are defined in primitive/meshlet ranges, and material slots are handles/paths.
/// The mesh buffer descriptor contains the packed vertex/index/meshlet payload that is uploaded to GAL buffers by xiiMeshBufferResource.
class XII_GRAPHICSCORE_DLL xiiMeshResourceDescriptor
{
public:
  /// \brief Returns a reference to the mesh buffer resource descriptor contained within this mesh resource descriptor.
  xiiMeshBufferResourceDescriptor& GetMeshBufferDescriptor();

  /// \brief Returns a reference to the mesh buffer resource descriptor contained within this mesh resource descriptor.
  const xiiMeshBufferResourceDescriptor& GetMeshBufferDescriptor() const;

  /// \brief Returns a handle to an existing mesh buffer resource that can be used for this mesh resource, or an invalid handle if no existing mesh buffer can be used.
  const xiiMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  /// \brief Returns a reference to the array of material paths contained within this mesh resource descriptor.
  xiiArrayPtr<const xiiString> GetMaterials() const;

  /// \brief Returns a reference to the array of sections contained within this mesh resource descriptor.
  xiiArrayPtr<const xiiMeshSection> GetSubMeshes() const;

  /// \brief Returns a reference to the array of LODs contained within this mesh resource descriptor.
  xiiArrayPtr<const xiiMeshLOD> GetLODs() const;

  /// \brief Returns the bounding volume for this mesh resource, used for culling and LOD selection.
  const xiiBoundingBoxSphere& GetBounds() const;

public:
  /// \brief Default constructor, initializes an empty mesh resource descriptor.
  xiiMeshResourceDescriptor();

  /// \brief Clears all data from the descriptor, resetting it to an empty state.
  void Clear();

  /// \brief Specifies that the mesh buffer for this mesh resource should be created from the given descriptor, instead of using an existing mesh buffer resource.
  void UseExistingMeshBuffer(const xiiMeshBufferResourceHandle& hBuffer);

  /// \brief Adds a new material slot to the mesh resource descriptor with the given path, and returns the index of the new material slot.
  xiiUInt32 AddMaterialSlot(xiiStringView sPathToMaterial);

  /// \brief Sets the material path for the material slot at the given index.
  ///
  /// \param uiMaterialIndex Index of the material slot to set, must be less than the number of material slots in the descriptor.
  /// \param sPathToMaterial Path to the material to set for this material slot.
  void SetMaterial(xiiUInt32 uiMaterialIndex, xiiStringView sPathToMaterial);

  /// \brief Adds a new section to the mesh resource descriptor with the given properties, and returns a reference to the new section.
  ///
  /// \param uiPrimitiveCount Number of primitives in this section, used for rendering and culling.
  /// \param uiFirstPrimitive Index of the first primitive in this section, used for rendering and culling.
  /// \param uiMaterialIndex Index into the mesh's material array for the material used by this section, used for rendering.
  /// \param uiLodIndex Index of the LOD to which this section belongs. If the specified LOD does not exist, it will be created. If uiLodIndex is greater than 0, the section will be added to the specified LOD; otherwise, it will be added as a sub-mesh (section at LOD 0).
  xiiMeshSection& AddSection(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex, xiiUInt32 uiLodIndex = 0U);

  /// \brief Adds a new sub-mesh (section at LOD 0) to the mesh resource descriptor with the given properties.
  ///
  /// \param uiPrimitiveCount Number of primitives in this sub-mesh, used for rendering and culling.
  /// \param uiFirstPrimitive Index of the first primitive in this sub-mesh, used for rendering and culling.
  /// \param uiMaterialIndex Index into the mesh's material array for the material used by this sub-mesh, used for rendering.
  void AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex);

  /// \brief Adds a new LOD to the mesh resource descriptor with the given properties, and returns a reference to the new LOD.
  ///
  /// \param fScreenSize Screen size threshold for this LOD, used for LOD selection when m_LodMode is xiiMeshLodSelectionMode::ScreenSize.
  /// \param fMaxDistance Maximum distance for this LOD, used for LOD selection.
  ///
  /// \return Reference to the newly added LOD, which can be further modified (e.g., by adding sections to it).
  xiiMeshLOD& AddLOD(float fScreenSize, float fMaxDistance = 0.0f);

  /// \brief Collapses all sub-meshes (sections at LOD 0) into a single section, used to optimize meshes that have multiple small sections.
  void CollapseSubMeshes();

  /// \brief Computes the bounding volume for the mesh resource based on the vertex data in the mesh buffer descriptor, used for culling and LOD selection.
  void ComputeBounds();

  /// \brief Builds meshlets for the mesh resource based on the sections and LODs defined in the descriptor, used for rendering with mesh shaders.
  ///
  /// \param uiMaxVertices Maximum number of vertices per meshlet, used to control the size of meshlets for rendering.
  /// \param uiMaxPrimitives Maximum number of primitives per meshlet, used to control the size of meshlets for rendering.
  void BuildMeshlets(xiiUInt32 uiMaxVertices = 64U, xiiUInt32 uiMaxPrimitives = 124U);

  /// \brief Sets the bounding volume for this mesh resource, used for culling and LOD selection.
  void SetBounds(const xiiBoundingBoxSphere& bounds);

public:
  void      Save(xiiStreamWriter& inout_stream) const;
  xiiResult Save(xiiStringView sFile) const;

  xiiResult Load(xiiStreamReader& inout_stream);
  xiiResult Load(xiiStringView sFile);

public:
  xiiBitflags<xiiMeshResourceUsageFlags> m_UsageFlags       = xiiMeshResourceUsageFlags::Default;  ///< Usage flags for this mesh resource, used to specify intended usage patterns and GPU feature support.
  xiiEnum<xiiMeshLodSelectionMode>       m_LodMode          = xiiMeshLodSelectionMode::ScreenSize; ///< LOD selection mode for this mesh resource.
  xiiUInt32                              m_uiStreamingGroup = 0U;                                  ///< Streaming group index for this mesh resource, used to group meshes for streaming purposes.
  xiiUInt32                              m_uiMaxResidentLod = 0U;                                  ///< Maximum resident LOD index for this mesh resource, used to control how many LODs are kept in memory when streaming.
  xiiUInt32                              m_uiRuntimeHash    = 0U;                                  ///< Runtime hash for this mesh resource, used for quick comparisons and lookups at runtime.

  xiiSkeletonResourceHandle                      m_hDefaultSkeleton;            ///< Handle to the default skeleton resource for this mesh, used for skeletal animation when no specific skeleton is assigned.
  xiiHashTable<xiiHashedString, xiiMeshBoneData> m_Bones;                       ///< Hash table mapping bone names to bone data for this mesh, used for skeletal animation.
  xiiDynamicArray<xiiMeshMorphTarget>            m_MorphTargets;                ///< Array of morph targets for this mesh, used for shape animation.
  float                                          m_fMaxBoneVertexOffset = 0.0f; ///< Maximum vertex offset caused by bone influences in this mesh, used for bounding volume calculations and culling of skinned meshes.

private:
  xiiDynamicArray<xiiString>      m_Materials;                                    ///< Array of material paths for this mesh, used to reference materials for rendering.
  xiiDynamicArray<xiiMeshSection> m_Sections;                                     ///< Array of mesh sections (sub-meshes) for this mesh, used for rendering and culling.
  xiiDynamicArray<xiiMeshLOD>     m_LODs;                                         ///< Array of LODs for this mesh, used for LOD selection and rendering.
  xiiMeshBufferResourceDescriptor m_MeshBufferDescriptor;                         ///< Descriptor for the mesh buffer resource associated with this mesh, used to define the vertex/index/meshlet data for this mesh.
  xiiMeshBufferResourceHandle     m_hMeshBuffer;                                  ///< Handle to the mesh buffer resource associated with this mesh, used to reference the GPU buffers for rendering.
  xiiBoundingBoxSphere            m_Bounds = xiiBoundingBoxSphere::MakeInvalid(); ///< Bounding volume for this mesh, used for culling and LOD selection.
};

/// \brief Mesh resource class representing a renderable mesh in the engine, containing geometry, materials, LODs, and other properties for rendering and animation.
class XII_GRAPHICSCORE_DLL xiiMeshResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshResource);

  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshResource, xiiMeshResourceDescriptor);

public:
  xiiMeshResource();
  ~xiiMeshResource();

  /// \brief Returns the descriptor used to create this mesh resource, containing all the data and properties for this mesh.
  const xiiMeshResourceDescriptor& GetDescriptor() const;

  /// \brief Returns a handle to the mesh buffer resource associated with this mesh, used to access the vertex/index/meshlet data for rendering.
  const xiiMeshBufferResourceHandle& GetMeshBuffer() const;

  /// \brief Returns a reference to the material resource handle for the material slot at the given index, used to access the material for rendering.
  ///
  /// \param uiMaterialIndex Index of the material slot to retrieve, must be less than the number of material slots in this mesh.
  const xiiMaterialResourceHandle& GetMaterial(xiiUInt32 uiMaterialIndex) const;

  /// \brief Returns a reference to the array of material resource handles for this mesh, used to access the materials for rendering.
  xiiArrayPtr<const xiiMaterialResourceHandle> GetMaterials() const;

  /// \brief Returns a reference to the array of LODs for this mesh, used for LOD selection and rendering.
  xiiArrayPtr<const xiiMeshLOD> GetLODs() const;

  /// \brief Returns a reference to the array of sections for this mesh, used for rendering and culling.
  xiiArrayPtr<const xiiMeshSection> GetSections() const;

  /// \brief Returns the bounding volume for this mesh, used for culling and LOD selection.
  const xiiBoundingBoxSphere& GetBounds() const;

  /// \brief Returns the number of LODs for this mesh.
  xiiUInt32 GetLODCount() const;

  /// \brief Returns the number of sections for this mesh.
  xiiUInt32 GetMeshletCount() const;

  /// \brief Returns the usage flags for this mesh, used to specify intended usage patterns and GPU feature support.
  xiiBitflags<xiiMeshResourceUsageFlags> GetUsageFlags() const;

  /// \brief Returns the LOD selection mode for this mesh, used to determine how LODs are chosen at runtime.
  xiiEnum<xiiMeshLodSelectionMode> GetLodMode() const;

private:
  virtual xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  /// \brief Creates the mesh buffer resource for this mesh based on the mesh buffer descriptor contained in the provided mesh resource descriptor, used to initialize the GPU buffers for this mesh.
  void CreateMeshBufferFromDescriptor(xiiMeshResourceDescriptor& inout_descriptor);

  /// \brief Loads the material resources for this mesh based on the material paths contained in the provided mesh resource descriptor, used to initialize the materials for this mesh.
  void LoadMaterialSlots(const xiiMeshResourceDescriptor& descriptor);

private:
  xiiMeshResourceDescriptor                  m_Descriptor;
  xiiMeshBufferResourceHandle                m_hMeshBuffer;
  xiiDynamicArray<xiiMaterialResourceHandle> m_hMaterials;
};
