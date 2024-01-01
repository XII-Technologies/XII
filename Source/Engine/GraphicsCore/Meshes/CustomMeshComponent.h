#pragma once

#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsCore/Pipeline/Renderer.h>

using xiiDynamicMeshBufferResourceHandle = xiiTypedResourceHandle<class xiiDynamicMeshBufferResource>;
using xiiCustomMeshComponentManager      = xiiComponentManager<class xiiCustomMeshComponent, xiiBlockStorageType::Compact>;

/// \brief This component is used to render custom geometry.
///
/// Sometimes game code needs to build geometry on the fly to visualize dynamic things.
/// The xiiDynamicMeshBufferResource is an easy to use resource to build geometry and change it frequently.
/// This component takes such a resource and takes care of rendering it.
/// The same resource can be set on multiple components to instantiate it in different locations.
class XII_GRAPHICSCORE_DLL xiiCustomMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiCustomMeshComponent, xiiRenderComponent, xiiCustomMeshComponentManager);

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
  // xiiCustomMeshComponent

public:
  xiiCustomMeshComponent();
  ~xiiCustomMeshComponent();

  /// \brief Creates a new dynamic mesh buffer.
  ///
  /// The new buffer can hold the given number of vertices and indices (either 16 bit or 32 bit).
  xiiDynamicMeshBufferResourceHandle CreateMeshResource(xiiGALPrimitiveTopology::Enum topology, xiiUInt32 uiMaxVertices, xiiUInt32 uiMaxPrimitives, xiiGALValueType::Enum indexType);

  /// \brief Returns the currently set mesh resource.
  xiiDynamicMeshBufferResourceHandle GetMeshResource() const { return m_hDynamicMesh; }

  /// \brief Sets which mesh buffer to use.
  ///
  /// This can be used to have multiple xiiCustomMeshComponent's reference the same mesh buffer,
  /// such that the object gets instanced in different locations.
  void SetMeshResource(const xiiDynamicMeshBufferResourceHandle& hMesh);

  /// \brief Configures the component to render only a subset of the primitives in the mesh buffer.
  void SetUsePrimitiveRange(xiiUInt32 uiFirstPrimitive = 0, xiiUInt32 uiNumPrimitives = xiiMath::MaxValue<xiiUInt32>());

  /// \brief Sets the bounds that are used for culling.
  ///
  /// Note: It is very important that this is called whenever the mesh buffer is modified and the size of
  /// the mesh has changed, otherwise the object might not appear or be culled incorrectly.
  void SetBounds(const xiiBoundingBoxSphere& bounds);

  /// \brief Sets the material for rendering.
  void SetMaterial(const xiiMaterialResourceHandle& hMaterial);

  /// \brief Returns the material that is used for rendering.
  xiiMaterialResourceHandle GetMaterial() const;

  void        SetMaterialFile(const char* szMaterial); // [ property ]
  const char* GetMaterialFile() const;                 // [ property ]

  /// \brief Sets the mesh instance color.
  void SetColor(const xiiColor& color); // [ property ]

  /// \brief Returns the mesh instance color.
  const xiiColor& GetColor() const; // [ property ]

  void OnMsgSetMeshMaterial(xiiMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(xiiMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const;

  xiiMaterialResourceHandle m_hMaterial;
  xiiColor                  m_Color            = xiiColor::White;
  xiiUInt32                 m_uiFirstPrimitive = 0;
  xiiUInt32                 m_uiNumPrimitives  = 0xFFFFFFFF;
  xiiBoundingBoxSphere      m_Bounds;

  xiiDynamicMeshBufferResourceHandle m_hDynamicMesh;

  virtual void OnActivated() override;
};

/// \brief Temporary data used to feed the xiiCustomMeshRenderer.
class XII_GRAPHICSCORE_DLL xiiCustomMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomMeshRenderData, xiiRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  xiiDynamicMeshBufferResourceHandle m_hMesh;
  xiiMaterialResourceHandle          m_hMaterial;
  xiiColor                           m_Color = xiiColor::White;

  xiiUInt32 m_uiFlipWinding : 1;
  xiiUInt32 m_uiUniformScale : 1;

  xiiUInt32 m_uiFirstPrimitive = 0;
  xiiUInt32 m_uiNumPrimitives  = 0xFFFFFFFF;

  xiiUInt32 m_uiUniqueID = 0;
};

/// \brief A renderer that handles all xiiCustomMeshRenderData.
class XII_GRAPHICSCORE_DLL xiiCustomMeshRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCustomMeshRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiCustomMeshRenderer);

public:
  xiiCustomMeshRenderer();
  ~xiiCustomMeshRenderer();

  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void UpdateBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) override;
  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
