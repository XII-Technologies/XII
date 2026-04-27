#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

/// \brief Render data submitted per-frame by a procedural mesh component.
class XII_GRAPHICSCORE_DLL xiiProceduralMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProceduralMeshRenderData, xiiRenderData);

public:
  xiiDynamicMeshBufferResourceHandle m_hDynamicMesh;
  xiiMaterialResourceHandle          m_hMaterial;
  xiiUInt32                          m_uiVertexCount = 0;
  xiiUInt32                          m_uiIndexCount  = 0;
  bool                               m_bCastShadows  = false;
};

using xiiProceduralMeshComponentManager = xiiComponentManager<class xiiProceduralMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders runtime-generated geometry stored in a dynamic mesh buffer.
///
/// The geometry is owned externally (e.g. by a terrain system or physics debug overlay) and
/// uploaded to a xiiDynamicMeshBufferResource each frame. This component provides the render-side
/// hook: it holds the buffer handle and a single material and submits render data every frame.
class XII_GRAPHICSCORE_DLL xiiProceduralMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiProceduralMeshComponent, xiiRenderComponent, xiiProceduralMeshComponentManager);

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
  // xiiProceduralMeshComponent

public:
  xiiProceduralMeshComponent();
  ~xiiProceduralMeshComponent();

  void          SetMaterialFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMaterialFile() const;              // [ property ]

  void                             SetMaterial(const xiiMaterialResourceHandle& hMaterial);
  const xiiMaterialResourceHandle& GetMaterial() const { return m_hMaterial; }

  void SetCastShadows(bool bCast);                       // [ property ]
  bool GetCastShadows() const { return m_bCastShadows; } // [ property ]

  /// \brief Binds a dynamic mesh buffer resource that was previously created and filled by the caller.
  void SetDynamicMeshBuffer(const xiiDynamicMeshBufferResourceHandle& hBuffer, xiiUInt32 uiVertexCount, xiiUInt32 uiIndexCount);

  /// \brief Forces the local bounds to be recalculated with the given box.
  void SetBounds(const xiiBoundingBoxSphere& bounds);

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

  xiiDynamicMeshBufferResourceHandle m_hDynamicMesh;
  xiiMaterialResourceHandle          m_hMaterial;
  xiiBoundingBoxSphere               m_LocalBounds   = xiiBoundingBoxSphere::MakeInvalid();
  xiiUInt32                          m_uiVertexCount = 0;
  xiiUInt32                          m_uiIndexCount  = 0;
  bool                               m_bCastShadows  = false;
};
