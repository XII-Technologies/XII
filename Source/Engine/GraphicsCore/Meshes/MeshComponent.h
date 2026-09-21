/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Pipeline/RenderData.h>

class xiiGALBuffer;
class xiiMeshResource;
class xiiMaterialResource;

struct xiiMsgExtractRenderData;

using xiiStaticMeshComponentManager    = xiiComponentManager<class xiiStaticMeshComponent, xiiBlockStorageType::Compact>;
using xiiMeshComponentManager          = xiiComponentManager<class xiiMeshComponent, xiiBlockStorageType::Compact>;
using xiiDynamicMeshComponentManager   = xiiComponentManager<class xiiDynamicMeshComponent, xiiBlockStorageType::Compact>;
using xiiSkinnedMeshComponentManager   = xiiComponentManager<class xiiSkinnedMeshComponent, xiiBlockStorageType::Compact>;
using xiiInstancedMeshComponentManager = xiiComponentManager<class xiiInstancedMeshComponent, xiiBlockStorageType::Compact>;
using xiiLODMeshComponentManager       = xiiComponentManager<class xiiLODMeshComponent, xiiBlockStorageType::Compact>;

/// Mesh render data flags, used to specify various properties of the mesh render data that can affect how it is rendered.
struct XII_GRAPHICSCORE_DLL xiiMeshRenderDataFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None               = 0U,         ///< No special flags, default for most meshes.
    StaticObject       = XII_BIT(0), ///< Mesh represents static geometry that does not move or animate, allowing for more aggressive culling and batching optimizations.
    DynamicObject      = XII_BIT(1), ///< Mesh represents dynamic geometry that may move or animate, requiring different rendering strategies.
    Skinned            = XII_BIT(2), ///< Mesh is skinned and requires skinning matrices for rendering.
    MorphTargets       = XII_BIT(3), ///< Mesh has morph targets for shape blending.
    Instanced          = XII_BIT(4), ///< Mesh is instanced, allowing for efficient rendering of multiple instances.
    PreferMeshShader   = XII_BIT(5), ///< Prefer using mesh shaders for rendering, if available.
    ForceLOD           = XII_BIT(6), ///< Force the use of a specific LOD level.
    CpuCullingFallback = XII_BIT(7), ///< Use CPU culling as a fallback.
    RayTracingVisible  = XII_BIT(8), ///< Mesh is visible to ray tracing.

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

/// Renderer-facing packet for mesh draws.
///
/// The packet is deliberately meshlet/indirect friendly. Render passes can bind the mesh buffer
/// resources directly, select sections/LODs on GPU, upload instance/skin/morph streams, and emit
/// mesh shader or indexed fallback draws from the same data.
class XII_GRAPHICSCORE_DLL xiiMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle                      m_hMesh;       ///< The mesh resource to render, containing the mesh buffer and materials.
  xiiMeshBufferResourceHandle                m_hMeshBuffer; ///< The mesh buffer resource containing the GPU buffers for this mesh, used for rendering and culling.
  xiiDynamicArray<xiiMaterialResourceHandle> m_hMaterials;  ///< The material resources for this mesh, used for rendering.

  xiiUInt32 m_uiUniqueID       = 0U;              ///< A unique identifier for this mesh render data, used for caching and sorting.
  xiiUInt32 m_uiLODIndex       = 0U;              ///< The LOD index to render, used to select the appropriate LOD from the mesh resource.
  xiiUInt32 m_uiFirstMeshlet   = 0U;              ///< The index of the first meshlet to render, used for meshlet-based rendering.
  xiiUInt32 m_uiMeshletCount   = 0U;              ///< The number of meshlets to render, used for meshlet-based rendering.
  xiiUInt32 m_uiFirstPrimitive = 0U;              ///< The index of the first primitive to render, used for indexed rendering.
  xiiUInt32 m_uiPrimitiveCount = 0U;              ///< The number of primitives to render, used for indexed rendering.
  xiiUInt32 m_uiSectionIndex   = xiiInvalidIndex; ///< The section index to render, used to select a specific section of the mesh for rendering.
  xiiUInt32 m_uiInstanceCount  = 1U;              ///< The number of instances to render, used for instanced rendering.

  float m_fLodBias = 0.0f; ///< LOD bias to apply when selecting the LOD level to render, used to adjust LOD selection based on distance or performance requirements.

  xiiBitflags<xiiMeshRenderDataFlags> m_Flags = xiiMeshRenderDataFlags::Default; ///< Flags specifying various properties of the mesh render data, used to determine how the mesh should be rendered.

  xiiDynamicArray<xiiMat4> m_InstanceTransforms; ///< Array of instance transforms for instanced rendering, used to provide per-instance transformation data to the GPU.
  xiiDynamicArray<xiiMat4> m_SkinningMatrices;   ///< Array of skinning matrices for skinned rendering, used to provide per-vertex transformation data to the GPU.
  xiiDynamicArray<float>   m_MorphWeights;       ///< Array of morph weights for morph target rendering, used to interpolate between different mesh shapes.

  xiiSharedPtr<xiiGALBuffer> m_pInstanceDataBuffer;     ///< GPU buffer for instance data, used for instanced rendering to provide per-instance data to the GPU.
  xiiSharedPtr<xiiGALBuffer> m_pSkinningMatricesBuffer; ///< GPU buffer for skinning matrices, used for skinned rendering to provide per-vertex transformation data to the GPU.
  xiiSharedPtr<xiiGALBuffer> m_pMorphWeightsBuffer;     ///< GPU buffer for morph weights, used for morph target rendering to provide morph weight data to the GPU.
  xiiSharedPtr<xiiGALBuffer> m_pDrawCommandBuffer;      ///< GPU buffer for indirect draw commands, used for indirect rendering to provide draw command data to the GPU.
};

/// Common functionality for all mesh components.
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

  /// Sets the mesh resource for this component, used to specify the geometry and materials to render.
  void SetMesh(const xiiMeshResourceHandle& hMesh); // [ property ]

  /// Returns a handle to the mesh resource for this component, used to access the geometry and materials for rendering.
  const xiiMeshResourceHandle& GetMesh() const; // [ property ]

  /// Sets a material override for the material slot at the given index, used to override the material for rendering.
  void SetMaterialOverride(xiiUInt32 uiMaterialIndex, const xiiMaterialResourceHandle& hMaterial);

  /// Returns a handle to the material override for the material slot at the given index, used to access the overridden material for rendering.
  const xiiMaterialResourceHandle& GetMaterialOverride(xiiUInt32 uiMaterialIndex) const;

  /// Clears all material overrides, used to reset the materials to the defaults specified in the mesh resource.
  void ClearMaterialOverrides();

  /// Returns a reference to the array of material overrides for this component, used to access all overridden materials for rendering.
  xiiArrayPtr<const xiiMaterialResourceHandle> GetMaterialOverrides() const;

  /// Reflection accessor: returns material override count.
  xiiUInt32 GetMaterialOverrideCount() const; // [ property ]

  /// Reflection accessor: returns the material override resource ID at the given index.
  xiiStringView GetMaterialOverrideFile(xiiUInt32 uiMaterialIndex) const; // [ property ]

  /// Reflection accessor: sets a material override from a resource ID.
  void SetMaterialOverrideFile(xiiUInt32 uiMaterialIndex, xiiStringView sFile); // [ property ]

  /// Reflection accessor: inserts a material override from a resource ID.
  void InsertMaterialOverrideFile(xiiUInt32 uiMaterialIndex, xiiStringView sFile); // [ property ]

  /// Reflection accessor: removes a material override at the given index.
  void RemoveMaterialOverrideFile(xiiUInt32 uiMaterialIndex); // [ property ]

  /// Sets the section index for this component, used to select a specific section of the mesh for rendering.
  void SetSectionIndex(xiiUInt32 uiSectionIndex); // [ property ]

  /// Returns the section index for this component, used to select a specific section of the mesh for rendering.
  xiiUInt32 GetSectionIndex() const; // [ property ]

  /// Sets whether to prefer using mesh shaders for rendering this mesh, if available, used to optimize rendering performance on supported hardware.
  void SetPreferMeshShaders(bool bPreferMeshShaders); // [ property ]

  /// Returns whether to prefer using mesh shaders for rendering this mesh, if available, used to determine the rendering path for this mesh.
  bool GetPreferMeshShaders() const; // [ property ]

  /// Sets whether this mesh should be visible to ray tracing, used to control ray tracing visibility for this mesh.
  void SetRayTracingVisible(bool bVisible); // [ property ]

  /// Returns whether this mesh is visible to ray tracing, used to determine ray tracing visibility for this mesh.
  bool GetRayTracingVisible() const; // [ property ]

protected:
  /// Handles the message for extracting render data for this mesh, used to prepare and submit the render data for this mesh when requested by the renderer.
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  /// Fills the given mesh render data with the appropriate data for rendering this mesh, used to prepare the render data for this mesh based on the mesh resource and component properties.
  virtual void FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const;

  /// Selects the appropriate LOD index for rendering this mesh based on the given mesh resource, used to determine which LOD level of the mesh to render based on distance or performance requirements.
  virtual xiiUInt32 SelectLOD(const xiiMeshResource& mesh) const;

  /// Returns the render data caching strategy for this mesh component, used to determine how the render data for this mesh should be cached and reused.
  virtual xiiRenderData::Caching::Enum GetRenderDataCaching() const;

  /// Returns the mesh render data flags for this mesh component, used to specify various properties of the mesh render data that can affect how it is rendered.
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const;

  /// Updates the local bounds for this mesh component based on the given mesh resource, used to calculate the bounding volume for this mesh for culling and LOD selection.
  virtual void UpdateLocalBoundsForInstances(xiiBoundingBoxSphere& ref_bounds) const;

protected:
  xiiMeshResourceHandle                      m_hMesh;             ///< The mesh resource for this component, containing the mesh buffer and materials, used to specify the geometry and materials to render.
  xiiDynamicArray<xiiMaterialResourceHandle> m_MaterialOverrides; ///< Array of material overrides for this component, used to override the materials specified in the mesh resource for rendering.

  xiiUInt32 m_uiSectionIndex      = xiiInvalidIndex; ///< The section index for this component, used to select a specific section of the mesh for rendering.
  bool      m_bPreferMeshShaders  = true;            ///< Whether to prefer using mesh shaders for rendering this mesh, if available, used to optimize rendering performance on supported hardware.
  bool      m_bRayTracingVisible  = true;            ///< Whether this mesh should be visible to ray tracing, used to control ray tracing visibility for this mesh.
  bool      m_bCpuCullingFallback = true;            ///< Whether to use CPU culling as a fallback, used to determine whether to fall back to CPU culling if GPU culling is not available or fails for this mesh.
};

/// Static mesh component optimized for cached extraction and GPU-driven rendering.
class XII_GRAPHICSCORE_DLL xiiStaticMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiStaticMeshComponent, xiiMeshComponentBase, xiiStaticMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual xiiRenderData::Caching::Enum        GetRenderDataCaching() const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiStaticMeshComponent

public:
  xiiStaticMeshComponent();
  ~xiiStaticMeshComponent();
};

/// Default mesh component name used by scenes and tools.
class XII_GRAPHICSCORE_DLL xiiMeshComponent : public xiiStaticMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMeshComponent, xiiStaticMeshComponent, xiiMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiStaticMeshComponent

public:
  xiiMeshComponent();
  ~xiiMeshComponent();
};

/// Dynamic mesh component for frequently updated meshes and per-frame extraction.
class XII_GRAPHICSCORE_DLL xiiDynamicMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiDynamicMeshComponent, xiiMeshComponentBase, xiiDynamicMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiDynamicMeshComponent

public:
  xiiDynamicMeshComponent();
  ~xiiDynamicMeshComponent();
};

/// Mesh component carrying palette skinning and morph target data.
class XII_GRAPHICSCORE_DLL xiiSkinnedMeshComponent : public xiiDynamicMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkinnedMeshComponent, xiiDynamicMeshComponent, xiiSkinnedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual void                                FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSkinnedMeshComponent

public:
  xiiSkinnedMeshComponent();
  ~xiiSkinnedMeshComponent();

  /// Sets the skeleton resource for this skinned mesh component, used to specify the skeleton for skinning this mesh.
  void SetSkeleton(const xiiSkeletonResourceHandle& hSkeleton); // [ property ]

  /// Returns a handle to the skeleton resource for this skinned mesh component, used to access the skeleton for skinning this mesh.
  const xiiSkeletonResourceHandle& GetSkeleton() const; // [ property ]

  /// Sets the skinning matrices for this skinned mesh component, used to provide the skinning transformation data for rendering this mesh.
  void SetSkinningMatrices(xiiArrayPtr<const xiiMat4> pMatrices);

  /// Returns a reference to the array of skinning matrices for this skinned mesh component, used to access the skinning transformation data for rendering this mesh.
  xiiArrayPtr<const xiiMat4> GetSkinningMatrices() const;

  /// Sets the morph target weights for this skinned mesh component, used to provide the morph target blending data for rendering this mesh.
  void SetMorphWeights(xiiArrayPtr<const float> pWeights);

  /// Returns a reference to the array of morph target weights for this skinned mesh component, used to access the morph target blending data for rendering this mesh.
  xiiArrayPtr<const float> GetMorphWeights() const;

  /// Clears all skinning and morph target data, used to reset the skinning and morphing state for this mesh.
  void ClearPose();

protected:
  xiiSkeletonResourceHandle m_hSkeleton;        ///< The skeleton resource for this skinned mesh component, used to specify the skeleton for skinning this mesh.
  xiiDynamicArray<xiiMat4>  m_SkinningMatrices; ///< Array of skinning matrices for this skinned mesh component, used to provide the skinning transformation data for rendering this mesh.
  xiiDynamicArray<float>    m_MorphWeights;     ///< Array of morph target weights for this skinned mesh component, used to provide the morph target blending data for rendering this mesh.
};

/// Instanced mesh component for many local-space instances of one mesh resource.
class XII_GRAPHICSCORE_DLL xiiInstancedMeshComponent : public xiiMeshComponentBase
{
  XII_DECLARE_COMPONENT_TYPE(xiiInstancedMeshComponent, xiiMeshComponentBase, xiiInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual void                                FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;
  virtual void                                UpdateLocalBoundsForInstances(xiiBoundingBoxSphere& ref_bounds) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiInstancedMeshComponent

public:
  xiiInstancedMeshComponent();
  ~xiiInstancedMeshComponent();

  /// Returns the number of instances for this instanced mesh component, used to determine how many instances of this mesh are being rendered.
  xiiUInt32 GetInstanceCount() const;

  /// Returns a reference to the array of instance transforms for this instanced mesh component, used to access the transformation data for rendering each instance of this mesh.
  xiiArrayPtr<const xiiMat4> GetInstanceTransforms() const;

  /// Adds an instance with the given transform to this instanced mesh component, used to specify the transformation for a new instance of this mesh to render.
  xiiUInt32 AddInstance(const xiiMat4& transform);

  /// Sets the transform for the instance at the given index, used to update the transformation for a specific instance of this mesh to render.
  void SetInstanceTransform(xiiUInt32 uiIndex, const xiiMat4& transform);

  /// Removes the instance at the given index from this instanced mesh component, used to stop rendering a specific instance of this mesh.
  void RemoveInstance(xiiUInt32 uiIndex);

  /// Sets the number of instances for this instanced mesh component, used to specify how many instances of this mesh to render. If the count is increased, new instances will be added with identity transforms. If the count is decreased, existing instances will be removed from the end.
  void SetInstanceCount(xiiUInt32 uiCount);

  /// Removes all instances from this instanced mesh component, used to stop rendering all instances of this mesh.
  void ClearInstances();

protected:
  xiiDynamicArray<xiiMat4> m_InstanceTransforms; ///< Array of instance transforms for this instanced mesh component, used to provide the transformation data for rendering each instance.
};

/// Mesh component with explicit CPU-side LOD controls.
class XII_GRAPHICSCORE_DLL xiiLODMeshComponent : public xiiStaticMeshComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiLODMeshComponent, xiiStaticMeshComponent, xiiLODMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiMeshComponentBase

protected:
  virtual void                                FillRenderData(xiiMeshRenderData& ref_renderData, const xiiMeshResource& mesh) const override;
  virtual xiiUInt32                           SelectLOD(const xiiMeshResource& mesh) const override;
  virtual xiiBitflags<xiiMeshRenderDataFlags> GetMeshRenderFlags() const override;

public:
  xiiLODMeshComponent();
  ~xiiLODMeshComponent();

  /// Sets the forced LOD index for this mesh component, used to override the automatic LOD selection and force a specific LOD level to be rendered.
  void SetForcedLOD(xiiUInt32 uiLOD); // [ property ]

  /// Returns the forced LOD index for this mesh component, used to determine if a specific LOD level is being forced for rendering this mesh.
  xiiUInt32 GetForcedLOD() const; // [ property ]

  /// Sets the LOD bias for this mesh component, used to adjust the LOD selection for this mesh based on distance or performance requirements.
  void SetLodBias(float fBias); // [ property ]

  /// Returns the LOD bias for this mesh component, used to determine the LOD selection bias for rendering this mesh.
  float GetLodBias() const; // [ property ]

protected:
  xiiUInt32 m_uiForcedLOD = xiiInvalidIndex; ///< The forced LOD index for this mesh component, used to override the automatic LOD selection and force a specific LOD level to be rendered. If set to xiiInvalidIndex, no LOD is forced and the LOD selection will be automatic based on distance or performance requirements.
  float     m_fLodBias    = 0.0f;            ///< The LOD bias for this mesh component, used to adjust the LOD selection for this mesh based on distance or performance requirements. A positive bias will make the LOD selection favor higher detail levels, while a negative bias will favor lower detail levels.
};
