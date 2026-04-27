#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>

struct xiiMsgExtractRenderData;

// ============================================================
//  Soft Body Mesh Component
// ============================================================

using xiiSoftBodyMeshComponentManager = xiiComponentManager<class xiiSoftBodyMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders highly deformable meshes where vertices are driven directly by a physics simulation.
///
/// In robotics and MD, precise vertex deformations are required. The simulation writes directly to
/// a GPU buffer (position + normal), and this component renders it without a CPU readback.
class XII_GRAPHICSCORE_DLL xiiSoftBodyMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSoftBodyMeshComponent, xiiRenderComponent, xiiSoftBodyMeshComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiSoftBodyMeshComponent();
  ~xiiSoftBodyMeshComponent();

  // ---- Mesh Topology ----
  void          SetTopologyMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetTopologyMeshFile() const;
  void          SetTopologyMesh(const xiiMeshResourceHandle& hMesh);

  // ---- Materials ----
  xiiUInt32                 GetMaterialCount() const;
  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;
  void                      SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile);
  xiiStringView             GetMaterialFile(xiiUInt32 uiIndex) const;

  // ---- Simulation Buffer ----
  /// \brief Link the rendering to a physics buffer containing the actual vertex positions.
  void SetSimulationBuffer(xiiGALBufferHandle hVertexData, xiiUInt32 uiVertexCount, const xiiBoundingBoxSphere& bounds);

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiMeshResourceHandle                      m_hTopologyMesh; /// Provides the indices and UVs
  xiiGALBufferHandle                         m_hVertexData;   /// Driven by compute/physics
  xiiUInt32                                  m_uiVertexCount = 0;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;

  xiiBoundingBoxSphere m_SimBounds = xiiBoundingBoxSphere::MakeZero();
};

// ============================================================
//  Morph Target Component
// ============================================================

using xiiMorphTargetComponentManager = xiiComponentManager<class xiiMorphTargetComponent, xiiBlockStorageType::Compact>;

/// \brief Renders meshes with morph target (blend shape) animations.
///
/// Dispatches a compute shader to blend the active morph targets into a dynamic vertex buffer
/// before rendering. Crucial for facial animation or precise parameterized shape deformations.
class XII_GRAPHICSCORE_DLL xiiMorphTargetComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiMorphTargetComponent, xiiRenderComponent, xiiMorphTargetComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiMorphTargetComponent();
  ~xiiMorphTargetComponent();

  // ---- Mesh ----
  void          SetMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMeshFile() const;
  void          SetMesh(const xiiMeshResourceHandle& hMesh);

  // ---- Morph Weights ----
  void  SetMorphWeight(xiiStringView sShapeName, float fWeight);
  float GetMorphWeight(xiiStringView sShapeName) const;

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiMeshResourceHandle m_hMesh;
  // In a full implementation, we would map string names to weight indices.
  // We store the weights here, and they are uploaded to a CBuffer for the compute pass.
  xiiDynamicArray<float> m_MorphWeights;

  xiiGALBufferHandle m_hBlendedVertexBuffer;
};
