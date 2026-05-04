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

struct XII_GRAPHICSCORE_DLL xiiMeshResourceUsageFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None              = 0U,
    StaticGeometry    = XII_BIT(0),
    DynamicGeometry   = XII_BIT(1),
    Skinned           = XII_BIT(2),
    MorphTargets      = XII_BIT(3),
    Instancing        = XII_BIT(4),
    MeshShaderReady   = XII_BIT(5),
    RayTracingReady   = XII_BIT(6),
    Streaming         = XII_BIT(7),
    CpuReadable       = XII_BIT(8),

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

struct XII_GRAPHICSCORE_DLL xiiMeshLodSelectionMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Distance,
    ScreenSize,
    Explicit,

    ENUM_COUNT,

    Default = ScreenSize
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshLodSelectionMode);

struct XII_GRAPHICSCORE_DLL xiiMeshSection
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32            m_uiFirstPrimitive = 0U;
  xiiUInt32            m_uiPrimitiveCount = 0U;
  xiiUInt32            m_uiFirstMeshlet   = 0U;
  xiiUInt32            m_uiMeshletCount   = 0U;
  xiiUInt16            m_uiMaterialIndex  = 0U;
  xiiUInt16            m_uiFlags          = 0U;
  xiiBoundingBoxSphere m_Bounds           = xiiBoundingBoxSphere::MakeInvalid();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiMeshLOD
{
  float                 m_fScreenSize = 1.0f;
  float                 m_fMaxDistance = 0.0f;
  xiiUInt32             m_uiFirstMeshlet = 0U;
  xiiUInt32             m_uiMeshletCount = 0U;
  xiiHybridArray<xiiMeshSection, 8> m_Sections;
  xiiBoundingBoxSphere  m_Bounds = xiiBoundingBoxSphere::MakeInvalid();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiMeshMorphTarget
{
  xiiHashedString     m_Name;
  xiiUInt32           m_uiVertexOffset = 0U;
  xiiUInt32           m_uiVertexCount  = 0U;
  xiiBoundingBoxSphere m_Bounds        = xiiBoundingBoxSphere::MakeInvalid();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiMeshBoneData
{
  xiiMat4   m_GlobalInverseRestPoseMatrix = xiiMat4::MakeIdentity();
  xiiUInt16 m_uiBoneIndex                 = xiiMath::MaxValue<xiiUInt16>();

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

  xiiUInt32 AddMaterialSlot(xiiStringView sPathToMaterial);
  void      SetMaterial(xiiUInt32 uiMaterialIndex, xiiStringView sPathToMaterial);
  xiiArrayPtr<const xiiString> GetMaterials() const;

  xiiMeshSection& AddSection(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex, xiiUInt32 uiLodIndex = 0U);
  void            AddSubMesh(xiiUInt32 uiPrimitiveCount, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiMaterialIndex);
  xiiArrayPtr<const xiiMeshSection> GetSubMeshes() const;

  xiiMeshLOD& AddLOD(float fScreenSize, float fMaxDistance = 0.0f);
  xiiArrayPtr<const xiiMeshLOD> GetLODs() const;

  void CollapseSubMeshes();
  void ComputeBounds();
  void BuildMeshlets(xiiUInt32 uiMaxVertices = 64U, xiiUInt32 uiMaxPrimitives = 124U);

  const xiiBoundingBoxSphere& GetBounds() const;
  void                        SetBounds(const xiiBoundingBoxSphere& bounds);

  void     Save(xiiStreamWriter& inout_stream) const;
  xiiResult Save(const char* szFile) const;
  xiiResult Load(xiiStreamReader& inout_stream);
  xiiResult Load(const char* szFile);

  xiiBitflags<xiiMeshResourceUsageFlags> m_UsageFlags = xiiMeshResourceUsageFlags::Default;
  xiiEnum<xiiMeshLodSelectionMode>       m_LodMode    = xiiMeshLodSelectionMode::ScreenSize;
  xiiUInt32                              m_uiStreamingGroup = 0U;
  xiiUInt32                              m_uiMaxResidentLod = 0U;
  xiiUInt32                              m_uiRuntimeHash    = 0U;

  xiiSkeletonResourceHandle                 m_hDefaultSkeleton;
  xiiHashTable<xiiHashedString, xiiMeshBoneData> m_Bones;
  xiiHybridArray<xiiMeshMorphTarget, 4>     m_MorphTargets;
  float                                     m_fMaxBoneVertexOffset = 0.0f;

private:
  xiiHybridArray<xiiString, 8>     m_Materials;
  xiiHybridArray<xiiMeshSection, 8> m_Sections;
  xiiHybridArray<xiiMeshLOD, 4>     m_LODs;
  xiiMeshBufferResourceDescriptor  m_MeshBufferDescriptor;
  xiiMeshBufferResourceHandle      m_hMeshBuffer;
  xiiBoundingBoxSphere             m_Bounds = xiiBoundingBoxSphere::MakeInvalid();
};

class XII_GRAPHICSCORE_DLL xiiMeshResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiMeshResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiMeshResource, xiiMeshResourceDescriptor);

public:
  xiiMeshResource();
  ~xiiMeshResource();

  const xiiMeshBufferResourceHandle& GetMeshBuffer() const;
  const xiiMaterialResourceHandle&   GetMaterial(xiiUInt32 uiMaterialIndex) const;
  xiiArrayPtr<const xiiMaterialResourceHandle> GetMaterials() const;

  xiiArrayPtr<const xiiMeshLOD>     GetLODs() const;
  xiiArrayPtr<const xiiMeshSection> GetSections() const;

  const xiiBoundingBoxSphere& GetBounds() const;
  xiiUInt32                   GetLODCount() const;
  xiiUInt32                   GetMeshletCount() const;
  xiiBitflags<xiiMeshResourceUsageFlags> GetUsageFlags() const;
  xiiEnum<xiiMeshLodSelectionMode>       GetLodMode() const;

  const xiiMeshResourceDescriptor& GetDescriptor() const;

private:
  virtual xiiResourceLoadDesc UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateMeshBufferFromDescriptor(xiiMeshResourceDescriptor& inout_descriptor);
  void LoadMaterialSlots(const xiiMeshResourceDescriptor& descriptor);

  xiiMeshResourceDescriptor m_Descriptor;
  xiiMeshBufferResourceHandle m_hMeshBuffer;
  xiiHybridArray<xiiMaterialResourceHandle, 8> m_hMaterials;
};
