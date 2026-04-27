#pragma once

#include <GraphicsCore/Components/Render/RenderComponent.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/RenderData.h>

struct xiiMsgExtractRenderData;

// ============================================================
//  Skinned Mesh Render Data
// ============================================================

class XII_GRAPHICSCORE_DLL xiiSkinnedMeshRenderData : public xiiRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderData, xiiRenderData);

public:
  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiGALBufferHandle                         m_hSkinningTransforms; ///< GPU buffer containing bone matrices
  xiiUInt32                                  m_uiCustomSeed = 0;
  bool                                       m_bCastShadows = true;
};

// ============================================================
//  Skinned Mesh Component
// ============================================================

using xiiSkinnedMeshComponentManager = xiiComponentManager<class xiiSkinnedMeshComponent, xiiBlockStorageType::Compact>;

/// \brief Renders animated meshes using GPU skinning.
///
/// This component is designed to work in tandem with an animation component
/// that updates the internal GPU bone transform buffer each frame.
class XII_GRAPHICSCORE_DLL xiiSkinnedMeshComponent : public xiiRenderComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkinnedMeshComponent, xiiRenderComponent, xiiSkinnedMeshComponentManager);

public:
  virtual void      SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void      DeserializeComponent(xiiWorldReader& inout_stream) override;
  virtual xiiResult GetLocalBounds(xiiBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, xiiMsgUpdateLocalBounds& ref_msg) override;

  xiiSkinnedMeshComponent();
  ~xiiSkinnedMeshComponent();

  // ---- Mesh ----
  void          SetMeshFile(xiiStringView sFile); // [ property ]
  xiiStringView GetMeshFile() const;              // [ property ]
  void          SetMesh(const xiiMeshResourceHandle& hMesh);

  // ---- Materials ----
  xiiUInt32                 GetMaterialCount() const;
  void                      SetMaterial(xiiUInt32 uiIndex, const xiiMaterialResourceHandle& hMaterial);
  xiiMaterialResourceHandle GetMaterial(xiiUInt32 uiIndex) const;
  void                      SetMaterialFile(xiiUInt32 uiIndex, xiiStringView sFile);
  xiiStringView             GetMaterialFile(xiiUInt32 uiIndex) const;
  void                      SetMaterial0Prop(xiiStringView s); // [ property ]
  xiiStringView             GetMaterial0Prop() const;          // [ property ]

  // ---- GPU Skinning ----
  /// \brief Updates the GPU bone matrices. Typically called by xiiAnimationComponent.
  void UpdateSkinningTransforms(xiiArrayPtr<const xiiMat4> transforms);

  // ---- Flags ----
  void SetCastShadows(bool b); // [ property ]
  bool GetCastShadows() const { return m_bCastShadows; }

protected:
  void OnMsgExtractRenderData(xiiMsgExtractRenderData& ref_msg) const;

private:
  xiiMeshResourceHandle                      m_hMesh;
  xiiDynamicArray<xiiMaterialResourceHandle> m_Materials;
  xiiGALBufferHandle                         m_hSkinningTransforms;

  xiiBoundingBoxSphere m_SkinningBounds = xiiBoundingBoxSphere::MakeZero();
  bool                 m_bCastShadows   = true;
};
