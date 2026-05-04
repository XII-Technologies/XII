/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Pipeline/RenderData.h>

#include <Foundation/Types/SharedPtr.h>

class xiiGALBuffer;
struct xiiMsgExtractRenderData;

using xiiStaticMeshComponentManager    = xiiComponentManager<class xiiStaticMeshComponent, xiiBlockStorageType::Compact>;
using xiiMeshComponentManager          = xiiComponentManager<class xiiMeshComponent, xiiBlockStorageType::Compact>;
using xiiDynamicMeshComponentManager   = xiiComponentManager<class xiiDynamicMeshComponent, xiiBlockStorageType::Compact>;
using xiiSkinnedMeshComponentManager   = xiiComponentManager<class xiiSkinnedMeshComponent, xiiBlockStorageType::Compact>;
using xiiInstancedMeshComponentManager = xiiComponentManager<class xiiInstancedMeshComponent, xiiBlockStorageType::Compact>;
using xiiLODMeshComponentManager       = xiiComponentManager<class xiiLODMeshComponent, xiiBlockStorageType::Compact>;

struct XII_GRAPHICSCORE_DLL xiiMeshRenderDataFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None              = 0U,
    StaticObject      = XII_BIT(0),
    DynamicObject     = XII_BIT(1),
    Skinned           = XII_BIT(2),
    MorphTargets      = XII_BIT(3),
    Instanced         = XII_BIT(4),
    PreferMeshShader  = XII_BIT(5),
    ForceLOD          = XII_BIT(6),
    CpuCullingFallback = XII_BIT(7),
    RayTracingVisible = XII_BIT(8),

    Default = PreferMeshShader
  };

  struct Bits
  {
    StorageType StaticObject : 1;
    StorageType DynamicObject : 1;
    StorageType Skinned : 1;
    StorageType MorphTargets : 1;
    StorageType Instanced : 1;
    StorageType PreferMeshShader : 1;
    StorageType ForceLOD : 1;
    StorageType CpuCullingFallback : 1;
    StorageType RayTracingVisible : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiMeshRenderDataFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMeshRenderDataFlags);

/// \brief Renderer-facing packet for mesh draws.
///
/// The packet is deliberately meshlet/indirect friendly. Render passes can bind the mesh buffer
/// resources directly, select sections/LODs on GPU, upload instance/skin/morph streams, and emit
/// mesh shader or indexed fallback draws from the same data.
class XII_GRAPHICSCORE_DLL xiiMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle       m_hMesh;
  xiiMeshBufferResourceHandle m_hMeshBuffer;
  xiiHybridArray<xiiMaterialResourceHandle, 8> m_hMaterials;

  xiiUInt32 m_uiUniqueID       = 0U;
  xiiUInt32 m_uiLODIndex       = 0U;
  xiiUInt32 m_uiFirstMeshlet   = 0U;
  xiiUInt32 m_uiMeshletCount   = 0U;
  xiiUInt32 m_uiFirstPrimitive = 0U;
  xiiUInt32 m_uiPrimitiveCount = 0U;
  xiiUInt32 m_uiSectionIndex   = xiiInvalidIndex;
  xiiUInt32 m_uiInstanceCount  = 1U;

  float m_fLodBias = 0.0f;

  xiiBitflags<xiiMeshRenderDataFlags> m_Flags = xiiMeshRenderDataFlags::Default;

  xiiHybridArray<xiiMat4, 64> m_InstanceTransforms;
  xiiHybridArray<xiiMat4, 96> m_SkinningMatrices;
  xiiHybridArray<float, 16>   m_MorphWeights;

  xiiSharedPtr<xiiGALBuffer> m_pInstanceDataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pSkinningMatricesBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pMorphWeightsBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pDrawCommandBuffer;
};

/// \brief Common functionality for all mesh components.
class XII_GRAPHICSCORE_DLL xiiMeshComponentBase : public xiiRenderComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiMeshComponentBase, xiiRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRenderComponent

public:
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

public:
  xiiMeshComponentBase();
  ~xiiMeshComponentBase();

  void                         SetMesh(const xiiMeshResourceHandle& hMesh); // [ property ]
  const xiiMeshResourceHandle& GetMesh() const;                             // [ property ]

  void                             SetMaterialOverride(xiiUInt32 uiMaterialIndex, const xiiMaterialResourceHandle& hMaterial);
  const xiiMaterialResourceHandle& GetMaterialOverride(xiiUInt32 uiMaterialIndex) const;
  void                             ClearMaterialOverrides();

  void SetSectionIndex(xiiUInt32 uiSectionIndex); // [ property ]
  xiiUInt32 GetSectionIndex() const;              // [ property ]

  void SetPreferMeshShaders(bool bPreferMeshShaders); // [ property ]
  bool GetPreferMeshShaders() const;                   // [ property ]

  void SetRayTracingVisible(bool bVisible); // [ property ]
  bool GetRayTracingVisible() const;        // [ property ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  virtual void                         FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const;
  virtual xiiUInt32                    SelectLOD(const xiiMeshResource& mesh) const;
  virtual xiiRenderData::Caching::Enum GetRenderDataCaching() const;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const;
  virtual void                         UpdateLocalBoundsForInstances(xiiBoundingBoxSphere& ref_bounds) const;

protected:
  xiiMeshResourceHandle m_hMesh;
  xiiHybridArray<xiiMaterialResourceHandle, 8> m_MaterialOverrides;

  xiiUInt32 m_uiSectionIndex       = xiiInvalidIndex;
  bool      m_bPreferMeshShaders   = true;
  bool      m_bRayTracingVisible   = true;
  bool      m_bCpuCullingFallback  = true;
};

/// \brief Static mesh component optimized for cached extraction and GPU-driven rendering.
class XII_GRAPHICSCORE_DLL xiiStaticMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiStaticMeshComponent, xiiMeshComponentBase, xiiStaticMeshComponentManager);

public:
  xiiStaticMeshComponent();
  ~xiiStaticMeshComponent();

protected:
  virtual xiiRenderData::Caching::Enum GetRenderDataCaching() const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;
};

/// \brief Default mesh component name used by scenes and tools.
class XII_GRAPHICSCORE_DLL xiiMeshComponent : public xiiStaticMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMeshComponent, xiiStaticMeshComponent, xiiMeshComponentManager);

public:
  xiiMeshComponent();
  ~xiiMeshComponent();
};

/// \brief Dynamic mesh component for frequently updated meshes and per-frame extraction.
class XII_GRAPHICSCORE_DLL xiiDynamicMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiDynamicMeshComponent, xiiMeshComponentBase, xiiDynamicMeshComponentManager);

public:
  xiiDynamicMeshComponent();
  ~xiiDynamicMeshComponent();

protected:
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;
};

/// \brief Mesh component carrying palette skinning and morph target data.
class XII_GRAPHICSCORE_DLL xiiSkinnedMeshComponent : public xiiDynamicMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkinnedMeshComponent, xiiDynamicMeshComponent, xiiSkinnedMeshComponentManager);

public:
  xiiSkinnedMeshComponent();
  ~xiiSkinnedMeshComponent();

  void SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const;         // [ property ]

  void SetSkinningMatrices(xiiArrayPtr<const xiiMat4> matrices);
  void SetMorphWeights(xiiArrayPtr<const float> weights);
  void ClearPose();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;

protected:
  xiiSkeletonResourceHandle m_hSkeleton;
  xiiHybridArray<xiiMat4, 96> m_SkinningMatrices;
  xiiHybridArray<float, 16>   m_MorphWeights;
};

/// \brief Instanced mesh component for many local-space instances of one mesh resource.
class XII_GRAPHICSCORE_DLL xiiInstancedMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiInstancedMeshComponent, xiiMeshComponentBase, xiiInstancedMeshComponentManager);

public:
  xiiInstancedMeshComponent();
  ~xiiInstancedMeshComponent();

  xiiUInt32 AddInstance(const xiiMat4& transform);
  void      SetInstanceTransform(xiiUInt32 uiIndex, const xiiMat4& transform);
  void      SetInstanceCount(xiiUInt32 uiCount);
  void      ClearInstances();

  xiiUInt32 GetInstanceCount() const;
  xiiArrayPtr<const xiiMat4> GetInstanceTransforms() const;

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;
  virtual void UpdateLocalBoundsForInstances(xiiBoundingBoxSphere& ref_bounds) const override;

protected:
  xiiHybridArray<xiiMat4, 64> m_InstanceTransforms;
};

/// \brief Mesh component with explicit CPU-side LOD controls.
class XII_GRAPHICSCORE_DLL xiiLODMeshComponent : public xiiStaticMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLODMeshComponent, xiiStaticMeshComponent, xiiLODMeshComponentManager);

public:
  xiiLODMeshComponent();
  ~xiiLODMeshComponent();

  void      SetForcedLOD(xiiUInt32 uiLOD); // [ property ]
  xiiUInt32 GetForcedLOD() const;          // [ property ]

  void  SetLodBias(float fBias); // [ property ]
  float GetLodBias() const;      // [ property ]

protected:
  virtual void      FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const override;
  virtual xiiUInt32 SelectLOD(const xiiMeshResource& mesh) const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;

protected:
  xiiUInt32 m_uiForcedLOD = xiiInvalidIndex;
  float     m_fLodBias    = 0.0f;
};
